#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "utils/definitions.h"

namespace Loopers {
class NoiseScanConfig;
}

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

  /** Create empty noise masks with the default config. */
  NoiseMask();
  /** Create an empty noise mask with the given scan config. */
  explicit NoiseMask(const Loopers::NoiseScanConfig* config);
  ~NoiseMask();

  /** Read noise masks and configuration from a file. */
  void readFile(const std::string& path);
  /** Write the current noise masks and configuration to a file. */
  void writeFile(const std::string& path) const;

  void maskPixel(Index sensorId, Index col, Index row);
  const ColumnRowSet& getMaskedPixels(Index sensorId) const;
  const size_t getNumMaskedPixels() const;

  const std::string& getFileName() { return m_path; }
  const Loopers::NoiseScanConfig* getConfig() const { return m_cfg.get(); }

private:
  void parseLine(std::stringstream& line, Index& nsens, Index& x, Index& y);
  void parseComments(std::stringstream& comments);

private:
  std::map<Index, ColumnRowSet> m_maskedPixels;
  std::unique_ptr<Loopers::NoiseScanConfig> m_cfg;
  std::string m_path;
};

} // namespace Mechanics

#endif // NOISEMASK_H
