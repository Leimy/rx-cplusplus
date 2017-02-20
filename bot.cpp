#include "bot.hpp"

std::ostream & operator << (std::ostream& os, lua_State *L)
{
  int i;
  int top = lua_gettop(L);  /* depth of the stack */
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
    case LUA_TSTRING: {  /* strings */
      os << lua_tostring(L, i);
      break;
    }
    case LUA_TBOOLEAN: {  /* booleans */
      os << (lua_toboolean(L, i) ? "true" : "false");
      break;
    }
    case LUA_TNUMBER: {  /* numbers */
      os << lua_tonumber(L, i);
      break;
    }
    default: {  /* other values */
      os << lua_typename(L, t);
      break;
    }
    }
    os << "  ";  /* put a separator */
  }
  os << "\n";  /* end the listing */
  return os;
}

bot::~bot () {
  g_ms.removeHandler(bot_nick_);
  lua_close(ls_);
}

bot::bot(boost::asio::io_service &io_service,
    string server, string bot_nick, string room,
    string lua_script)
  : resolver_(io_service),
    socket_(io_service),
    bot_nick_(bot_nick),
    room_(room),
    server_(server),
    timer_(io_service, boost::posix_time::seconds(1)),
    strand_(io_service)
{
  // TODO: errors
  ls_ = luaL_newstate();
  luaL_openlibs(ls_);
  // see if this works
  luaL_requiref(ls_, "md", luaopen_libmetalua, 1);  // register our C funcs
  // gaaaah
  luaL_loadfile(ls_, lua_script.c_str());
  lua_pcall(ls_, 0, 0, 0);  // load it up!
  lua_getglobal(ls_, "parameters"); // tell it where we is!
  lua_pushstring(ls_, "channel");
  lua_pushstring(ls_, room.c_str());
  std::cerr << "Configuring lua...\n";
  lua_settable(ls_, -3);
  lua_pop(ls_, 1);
  // Lua is now configured...

  // register our bot handler with the metastate
  g_ms.registerHandler(bot_nick_, [&](std::string s) { onUpdateMetadata(s); });


  tcp::resolver::query query(server_, "6667");
  resolver_.async_resolve(query,
			  [&](error_code const &err,
			      tcp::resolver::iterator endpoint_iterator) {
			    handle_resolve(err, endpoint_iterator);
			  });
}

void bot::handle_resolve(error_code const& err, tcp::resolver::iterator endpoint_iterator) {
  if (err) {
    std::cerr << "Failed to resolve: " << err.message() << std::endl;
    return;
  }
  socket_.async_connect(*endpoint_iterator,
			[&](error_code const &err) {
			  handle_connect(err, ++endpoint_iterator);
			});
}

void bot::handle_connect(error_code const& err, tcp::resolver::iterator endpoint_iterator) {
  if (err && endpoint_iterator != tcp::resolver::iterator()) {
    socket_.async_connect(*endpoint_iterator,
			  [&](error_code const& err) {
			    handle_connect(err, ++endpoint_iterator);
			  });
    return;
  }
  else if (err) {
    std::cerr << "Failed to resolve: " << err.message() << "\n";
    return;
  }

  timer_.async_wait([&](error_code const &ec) {
      strand_.post([=]{
          (*this)(ec, 0);
      });
    });
}

void bot::operator() (error_code const &ec, std::size_t n) {
  auto continuation = [=](error_code const &ec, std::size_t n) {
    strand_.post([=]{(*this)(ec,n);});
    //    (*this)(ec,c);
  };
  std::ostream out(&outbuf_);
  std::istream in(&inbuf_);
  std::string line;

  if (!ec) reenter (this) {
      out << "NICK " << bot_nick_ << "\r\n";
      out << "USER " << bot_nick_ << " 0 * :tutorial bot\r\n";
      out << "JOIN " << room_ << "\r\n";
      std::cerr << "Writing: login stuff...\n";
      yield boost::asio::async_write(socket_, outbuf_, continuation);
      for (;;) {
	yield boost::asio::async_read_until(socket_, inbuf_, "\r\n", continuation);
	// We process all the IRC lines via our lua configuration file.
	getline(in, line);

	// Call lua's entry
	lua_getglobal(ls_, "fromirc");
	// with the argument
	lua_pushlstring(ls_, line.c_str(), line.size());
	// expecting bool, <result>
	if (lua_pcall(ls_, 1, 2, 0) != LUA_OK) { // One argument in, two out!
	  std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
	  throw "lua call failed";
	}
	// Sanity
	if (!lua_isboolean(ls_, -1)) {
	  std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
	  throw "lua: unexpected result";
	}
	// Check success
	if (lua_toboolean(ls_, -1)) {
	  lua_pop(ls_, 1); // cleanup bool, we're done with it
	  // Multi-line response from lua is in a table of strings
	  if (lua_istable(ls_, -1)) {
	    // stack is just -1 = table
	    lua_pushnil(ls_);
	    // stack is  now -1 = nil, -2 = table
	    while (lua_next(ls_, -2) != 0) {
	      // stack is now -1 = value, -2 = key, -3 = table
	      out << lua_tostring(ls_, -1) << "\r\n";
	      yield boost::asio::async_write(socket_, outbuf_, continuation);
	      lua_pop(ls_, 1);
	      // stack is now  -1 = key, -2 = table
	      timer_.expires_from_now(boost::posix_time::seconds(1));
	      timer_.wait();
	    }
	    // stack is now -1 = key, -2 = table
	    lua_pop(ls_, 1);
	  }
	  // Single line response is just a string
	  else if (lua_isstring(ls_, -1)) {
	    out << lua_tostring(ls_, -1) << "\r\n";
	    yield boost::asio::async_write(socket_, outbuf_, continuation);
	    lua_pop(ls_, 1);  // cleanup the string
	  }
	}
	else {
	  lua_pop(ls_, 2);
	}
      }
    }
  // ASIO errors
  else {
    std::cerr << "Got an error!(" << ec.message() << ")\n";
    return;
  }
}

void bot::onUpdateMetadata(std::string md) {
    strand_.post(
	[=]{
	    // Call lua's entry
	    lua_getglobal(ls_, "onUpdateMetadata");
	    // with the argument
	    lua_pushlstring(ls_, md.c_str(), md.size());
	    // expecting bool, <result>
	    if (lua_pcall(ls_, 1, 2, 0) != LUA_OK) { // One argument in, two out!
		std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
		throw "lua call failed";
	    }
	    // Sanity
	    if (!lua_isboolean(ls_, -1)) {
		std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
		throw "lua: unexpected result";
	    }
	    // Check success - we should write the metadata
	    if (lua_toboolean(ls_, -1)) {
		lua_pop(ls_, 1); // cleanup bool, we're done with it

		if (lua_isstring(ls_, -1)) {
		    std::ostream out(&outbuf_);
		    out << lua_tostring(ls_, -1) << "\r\n";
		    boost::asio::async_write(socket_, outbuf_,
					     [&](const boost::system::error_code& err, size_t bytes) {
						 if (err) {
						     std::cerr << "Failed to write metadata to channel: " << err.message() << "\n";
						 }
					     });
		    lua_pop(ls_, 1); // cleanup string
		}
	    }
	    else {
		lua_pop(ls_, 2);
	    }
	});
}
