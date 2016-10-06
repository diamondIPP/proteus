#ifndef __JD_CONFIG_H__
#define __JD_CONFIG_H__

#include <string>

#include <toml/toml.h>

namespace Utils {
namespace Config {

/** Check if the given path is an absolute path. */
bool pathIsAbsolute(const std::string& path);
/** The dirname of the path up to, but not including, the last '/'. */
std::string pathDirname(const std::string& path);
/** Split file extension from the path. */
std::string pathExtension(const std::string& path);
/** Prepend an additional directory if the given path is relative. */
std::string pathRebaseIfRelative(const std::string& path,
                                 const std::string& dir);

/** Read a ConfigParser config file and convert it to toml. */
toml::Value readConfigParser(const std::string& path);
/** Read a toml config file with automatic error handling. */
toml::Value readConfig(const std::string& path);

/** Write a toml::Value to file. */
void writeConfig(const toml::Value& cfg, const std::string& path);

template <typename T>
T get(const toml::Value& cfg, const std::string& key, const T& default_value)
{
  if (cfg.has(key))
    return cfg.get<T>(key);
  return default_value;
}

} // namespace Config
} // namespace Utils

#endif // __JD_CONFIG_H__
