#ifndef PT_NOISEMASK_H
#define PT_NOISEMASK_H

#include <iosfwd>
#include <map>
#include <set>
#include <string>

#include "utils/config.h"
#include "utils/definitions.h"

namespace Mechanics {

/** Store and process masked pixels. */
class NoiseMask {
public:
  NoiseMask() = default;

  /** Construct a noise mask from a configuration file. */
  static NoiseMask fromFile(const std::string& path);
  /** Write the noise mask to a configuration file. */
  void writeFile(const std::string& path) const;

  /** Construct noise mask from a configuration object. */
  static NoiseMask fromConfig(const toml::Value& cfg);
  /** Convert noise mask into a configuration object. */
  toml::Value toConfig() const;

  /** Merge masked pixels from another NoiseMask into this NoiseMask. */
  void merge(const NoiseMask& other);

  void maskPixel(Index sensorId, Index col, Index row);
  const std::set<ColumnRow>& getMaskedPixels(Index sensorId) const;
  size_t getNumMaskedPixels() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::map<Index, std::set<ColumnRow>> m_maskedPixels;
};

} // namespace Mechanics

#endif // PT_NOISEMASK_H
