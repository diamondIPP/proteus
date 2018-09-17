#ifndef PT_ALIGNER_H
#define PT_ALIGNER_H

#include "loop/analyzer.h"
#include "mechanics/geometry.h"

namespace Alignment {

/** Interface for alignment implementations.
 *
 * This extends the `Loop::Analyzer` interface with a single method that
 * must provide a new updated geometry object. Any alignment implementation
 * should implement this interface so they can be easily interchanged.
 */
class Aligner : public Loop::Analyzer {
public:
  virtual Mechanics::Geometry updatedGeometry() const = 0;
};

} // namespace Alignment

#endif /* end of include guard: PT_ALIGNER_H */
