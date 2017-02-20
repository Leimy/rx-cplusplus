#ifndef __BOT_HPP__
#define __BOT_HPP__
#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <string>
#include <iostream>

#include "lua.hpp"
#include "metastate.hpp"
#include "metalua.hpp"

using std::string;
using boost::asio::ip::tcp;
using boost::system::error_code;
using boost::asio::strand;

std::ostream & operator << (std::ostream& os, lua_State *L);

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
  strand strand_;

  virtual ~bot ();

  bot(boost::asio::io_service &io_service,
      string server, string bot_nick, string room,
      string lua_script = "./botlogic.lua");

  void handle_resolve(error_code const& err, tcp::resolver::iterator endpoint_iterator);
  
  void handle_connect(error_code const& err, tcp::resolver::iterator endpoint_iterator);

  void operator() (error_code const &ec = error_code(), std::size_t n = 0);

  void onUpdateMetadata(std::string md);
};
#endif // __BOT_HPP__
