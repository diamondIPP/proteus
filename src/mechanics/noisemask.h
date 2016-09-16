#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <iosfwd>
#include <map>
#include <set>
#include <string>

#include "utils/definitions.h"

namespace Mechanics {

/** Store and process masked pixels. */
class NoiseMask {
public:
  using ColumnRowSet = std::set<ColumnRow>;

  /** Construct a noise mask from a configuration file. */
  static NoiseMask fromFile(const std::string& path);

  NoiseMask() = default;

  /** Write the current noise masks and configuration to a file. */
  void writeFile(const std::string& path) const;

  void maskPixel(Index sensorId, Index col, Index row);
  const ColumnRowSet& getMaskedPixels(Index sensorId) const;
  size_t getNumMaskedPixels() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::map<Index, ColumnRowSet> m_maskedPixels;
};

} // namespace Mechanics

#endif // NOISEMASK_H
