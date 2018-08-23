#ifndef PT_ALIGNER_H
#define PT_ALIGNER_H

#include "loop/analyzer.h"
#include "mechanics/geometry.h"
#include "utils/definitions.h"

namespace Alignment {

class Aligner : public Loop::Analyzer {
public:
  virtual std::string name() const = 0;
  virtual void execute(const Storage::Event& event) = 0;
  virtual void finalize() = 0;

  virtual Mechanics::Geometry updatedGeometry() const = 0;
};

} // namespace Alignment

#endif /* end of include guard: PT_ALIGNER_H */
