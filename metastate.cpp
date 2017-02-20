#include "metastate.hpp"

metastate::metastate() {
    if (pthread_mutex_init(&mutex_, NULL) != 0) {
	throw "mutex init failed";
    }
}

metastate::~metastate() {
    if (pthread_mutex_destroy(&mutex_) != 0) {
	throw "mutex destroy failed";
    }
}

std::string metastate::get () const {
    pthread_mutex_lock(&mutex_);
    std::string rv = lastsong_;
    pthread_mutex_unlock(&mutex_);

    return rv;
}

void metastate::set (std::string metadata) {
    pthread_mutex_lock(&mutex_);
    lastsong_ = metadata;
    for (auto&& it : handlers_) {
	it.second(metadata);
    }
    pthread_mutex_unlock(&mutex_);
}

void metastate::registerHandler(std::string who, std::function<void(std::string)> what) {
    pthread_mutex_lock(&mutex_);
    handlers_[who] = what;
    pthread_mutex_unlock(&mutex_);
}

void metastate::removeHandler(std::string who) {
    pthread_mutex_lock(&mutex_);
    auto it = handlers_.find(who);
    if (it != handlers_.end()) {
	handlers_.erase(it);
    }
    pthread_mutex_unlock(&mutex_);
}

metastate g_ms;

extern "C" {
    char *getMetaData() {
	return strdup(g_ms.get().c_str());
    }
}
