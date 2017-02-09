#ifndef PT_CORRELATIONALIGNER_H
#define PT_CORRELATIONALIGNER_H

#include <memory>
#include <vector>

#include "aligner.h"

class TDirectory;

namespace Analyzers {
class Correlations;
}
namespace Mechanics {
class Device;
}

namespace Alignment {

/** Align sensors in the xy-plane using only cluster correlations.
 *
 * This implictely assumes a straight track propagation with zero slope along
 * the z-axis.
 */
class CorrelationsAligner : public Aligner {
public:
  /**
   * \param device    The telescope device.
   * \param fixedId   Reference sensor that will be kept fixed.
   * \param alignIds  Sensors that should be aligned; must not contain fixedId.
   * \param dir       Histogram output directory.
   *
   * \warning This will add a `Correlations`-analyzer internally.
   */
  CorrelationsAligner(const Mechanics::Device& device,
                      const Index fixedId,
                      const std::vector<Index>& alignIds,
                      TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::Geometry updatedGeometry() const;

private:
  const Mechanics::Device& m_device;
  std::vector<Index> m_sensorIds;
  std::unique_ptr<Analyzers::Correlations> m_corr;
};

} // namespace Alignment

#endif /* end of include guard: PT_CORRELATIONALIGNER_H */
