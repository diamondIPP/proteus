#include "pixelmasks.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ostream>
#include <stdexcept>

#include "mechanics/device.h"
#include "mechanics/sensor.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(PixelMasks)

namespace proteus {

PixelMasks PixelMasks::fromFile(const std::string& path)
{
  auto cfg = configRead(path);
  INFO("read pixel masks from '", path, "'");
  return fromConfig(cfg);
}

void PixelMasks::writeFile(const std::string& path) const
{
  configWrite(toConfig(), path);
  INFO("wrote pixel masks to '", path, "'");
}

PixelMasks PixelMasks::fromConfig(const toml::Value& cfg)
{
  PixelMasks masks;

  auto sensors = cfg.get<toml::Array>("sensors");
  for (auto is = sensors.begin(); is != sensors.end(); ++is) {
    auto id = is->get<int>("id");
    auto pixels = is->get<toml::Array>("masked_pixels");
    for (auto ip = pixels.begin(); ip != pixels.end(); ++ip) {
      // column / row array *must* have exactly two elements
      if (ip->size() != 2)
        throw std::runtime_error("PixelMasks: column/row array size " +
                                 std::to_string(ip->size()) + " != 2");
      masks.maskPixel(id, ip->get<int>(0), ip->get<int>(1));
    }
  }
  return masks;
}

toml::Value PixelMasks::toConfig() const
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

void PixelMasks::merge(const PixelMasks& other)
{
  for (auto mask = other.m_maskedPixels.begin();
       mask != other.m_maskedPixels.end(); ++mask) {
    Index sensorId = mask->first;
    const auto& pixels = mask->second;
    m_maskedPixels[sensorId].insert(pixels.begin(), pixels.end());
  }
}

void PixelMasks::maskPixel(Index sensorId, Index col, Index row)
{
  m_maskedPixels[sensorId].emplace(col, row);
}

const std::set<ColumnRow>& PixelMasks::getMaskedPixels(Index sensorId) const
{
  static const std::set<ColumnRow> EMPTY;

  auto mask = m_maskedPixels.find(sensorId);
  if (mask != m_maskedPixels.end())
    return mask->second;
  return EMPTY;
}

size_t PixelMasks::getNumMaskedPixels() const
{
  size_t n = 0;
  for (auto it = m_maskedPixels.begin(); it != m_maskedPixels.end(); ++it)
    n += it->second.size();
  return n;
}

void PixelMasks::print(std::ostream& os, const std::string& prefix) const
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

} // namespace proteus
