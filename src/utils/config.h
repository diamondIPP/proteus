#ifndef __JD_CONFIG_H__
#define __JD_CONFIG_H__

#include <string>

#include <toml/toml.h>

namespace Utils {
namespace Config {

/** Read a config file, either .cfg or .toml, and convert it to toml::Value. */
toml::Value readConfig(const std::string& path);

/** Write a toml::Value to file. */
void writeConfig(const toml::Value& cfg, const std::string& path);

template <typename T> T get(const toml::Value& cfg, const std::string& key)
{
  return cfg.get<T>(key);
}

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
