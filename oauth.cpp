#include <atomic>
#include <string>
#include <random>
#include <chrono>
#include <cmath>
#include <sstream>

namespace {
struct nonceString_ {
    std::atomic_uint_fast64_t val_;    
    nonceString_() {
	std::random_device rd;
	std::mt19937_64 rnd(rd());
	auto n = std::llround(rnd());
	n ^= std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	val_.store(n);
    }

    std::string operator () () {
	++val_;
	std::ostringstream ss;
	ss << val_.load();
	return ss.str();
    }
} nonceString;
}

#include <iostream>
int main () {
    std::cout << nonceString() << " " << nonceString() << std::endl;
}


