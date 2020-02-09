#pragma once

#include "Kernel.hh"

#include <map>
#include <string>

class Config : public Tool {
public:
  Config(const char* confFile);

  template <class T>
  T get(const char* key) const;

private:
  std::map<std::string, int> intMap;
  std::map<std::string, double> floatMap;
  std::map<std::string, std::string> strMap;
};

#define BEGIN_CONFIG(confvar)                 \
{                                             \
  const Config* config__ = confvar

#define CONFIG(var)                           \
  var = config__->get<decltype(var)>(#var)

#define END_CONFIG()                          \
}