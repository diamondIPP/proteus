#ifndef PT_CONFIG_H
#define PT_CONFIG_H

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

/** Set missing values using the given defaults. */
toml::Value withDefaults(const toml::Value& cfg, const toml::Value& defaults);

/** Construct per-sensor configuration with optional defaults.
 *
 * The input configuration **must** have a list of objects named `sensors`.
 * This will be used to create a vector of per-sensor configurations, one for
 * each entry. Entries are created by merging the given defaults, the global
 * part of the input configuration (without `sensors`), and the per-sensor
 * configuration.
 */
std::vector<toml::Value> perSensor(const toml::Value& cfg,
                                   const toml::Value& defaults);

} // namespace Config
} // namespace Utils

#endif // PT_CONFIG_H
