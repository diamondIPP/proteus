#ifndef PT_ALIGNER_H
#define PT_ALIGNER_H

#include "analyzers/analyzer.h"
#include "mechanics/alignment.h"
#include "utils/definitions.h"

namespace Alignment {

class Aligner : public Analyzers::Analyzer {
public:
  virtual std::string name() const = 0;
  virtual void analyze(const Storage::Event& event) = 0;
  virtual void finalize() = 0;

  virtual Mechanics::Alignment updatedGeometry() const = 0;
};

} // namespace Alignment

#endif /* end of include guard: PT_ALIGNER_H */
