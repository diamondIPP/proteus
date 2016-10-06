#include "config.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "configparser.h"

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

// create a valid toml identifier, i.e. lower case and no spaces
static std::string cfgConvertKey(std::string key)
{
  std::replace(key.begin(), key.end(), ' ', '_');
  std::transform(key.begin(), key.end(), key.begin(), ::tolower);
  return std::move(key);
}

// convert the ConfigParser value string into a toml::Value
static toml::Value cfgConvertValue(const std::string& value)
{
  static const char* truths[] = {"true", "on", "yes"};
  static const char* falses[] = {"false", "off", "no"};

  bool isBoolTrue = false;
  bool isBoolFalse = false;
  for (int i = 0; i < 3; ++i) {
    isBoolTrue ^= (value.compare(truths[i]) == 0);
    isBoolFalse ^= (value.compare(falses[i]) == 0);
  }

  if (isBoolTrue)
    return toml::Value(true);
  if (isBoolFalse)
    return toml::Value(false);
  // make your life easier by reusing toml internals
  if (toml::internal::isInteger(value))
    return toml::Value(stoi(value));
  if (toml::internal::isDouble(value))
    return toml::Value(stod(value));
  // must be just a string then
  return toml::Value(value);
}

toml::Value Utils::Config::readConfigParser(const std::string& path)
{
  toml::Value cfg = toml::Table();
  toml::Value* section;

  ConfigParser parser(path.c_str());
  for (unsigned int i = 0; i < parser.getNumRows(); ++i) {
    const ConfigParser::Row* row = parser.getRow(i);

    if (row->isHeader) {
      if (row->header.compare(0, 3, "End") == 0) {
        section = &cfg;
      } else {
        auto key = cfgConvertKey(row->header);
        auto curr = cfg.findChild(key);
        // recurring sections with the same name are automatically
        // converted to an array of the same name.
        if (curr && curr->is<toml::Array>()) {
          section = curr->push(toml::Table());
        } else if (curr) {
          section = cfg.setChild(key, toml::Array{*curr})->push(toml::Table());
        } else {
          section = cfg.setChild(key, toml::Table());
        }
      }
      continue;
    }

    section->set(cfgConvertKey(row->key), cfgConvertValue(row->value));
  }

  return cfg;
}

toml::Value Utils::Config::readConfig(const std::string& path)
{
  std::ifstream file(path, std::ifstream::in | std::ifstream::binary);
  if (!file.good())
    throw std::runtime_error("Could not open file '" + path + "' to read");

  toml::ParseResult result = toml::parse(file);
  if (!result.valid())
    throw std::runtime_error("Could not parse TOML file '" + path + "': " +
                             result.errorReason);

  return std::move(result.value);
}

void Utils::Config::writeConfig(const toml::Value& cfg, const std::string& path)
{
  std::ofstream file(path, std::ofstream::out | std::ofstream::binary);
  if (!file.good())
    throw std::runtime_error("Could not open file '" + path + "' to write");

  cfg.write(&file);
  file.close();
}
