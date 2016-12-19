#ifndef PT_RESIDUALSALIGNER_H
#define PT_RESIDUALSALIGNER_H

#include <vector>

#include "aligner.h"
#include "analyzers/residuals.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Alignment {

/** Align sensors in the xy-plane using track residuals. */
class ResidualsAligner : public Aligner {
public:
  /**
   * \param damping Scale factor for raw corrections to avoid oscillations
   * \param pixelRange Offset histogram range in number of pixels
   * \param gammaRange Rotation histogram range in radian
   * \param slopeRange Track slope histogram range in radian
   * \param bins Number of histogram bins
   */
  ResidualsAligner(const Mechanics::Device& device,
                   const std::vector<Index>& alignIds,
                   TDirectory* dir,
                   const double damping = 1,
                   const double pixelRange = 1.0,
                   const double gammaRange = 0.1,
                   const double slopeRange = 0.01,
                   const int bins = 128);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::Geometry updatedGeometry() const;

private:
  struct Histograms {
    Index sensorId;
    TH1D* corrU;
    TH1D* corrV;
    TH1D* corrGamma;
  };

  const Mechanics::Device& m_device;
  TH2D* m_trackSlope;
  std::vector<Histograms> m_hists;
  double m_damping;
};

} // namespace Alignment

#endif /* end of include guard: PT_RESIDUALSALIGNER_H */
