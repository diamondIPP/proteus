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

namespace Loopers { class NoiseScanConfig; }

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

    void maskPixel(Index sensor_id, Index col, Index row);
    const ColumnRowSet& getMaskedPixels(Index sensor_id) const;
    const size_t getNumMaskedPixels() const;

    const char* getFileName() { return _fileName.c_str(); }
    const Loopers::NoiseScanConfig* getConfig() const { return _config.get(); }

  private:
    void parseLine(std::stringstream& line, Index& nsens, Index& x, Index& y);
    void parseComments(std::stringstream& comments);

  private:
    std::map<Index,ColumnRowSet> _masks;
    std::unique_ptr<Loopers::NoiseScanConfig> _config;
    std::string _fileName;

  }; // end of class

} // end of namespace

#endif // NOISEMASK_H
