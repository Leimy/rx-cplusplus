#include "metadata.hpp"
#include "metastate.hpp"

metastream::metastream(boost::asio::io_service& io_service,
		       std::string server, std::string port)
  : resolver_(io_service),
    socket_(io_service)
{
  std::ostream req_stream(&request_);
  req_stream << "GET /relay HTTP/1.0\r\n";
  req_stream << "Host: " << server << "\r\n";
  req_stream << "Accept: */*\r\n";
  req_stream << "Connection: close\r\n";
  req_stream << "Icy-MetaData: 1\r\n\r\n";
  tcp::resolver::query query(server, port);
  resolver_.async_resolve(query,
			  [&](const boost::system::error_code& err,
			      tcp::resolver::iterator endpoint_iterator) {
			    handle_resolve(err, endpoint_iterator);
			  });
}

metastream::~metastream(){ socket_.close(); }

void metastream::handle_resolve(const boost::system::error_code& err,
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

void metastream::handle_connect(const boost::system::error_code& err,
		    tcp::resolver::iterator endpoint_iterator) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }

  if (err && endpoint_iterator != tcp::resolver::iterator()) {
    socket_.async_connect(*endpoint_iterator,
			  [&](const boost::system::error_code& err) {
			    handle_connect(err, ++endpoint_iterator);
			  });
    return;
  }
  // The connection was successful. Send the request.
  boost::asio::async_write(socket_, request_,
			   [&](const boost::system::error_code& err,
			       const size_t amount) {
			     handle_write(err, amount);
			   });
  return;
}

void metastream::handle_write(const boost::system::error_code& err, const size_t amount) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }
  // Read the response status line.
  boost::asio::async_read_until(socket_, response_, "\r\n",
				[&](const boost::system::error_code& err,
				    const size_t amount) {
				  handle_read_status_line(err, amount);
				});
  return;
}

void metastream::handle_read_status_line(const boost::system::error_code& err, const size_t amount) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }
  std::istream response_stream(&response_);
  std::string http_version;
  response_stream >> http_version;
  unsigned int status_code;
  response_stream >> status_code;
  std::string status_message;
  std::getline(response_stream, status_message);
  if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
      std::cerr << "Invalid response\n";
      return;
    }
  if (status_code != 200)
    {
      std::cerr << "Response returned with status code ";
      std::cerr << status_code << std::endl;
      return;
    }
  // Read the response headers, which are terminated by a blank line.
  boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
				[&](const boost::system::error_code& err, const size_t amount) {
				  handle_read_headers(err, amount);
				});
  return;
}

void metastream::handle_read_headers(const boost::system::error_code& err, const size_t amount) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }

  // Process the response headers.
  std::istream response_stream(&response_);
  std::string header;
  while (std::getline(response_stream, header) && header != "\r") {
    if (header.substr(0, 12) == "icy-metaint:") {
      std::istringstream val(header.substr(12));
      val >> metaint_;
      std::clog << "Metadata space is: " << metaint_ << std::endl;
    }
  }
  boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(metaint_ + 1 - response_.size()),
			  [&](const boost::system::error_code& err, const size_t amount) {
			    handle_read_up_to_meta(err, amount);
			  });
  return;
}

void metastream::handle_read_up_to_meta(const boost::system::error_code& err, const size_t amount) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }

  response_.consume(metaint_); // drop non-meta
  std::istream in(&response_);
  char byte;
  in >> byte;
  last_metasize_ = (unsigned int) byte * 16;
  if (last_metasize_ == 0) { // nothing to do
    // but re-schedule
    boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(metaint_ + 1 - response_.size()),
			    [&](const boost::system::error_code& err, const size_t amount) {
			      handle_read_up_to_meta(err, amount);
			    });
    return;
  }

  if (response_.size() >= last_metasize_) {
    // can process metadata immediately... so do it
    process_metadata();
    return;
  }
  // or we have more to read
  boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(last_metasize_ - response_.size()),
			  [&](const boost::system::error_code& err, const size_t amount) {
			    handle_read_metadata(err, amount);
			  });
  return;
}

void metastream::handle_read_metadata(const boost::system::error_code& err, const size_t amount) {
  if (err) {
    std::cerr << "Err: " << err.message() << std::endl;
    return;
  }
  process_metadata();
  return;
}

void metastream::process_metadata() {
  std::istream in(&response_);
  char bytes[last_metasize_ + 1];
  bytes[last_metasize_] = '\0';
  if (!in.read(bytes, last_metasize_)) {
    std::cerr << "Failed to read the metadata!" << std::endl;
    return;
  }
  if (in.gcount() != last_metasize_) {
    std::cerr << "Did not read enough data. Only: " << in.gcount() << " needed: " << last_metasize_ << std::endl;
    return;
  }

  std::string s = bytes;

  // Metadata parsing - move to another function
  // Consider string views in 2017 - should be much
  // cheaper!
  s = s.substr(s.find_first_of("'")+1);

  typedef enum {START, NEXT} parserState;
  parserState state = START;
  bool done =false;
  std::string::size_type pos = 0;
  for (pos = 0; !done && pos < s.size(); ++pos) {
    switch (state) {
    case START:
      if (s[pos] == '\'') {
        state = NEXT;
      }
      break;
    case NEXT:
      if (s[pos] == ';') {
        pos -= 2; //back up for trim
        done = true;
      } else {
        state = START;
      }
    }
  }
  s = s.substr(0, pos);
  g_ms.set(s);
  std::clog << "Metadata: " << s << std::endl;

  boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least((metaint_ + 1) - response_.size()),
			  [&](const boost::system::error_code& err, const size_t amount) {
			    handle_read_up_to_meta(err, amount);
			  });
}
