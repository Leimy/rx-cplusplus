// IRC Bot
#include <boost/asio.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

using boost::asio::ip::tcp;

// Half-witted IRC bot.

class bot {
private:
    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf outbuf_;
    boost::asio::streambuf inbuf_;
    std::string bot_nick_;
    std::string room_;

public:
    bot(boost::asio::io_service& io_service,
	std::string server, std::string bot_nick, std::string room)
	: resolver_(io_service),
	  socket_(io_service),
	  bot_nick_(bot_nick),
	  room_(room) {
	std::ostream to_server(&outbuf_);
	to_server << "NICK " << bot_nick_ << "\r\n";
	to_server << "USER " << bot_nick_ << " 0 * :tutorial bot\r\n";
	to_server << "JOIN " << room_ << "\r\n";
	tcp::resolver::query query(server, "6667");
	resolver_.async_resolve(query,
				[&](const boost::system::error_code& err,
				    tcp::resolver::iterator endpoint_iterator) {
				    handle_resolve(err, endpoint_iterator);
				});
    }

    ~bot() { socket_.close(); }

private:
    void handle_resolve(const boost::system::error_code& err,
			tcp::resolver::iterator endpoint_iterator) {
	if (err) {
	    std::cerr << "Failed to resolve: " << err.message() << std::endl;
	    return;
	}
    
	// Attempt a connection to the first endpoint in the list. Each endpoint
	// will be tried until we successfully establish a connection.
	tcp::endpoint endpoint = *endpoint_iterator;
	socket_.async_connect(endpoint,
			      [&](const boost::system::error_code& err) {
				  handle_connect(err, ++endpoint_iterator);
			      });
    }

    void handle_connect(const boost::system::error_code& err,
			tcp::resolver::iterator endpoint_iterator) {
	if (err) {
	    std::cerr << "Err: " << err.message() << std::endl;
	    return;
	}
    
	if (err && endpoint_iterator != tcp::resolver::iterator()) {
	    socket_.close();
	    tcp::endpoint endpoint = *endpoint_iterator;
	    socket_.async_connect(endpoint,
				  [&](const boost::system::error_code& err) {
				      handle_connect(err, ++endpoint_iterator);
				  });
	    return;
	}
	// The connection was successful. Send the request.
	boost::asio::async_write(socket_, outbuf_,
				 [&](const boost::system::error_code& err,
				     const size_t amount) {
				     handle_write(err, amount);
				 });
	return;
    }

    void handle_write(const boost::system::error_code& err, const size_t amount) {
	if (err) {
	    std::cerr << "Err: " << err.message() << std::endl;
	    return;
	}
	boost::asio::async_read_until(socket_, inbuf_, "\r\n",
				      [&](const boost::system::error_code& err,
					  const size_t amount) {
					  process_line(err, amount);
				      });
	return;
    }

    void process_line(const boost::system::error_code& err, const size_t amount) {
	std::istream in(&inbuf_);
	std::string line;
	getline(in, line);
	if (line.substr(0,6) == "PING :") {
	    std::cerr << "Got a PING\n";
	    std::ostream out(&outbuf_);
	    out << line.replace(0, 4, "PONG") << "\r\n";
	    boost::asio::async_write(socket_, outbuf_,
				     [&](const boost::system::error_code& err,
					  const size_t amount) {
					 handle_write(err, amount);
				     });
	}
	std::cerr << "From IRC: " << line << std::endl;
	boost::asio::async_read_until(socket_, inbuf_, "\r\n",
				      [&](const boost::system::error_code& err,
					  const size_t amount) {
					  process_line(err, amount);
				      });
    }
};
