#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <fstream>
#include <map>
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

    /** Create an empty noise mask with the given scan config. */
    explicit NoiseMask(const Loopers::NoiseScanConfig* config);
    /** Create a noise mask by reading from a noise mask file. */
    NoiseMask(const std::string& path);
    ~NoiseMask();

    /** Write the current noise masks and configuration to a file. */
    void writeFile(const std::string& path) const;

    void maskPixel(Index sensor_id, Index col, Index row);
    const ColumnRowSet& getMaskedPixels(Index sensor_id) const;
    const size_t getNumMaskedPixels() const;

    const char* getFileName() { return _fileName.c_str(); }
    const Loopers::NoiseScanConfig* getConfig() const { return _config; }

  private:
    void readFile(const std::string& path);
    void parseLine(std::stringstream& line, Index& nsens, Index& x, Index& y);
    void parseComments(std::stringstream& comments);

  private:
    std::map<Index,ColumnRowSet> _masks;
    Loopers::NoiseScanConfig* _config;
    std::string _fileName;

  }; // end of class

} // end of namespace

#endif // NOISEMASK_H
