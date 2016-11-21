#ifndef PT_RESIDUALSALIGNER_H
#define PT_RESIDUALSALIGNER_H

#include <vector>

#include "aligner.h"
#include "analyzers/residuals.h"

class TDirectory;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Alignment {

/** Align sensors in the xy-plane and z-rotations using track residuals.
 *
 * \warning Instantiates Analyzers::UnbiasedResiduals internally. This Analyzer
 *          should therefore not be manually added to the event loop.
 */
class ResidualsAligner : public Aligner {
public:
  ResidualsAligner(const Mechanics::Device& device,
                   const std::vector<Index>& alignIds,
                   TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::Alignment updatedGeometry() const;

private:
  const Mechanics::Device& m_device;
  std::vector<Index> m_alignIds;
  Analyzers::UnbiasedResiduals m_res;
  TH2D* m_trackSlope;
};

} // namespace Alignment

#endif /* end of include guard: PT_RESIDUALSALIGNER_H */
