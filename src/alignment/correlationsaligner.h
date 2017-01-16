#ifndef PT_CORRELATIONALIGNER_H
#define PT_CORRELATIONALIGNER_H

#include <memory>
#include <vector>

#include "aligner.h"

namespace Analyzers {
class Correlation;
}
namespace Mechanics {
class Device;
}

namespace Alignment {

/** Align sensors in the xy-plane using cluster correlations.
 *
 * This assumes a straight track propagation without a slope along the z-axis.
 */
class CorrelationsAligner : public Aligner {
public:
  CorrelationsAligner(const Mechanics::Device& device,
                      const std::vector<Index>& alignIds,
                      std::shared_ptr<const Analyzers::Correlation> corr);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::Geometry updatedGeometry() const;

private:
  const Mechanics::Device& m_device;
  std::vector<Index> m_alignIds;
  std::shared_ptr<const Analyzers::Correlation> m_corr;
};

} // namespace Alignment

#endif /* end of include guard: PT_CORRELATIONALIGNER_H */
