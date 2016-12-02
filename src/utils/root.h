/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_ROOT_H
#define PT_ROOT_H

#include <cassert>
#include <stdexcept>
#include <string>

#include <TDirectory.h>

namespace Utils {

/** Create a directory relative to the parent or return an existing one. */
inline TDirectory* makeDir(TDirectory* parent, const std::string& path)
{
  assert(parent && "Parent directory must be non-NULL");

  TDirectory* dir = parent->GetDirectory(path.c_str());
  if (!dir)
    dir = parent->mkdir(path.c_str());
  if (!dir)
    throw std::runtime_error("Could not create ROOT directory '" + path + '\'');
  return dir;
}

} // namespace Utils

#endif // PT_ROOT_H
