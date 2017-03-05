/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-09
 */

#include "config.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Config)

bool Utils::Config::pathIsAbsolute(const std::string& path)
{
  return !path.empty() && (path.front() == '/');
}

std::string Utils::Config::pathDirname(const std::string& path)
{
  auto pos = path.find_last_of('/');
  // no slash means the path contains only a filename
  if (path.empty() || (pos == std::string::npos))
    return ".";
  // remove possible duplicates slashes at the end
  return path.substr(0, path.find_last_not_of('/', pos) + 1);
}

std::string Utils::Config::pathExtension(const std::string& path)
{
  auto pos = path.find_last_of('.');
  return ((pos != std::string::npos) ? path.substr(pos + 1) : std::string());
}

std::string Utils::Config::pathRebaseIfRelative(const std::string& path,
                                                const std::string& dir)
{
  if (pathIsAbsolute(path))
    return path;
  std::string full;
  if (!dir.empty()) {
    full = dir;
    full += '/';
  }
  full += path;
  return full;
}

toml::Value Utils::Config::readConfig(const std::string& path)
{
  std::ifstream file(path, std::ifstream::in | std::ifstream::binary);
  if (!file)
    throw std::runtime_error("Config: Could not open file '" + path +
                             "' to read");

  toml::ParseResult result = toml::parse(file);
  if (!result.valid())
    throw std::runtime_error("Config: Could not parse TOML file '" + path +
                             "': " + result.errorReason);

  return std::move(result.value);
}

void Utils::Config::writeConfig(const toml::Value& cfg, const std::string& path)
{
  std::ofstream file(path, std::ofstream::out | std::ofstream::binary);
  if (!file.good())
    throw std::runtime_error("Config: Could not open file '" + path +
                             "' to write");

  cfg.write(&file);
  file.close();
}

toml::Value Utils::Config::withDefaults(const toml::Value& cfg,
                                        const toml::Value& defaults)
{
  toml::Value combined(defaults);
  if (!combined.merge(cfg)) {
    ERROR("could not merge config w/ defaults");
    ERROR("config:\n", cfg);
    ERROR("defaults:\n", defaults);
    throw std::runtime_error("Config: could not merge defaults");
  }
  return combined;
}

std::vector<toml::Value> Utils::Config::perSensor(const toml::Value& cfg,
                                                  const toml::Value& defaults)
{
  std::vector<toml::Value> sensors =
      cfg.get<std::vector<toml::Value>>("sensors");
  toml::Value globals(cfg);
  globals.eraseChild("sensors");

  for (auto sensor = sensors.begin(); sensor != sensors.end(); ++sensor) {
    toml::Value combined(defaults);
    if (!combined.merge(globals))
      throw std::runtime_error("Config: could not merge globals");
    if (!combined.merge(*sensor))
      throw std::runtime_error("Config: could not merge sensor config");
    std::swap(combined, *sensor);
  }
  return sensors;
}
