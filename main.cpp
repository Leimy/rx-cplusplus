#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include <thread>

#include "metadata.hpp"
#include "bot.hpp"

void usage() {
    std::cerr << "Usage: meta server port ircserver nick" << std::endl;
    exit(1);
}

int main (int argc, char ** argv) {
    if (argc < 5) {
	usage();
    }
    boost::asio::io_service io_service;
    while ( true ) {
	metastream meta(io_service, argv[1], argv[2]);
	bot bot(io_service, argv[3], argv[4], "#radioxenu");
	io_service.run();
	io_service.reset();
	std::clog << "Reconnecting!" << std::endl;
	using namespace std::literals;
	std::this_thread::sleep_for(2s);
    }
}
