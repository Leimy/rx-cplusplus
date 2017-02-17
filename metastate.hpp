#ifndef __METASTATE_HPP__
#define __METASTATE_HPP__
#include <pthread.h>
#include <string>

class metastate {
private:
  mutable pthread_mutex_t mutex_;
  std::string lastsong_;
public:
  metastate ();
  ~metastate ();
  std::string get () const;
  void set (std::string metadata);
};

extern metastate g_ms;

extern "C" char * getMetaData();

#endif //__METASTATE_HPP__
