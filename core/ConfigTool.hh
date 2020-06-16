#pragma once

// TODO: Check that each variable has been read before we start the event
// processing loop. This should help to catch typos in the config files.

#include "Kernel.hh"

#include <map>
#include <optional>
#include <string>

class Config : public Tool {
public:
  Config(const char* confFile);

  template <class T>
  T get(const char* key, std::optional<T> dflt = std::nullopt) const;

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

template <>
int Config::get<int>(const char* key,
                     std::optional<int> dflt) const;

template <>
double Config::get<double>(const char* key,
                           std::optional<double> dflt) const;

template <>
float Config::get<float>(const char* key,
                         std::optional<float> dflt) const;

template <>
std::string Config::get<std::string>(const char* key,
                                     std::optional<std::string> dflt) const;
