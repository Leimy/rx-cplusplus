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

constexpr bool noEscape__ (uint8_t i) {
    if (i >= 'A' && i <= 'Z')
	return true;
    if (i >= 'a' && i <= 'z')
	return true;
    if (i >= '0' && i <= '9')
	return true;
    if (i == '-' || i == '.' || i == '_' || i == '~')
	return true;
    return false;
}

struct noEscape_ {
    bool data[256];
    noEscape_() {
	for (int i = 0; i < 256; ++i) {
	    data[i] = noEscape__(i);
	}
    }
    bool operator () (uint8_t i) { return data[i]; }
} noEscape;

std::string encode(std::string input, bool twice)
{
    const char * encodeStr = "0123456789ABCDEF";
    std::string result = "";

    for (int i = 0; i < input.size(); ++i) {
	if (noEscape(input[i])) {
	    result += input[i];
	} else if (twice) {
	    result += "%25";
	    result += encodeStr[input[i] >> 4];
	    result += encodeStr[input[i] & 15];
	} else {
	    result += "%";
	    result += encodeStr[input[i] >> 4];
	    result += encodeStr[input[i] & 15];
	}
    }

    return result;
}

}

#include <iostream>
int main () {
    std::cout << nonceString() << " " << nonceString() << std::endl;
}


