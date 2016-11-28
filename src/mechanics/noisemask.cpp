#include "noisemask.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ostream>
#include <stdexcept>

#include "mechanics/device.h"
#include "mechanics/sensor.h"
#include "utils/configparser.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

static void parseLine(std::stringstream& line, Index& nsens, Index& x, Index& y)
{
  Index counter = 0;
  while (line.good()) {
    std::string value;
    std::getline(line, value, ',');
    std::stringstream convert(value);
    switch (counter) {
    case 0:
      convert >> nsens;
      break;
    case 1:
      convert >> x;
      break;
    case 2:
      convert >> y;
      break;
    }
    counter++;
  }
  if (counter != 3)
    throw std::runtime_error("NoiseMask: failed to parse line");
}

static void parseFile(const std::string& path, Mechanics::NoiseMask& mask)
{
  std::fstream input(path, std::ios_base::in);

  if (!input)
    throw std::runtime_error("NoiseMask: failed to open file '" + path + "'");

  std::stringstream comments;
  std::size_t numLines = 0;
  while (input) {
    unsigned int nsens = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    std::string line;
    std::getline(input, line);
    if (!line.size())
      continue;
    if (line[0] == '#') {
      comments << line << "\n";
      continue;
    }
    std::stringstream lineStream(line);
    parseLine(lineStream, nsens, x, y);
    mask.maskPixel(nsens, x, y);
    numLines += 1;
  }

  if (numLines == 0)
    throw std::runtime_error("NoiseMask: empty file '" + path + "'");
}

Mechanics::NoiseMask Mechanics::NoiseMask::fromFile(const std::string& path)
{
  NoiseMask mask;

  if (Utils::Config::pathExtension(path) == "toml") {
    mask = fromConfig(Utils::Config::readConfig(path));
  } else {
    parseFile(path, mask);
  }
  INFO("read noise mask from '", path, "'");
  return mask;
}

void Mechanics::NoiseMask::writeFile(const std::string& path) const
{
  Utils::Config::writeConfig(toConfig(), path);
  INFO("wrote noise mask to '", path, "'");
}

Mechanics::NoiseMask Mechanics::NoiseMask::fromConfig(const toml::Value& cfg)
{
  NoiseMask mask;

  auto sensors = cfg.get<toml::Array>("sensors");
  for (auto is = sensors.begin(); is != sensors.end(); ++is) {
    auto id = is->get<int>("id");
    auto pixels = is->get<toml::Array>("masked_pixels");
    for (auto ip = pixels.begin(); ip != pixels.end(); ++ip) {
      // column / row array *must* have exactly two elements
      if (ip->size() != 2)
        throw std::runtime_error("NoiseMask: column/row array size " +
                                 std::to_string(ip->size()) + " != 2");
      mask.maskPixel(id, ip->get<int>(0), ip->get<int>(1));
    }
  }
  return mask;
}

toml::Value Mechanics::NoiseMask::toConfig() const
{
  toml::Value cfg;
  cfg["sensors"] = toml::Array();

  for (auto im = m_maskedPixels.begin(); im != m_maskedPixels.end(); ++im) {
    int id = static_cast<int>(im->first);
    const auto& pixels = im->second;

    toml::Array cfgPixels;
    for (auto ip = pixels.begin(); ip != pixels.end(); ++ip) {
      cfgPixels.emplace_back(toml::Array{static_cast<int>(ip->first),
                                         static_cast<int>(ip->second)});
    }

    toml::Value cfgSensor;
    cfgSensor["id"] = static_cast<int>(id);
    cfgSensor["masked_pixels"] = std::move(cfgPixels);
    cfg["sensors"].push(std::move(cfgSensor));
  }
  return cfg;
}

void Mechanics::NoiseMask::merge(const NoiseMask& other)
{
  for (auto mask = other.m_maskedPixels.begin();
       mask != other.m_maskedPixels.end(); ++mask) {
    Index sensorId = mask->first;
    const auto& pixels = mask->second;
    m_maskedPixels[sensorId].insert(pixels.begin(), pixels.end());
  }
}

void Mechanics::NoiseMask::maskPixel(Index sensorId, Index col, Index row)
{
  m_maskedPixels[sensorId].emplace(col, row);
}

const std::set<ColumnRow>&
Mechanics::NoiseMask::getMaskedPixels(Index sensorId) const
{
  static const std::set<ColumnRow> EMPTY;

  auto mask = m_maskedPixels.find(sensorId);
  if (mask != m_maskedPixels.end())
    return mask->second;
  return EMPTY;
}

size_t Mechanics::NoiseMask::getNumMaskedPixels() const
{
  size_t n = 0;
  for (auto it = m_maskedPixels.begin(); it != m_maskedPixels.end(); ++it)
    n += it->second.size();
  return n;
}

void Mechanics::NoiseMask::print(std::ostream& os,
                                 const std::string& prefix) const
{
  if (m_maskedPixels.empty()) {
    os << prefix << "no masked pixels\n";
    os.flush();
    return;
  }

  for (auto im = m_maskedPixels.begin(); im != m_maskedPixels.end(); ++im) {
    Index id = im->first;
    const auto& pixels = im->second;

    if (pixels.empty())
      continue;

    os << prefix << "sensor " << id << ":\n";
    for (auto cr = pixels.begin(); cr != pixels.end(); ++cr)
      os << prefix << "  col=" << cr->first << ", row=" << cr->second << '\n';
  }
  os.flush();
}
