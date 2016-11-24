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

/** Align sensors in the xy-plane using track residuals. */
class ResidualsAligner : public Aligner {
public:
  ResidualsAligner(const Mechanics::Device& device,
                   const std::vector<Index>& alignIds,
                   TDirectory* dir,
                   double damping = 1);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::Alignment updatedGeometry() const;

private:
  struct Histograms {
    Index sensorId;
    TH1D* deltaU;
    TH1D* deltaV;
  };

  const Mechanics::Device& m_device;
  std::vector<Histograms> m_hists;
  TH2D* m_trackSlope;
  double m_damping;
};

} // namespace Alignment

#endif /* end of include guard: PT_RESIDUALSALIGNER_H */
