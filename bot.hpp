#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <string>
#include <iostream>

#include "lua.hpp"

using std::string;
using boost::asio::ip::tcp;
using boost::system::error_code;

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

struct bot : boost::asio::coroutine {
  tcp::resolver resolver_;
  tcp::socket socket_;
  string bot_nick_;
  string room_;
  string server_;
  boost::asio::deadline_timer timer_;
  boost::asio::streambuf outbuf_;
  boost::asio::streambuf inbuf_;
  lua_State *ls_;

  virtual ~bot () {
    lua_close(ls_);
  }

  bot(boost::asio::io_service &io_service,
      string server, string bot_nick, string room,
      string lua_script = "./botlogic.lua")
    : resolver_(io_service),
      socket_(io_service),
      bot_nick_(bot_nick),
      room_(room),
      server_(server),
      timer_(io_service, boost::posix_time::seconds(5))
  {
    // TODO: errors
    ls_ = luaL_newstate();
    luaL_openlibs(ls_);
    luaL_loadfile(ls_, lua_script.c_str());
    std::cerr << ls_ << std::endl;
    lua_pcall(ls_, 0, 0, 0);  // load it up!
    std::cerr << ls_ << std::endl;

    lua_getglobal(ls_, "parameters"); // tell it where we is!
    lua_pushstring(ls_, "channel");
    lua_pushstring(ls_, room.c_str());
    std::cerr << "Configuring lua...\n";
    std::cerr << ls_ << std::endl;
    lua_settable(ls_, -3);
    std::cerr << ls_ << std::endl;
    lua_pop(ls_, 1);
    std::cerr << ls_ << std::endl;

    tcp::resolver::query query(server_, "6667");
    resolver_.async_resolve(query,
			    [&](error_code const &err,
				tcp::resolver::iterator endpoint_iterator) {
			      handle_resolve(err, endpoint_iterator);
			    });
  }

  void handle_resolve(error_code const& err, tcp::resolver::iterator endpoint_iterator) {
    if (err) {
      std::cerr << "Failed to resolve: " << err.message() << std::endl;
      return;
    }

    socket_.async_connect(*endpoint_iterator,
			   [&](error_code const &err) {
			     handle_connect(err, ++endpoint_iterator);
			   });
  }

  void handle_connect(error_code const& err, tcp::resolver::iterator endpoint_iterator) {
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
	(*this)(ec, 0);
      });
  }

  void operator() (error_code const &ec = error_code(), std::size_t n = 0) {
    auto continuation = [&](error_code const &ec, std::size_t n){(*this)(ec,n);};
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
	lua_getglobal(ls_, "fromirc");
	lua_pushlstring(ls_, line.c_str(), line.size());
	if (lua_pcall(ls_, 1, 2, 0) != LUA_OK) { // One argument in, two out!
	  std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
	  throw "lua call failed";
	}
	if (!lua_isboolean(ls_, -1)) {
	  std::cerr << "ERROR:\nLSTACK: " << ls_ << "\n";
	  throw "lua: unexpected result";
	}
	if (lua_toboolean(ls_, -1)) {
	  std::cerr << "Sending back: " << lua_tostring(ls_, -2) << std::endl;
	  out << lua_tostring(ls_, -2) << "\r\n";
	  yield boost::asio::async_write(socket_, outbuf_, continuation);
	}
	lua_pop(ls_, 2);
      }
    }
    else {
      std::cerr << "Got an error!(" << ec.message() << ")\n";
      return;
    }
  }
};
