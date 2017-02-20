#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include <thread>

#include "metadata.hpp"
#include "bot.hpp"
#include "metastate.hpp"


void usage() {
  std::cerr << "Usage: meta server port ircserver nick" << std::endl;
  exit(1);
}

int main (int argc, char ** argv) {
  if (argc < 5) {
    usage();
  }

  try {
  while ( true ) {
    boost::asio::io_service io_service;
    metastream meta(io_service, argv[1], argv[2]);
    bot bot(io_service, argv[3], argv[4], "#radioxenu");

    io_service.run();
    std::clog << "Reconnecting!" << std::endl;
    using namespace std::literals;
    std::this_thread::sleep_for(2s);
  }
  }
  catch (char const * w) {
    std::cerr << "Dangit: " << w << std::endl;
    return -1;
  }
}
