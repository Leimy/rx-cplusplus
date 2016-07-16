#ifndef __METATDATA_HPP_
#define __METATDATA_HPP_

// Icecast Metadata grabber
#include <boost/asio.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include "metastate.hpp"

using boost::asio::ip::tcp;

// bone-headed HTTP client for ripping icecast metadata from
// live streams that support icy-metadata headers
// needs a little work, and it's currently hard-coded to the /relay
// endpoint

class metastream {
private:
    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    size_t metaint_;
    size_t last_metasize_;

public:
  metastream(boost::asio::io_service& io_service, std::string server, std::string port);
  ~metastream();

private:
  void handle_resolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);
  void handle_connect(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);
  void handle_write(const boost::system::error_code& err, const size_t amount);
  void handle_read_status_line(const boost::system::error_code& err, const size_t amount);
  void handle_read_headers(const boost::system::error_code& err, const size_t amount);
  void handle_read_up_to_meta(const boost::system::error_code& err, const size_t amount);
  void handle_read_metadata(const boost::system::error_code& err, const size_t amount);
  void process_metadata();
};

#endif // __METADATA_HPP_
