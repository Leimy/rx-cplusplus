#ifndef __METASTATE_HPP__
#define __METASTATE_HPP__
#include <pthread.h>
#include <string>
#include <functional>
#include <map>

class metastate {
private:
  mutable pthread_mutex_t mutex_;
  std::string lastsong_;
  std::map <std::string, std::function<void(std::string)>> handlers_;
public:
  metastate ();
  ~metastate ();
  std::string get () const;
  void registerHandler(std::string who, std::function<void(std::string)> what);
  void removeHandler(std::string who);
  void set (std::string metadata);
};

extern metastate g_ms;

extern "C" char * getMetaData();

#endif //__METASTATE_HPP__
