#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

using std::shared_ptr;
using std::vector;
using std::string;
using boost::asio::ip::tcp;
using boost::system::error_code;

struct bot : boost::asio::coroutine {
  shared_ptr<tcp::resolver> resolver_;
  shared_ptr<tcp::socket> socket_;
  string bot_nick_;
  string room_;
  string server_;
  shared_ptr<boost::asio::deadline_timer> timer_;

  bot(boost::asio::io_service &io_service,
      string server, string bot_nick, string room)
    : resolver_(std::make_shared<tcp::resolver>(io_service)),
      socket_(std::make_shared<tcp::socket>(io_service)),
      bot_nick_(bot_nick),
      room_(room),
      server_(server),
      timer_(std::make_shared<boost::asio::deadline_timer>(io_service, boost::posix_time::seconds(5)))
  {
    tcp::resolver::query query(server_, "6667");
    resolver_->async_resolve(query,
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

    socket_->async_connect(*endpoint_iterator,
			   [&](error_code const &err) {
			     handle_connect(err, ++endpoint_iterator);
			   });

  }

  void handle_connect(error_code const& err, tcp::resolver::iterator endpoint_iterator) {
    if (err && endpoint_iterator != tcp::resolver::iterator()) {
      socket_->async_connect(*endpoint_iterator,
			    [&](const error_code& err) {
			      handle_connect(err, ++endpoint_iterator);
			    });
      return;
    }
    else if (err) {
      std::cerr << "Failed to resolve: " << err.message() << "\n";
      return;
    }

    timer_->async_wait([&](error_code const &ec) {
	(*this)(ec, 0);
      });
  }

  void operator() (error_code const &ec = error_code(), std::size_t n = 0) {
    static boost::asio::streambuf outbuf;
    static boost::asio::streambuf inbuf;

    std::ostream out(&outbuf);
    std::istream in(&inbuf);
    std::string line;

    if (!ec) reenter (this) {
	out << "NICK " << bot_nick_ << "\r\n";
	out << "USER " << bot_nick_ << " 0 * :tutorial bot\r\n";
	out << "JOIN " << room_ << "\r\n";
	std::cerr << "Writing: login stuff...\n";
	yield boost::asio::async_write(*socket_, outbuf, *this);
      for (;;) {
	yield boost::asio::async_read_until(*socket_, inbuf, "\r\n", *this);
	getline(in, line);
	if (line.substr(0,6) == "PING :") {
	  std::cerr << "Got a PING\n";
	  out << line.replace(0, 4, "PONG") << "\r\n";
	  std::cerr << "PONGING!\n";
	  yield boost::asio::async_write(*socket_, outbuf, *this);
	}
	std::cerr << "<IRC>  " << line << std::endl;
      }
    }
    else {
      std::cerr << "Shit, got an error!(" << ec.message() << ")\n";
      return;
    }
  }
};
