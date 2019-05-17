/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2018-07
 */

#ifndef PT_OPEN_H
#define PT_OPEN_H

#include <memory>
#include <string>

#include "loop/reader.h"

namespace toml {
class Value;
}
namespace proteus {

/** Open an event file with automatic determination of the file type.
 *
 * \param path  Path to the file to be opened
 * \param cfg   Configuration that will be passed to the reader
 */
std::shared_ptr<Reader> openRead(const std::string& path,
                                 const toml::Value& cfg);

} // namespace proteus

#endif // PT_OPEN_H
