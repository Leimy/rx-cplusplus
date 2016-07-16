#include "metastate.hpp"

metastate::metastate() {
  pthread_mutex_init(&mutex_, NULL);
}

metastate::~metastate() {
  pthread_mutex_destroy(&mutex_);
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
  pthread_mutex_unlock(&mutex_);
}

metastate g_ms;

extern "C" {
char *getMetaData() {
  return strdup(g_ms.get().c_str());
}
}
