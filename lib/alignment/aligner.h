#pragma once

#include "loop/analyzer.h"
#include "mechanics/geometry.h"

namespace proteus {

/** Interface for alignment implementations.
 *
 * This extends the `Analyzer` interface with a single method that
 * must provide a new updated geometry object. Any alignment implementation
 * should implement this interface so they can be easily interchanged.
 */
class Aligner : public Analyzer {
public:
  virtual Geometry updatedGeometry() const = 0;
};

} // namespace proteus
