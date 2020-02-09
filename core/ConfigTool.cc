#include "ConfigTool.hh"

#include <fstream>
#include <string>
#include <tuple>
#include <stdexcept>

template <>
int Config::get<int>(const char* key) const
{
  if (intMap.count(key))
    return intMap.at(key);

  auto msg = Form("Couldn't find int '%s'", key);
  throw std::runtime_error(msg);
}

template <>
double Config::get<double>(const char* key) const
{
  if (floatMap.count(key))
    return floatMap.at(key);

  // in case there's no decimal in the conf file
  else if (intMap.count(key))
    return intMap.at(key);

  auto msg = Form("Couldn't find float '%s'", key);
  throw std::runtime_error(msg);
}

template <>
float Config::get<float>(const char* key) const
{
  return float(get<double>(key));
}

template <>
std::string Config::get<std::string>(const char* key) const
{
  if (strMap.count(key))
    return strMap.at(key);

  auto msg = Form("Couldn't find str '%s'", key);
  throw std::runtime_error(msg);
}

static std::string trim(const std::string& str,
                        const std::string& whitespace = " \t")
{
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

static std::string strip_comment(const std::string& line)
{
  const auto hash_pt = line.find_first_of('#');

  if (hash_pt == std::string::npos)
    return line;
  else
    return trim(line.substr(0, hash_pt));
}

static std::tuple<std::string, std::string>
split_line(const std::string& line)
{
  const auto firstWS = line.find_first_of(" \t");
  const auto key = trim(line.substr(0, firstWS));
  const auto val = trim(line.substr(firstWS, line.length() - firstWS));

  return {key, val};
}

Config::Config(const char* confFile)
{
  std::ifstream ifs(confFile);

  if (!ifs.is_open() || !ifs.good()) {
    const char* msg = Form("Couldn't open %s", confFile);
    throw std::runtime_error(msg);
  }

  std::string line;

  while (getline(ifs, line)) {
    line = strip_comment(trim(line));
    if (line.empty()) continue;

    const auto [key, valStr] = split_line(line);

    try {
      const bool maybeFloat = valStr.find('.') != std::string::npos;
      if (maybeFloat)
        floatMap[key] = stod(valStr);
      else
        intMap[key] = stoi(valStr);
    } catch (std::invalid_argument&) {
      strMap[key] = valStr;
    }
  }
}
