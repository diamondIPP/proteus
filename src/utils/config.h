#ifndef __JD_CONFIG_H__
#define __JD_CONFIG_H__

#include <string>

#include <toml/toml.h>

namespace Utils {
namespace Config {

/** Check if the given path is an absolute path. */
inline bool pathIsAbsolute(const std::string& path)
{
  return !path.empty() && (path.front() == '/');
}

/** The dirname of the path up to, but not including, the last '/'. */
inline std::string pathDirname(const std::string& path)
{
  auto pos = path.find_last_of('/');
  // no slash means the path contains only a filename
  if (path.empty() || (pos == std::string::npos))
    return std::string('.', 1);
  // remove possible duplicates slashes at the end
  return path.substr(0, path.find_last_not_of('/', pos) + 1);
}

/** Split file extension from the path. */
inline std::string pathExtension(const std::string& path)
{
  auto pos = path.find_last_of('.');
  return ((pos != std::string::npos) ? path.substr(pos + 1) : std::string());
}

/** Join two paths with correct handling of empty paths. */
inline std::string pathJoin(const std::string& path0, const std::string& path1)
{
  return path0 + (path0.empty() ? std::string() : std::string('/', 1)) + path1;
}

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
