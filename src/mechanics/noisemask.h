#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <map>
#include <set>
#include <string>

#include "loopers/noisescan.h"
#include "utils/definitions.h"

namespace Mechanics {

/** @class NoiseMask
 **
 ** @brief Store masked pixels for multiple sensors
 **
 ** Writes and reads from a textfile of all noisy pixels
 ** in the format:
 **      sensor, x, y
 */
class NoiseMask {
public:
  using ColumnRowSet = std::set<ColumnRow>;

  /** Construct a noise mask from a configuration file. */
  static NoiseMask fromFile(const std::string& path);

  /** Create empty noise masks with the default config. */
  NoiseMask();
  /** Create an empty noise mask with the given scan config. */
  explicit NoiseMask(const Loopers::NoiseScanConfig* config);

  /** Write the current noise masks and configuration to a file. */
  void writeFile(const std::string& path) const;

  void maskPixel(Index sensorId, Index col, Index row);
  const ColumnRowSet& getMaskedPixels(Index sensorId) const;
  const size_t getNumMaskedPixels() const;

  const std::string& getFileName() const { return m_path; }
  const Loopers::NoiseScanConfig* getConfig() const { return &m_cfg; }

private:
  std::map<Index, ColumnRowSet> m_maskedPixels;
  Loopers::NoiseScanConfig m_cfg;
  std::string m_path;
};

} // namespace Mechanics

#endif // NOISEMASK_H
