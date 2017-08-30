#ifndef PT_RESIDUALSALIGNER_H
#define PT_RESIDUALSALIGNER_H

#include <vector>

#include "aligner.h"

class TDirectory;
class TH1D;

namespace Mechanics {
class Device;
}
namespace Alignment {

/** Sensor alignment in the local plane using track residual histograms. */
class ResidualsAligner : public Aligner {
public:
  /**
   * \param damping    Scale factor for raw corrections to avoid oscillations
   * \param pixelRange Offset histogram range in number of pixels
   * \param gammaRange Rotation histogram range in radian
   * \param bins       Number of histogram bins
   */
  ResidualsAligner(TDirectory* dir,
                   const Mechanics::Device& device,
                   const std::vector<Index>& alignIds,
                   const double damping = 1,
                   const double pixelRange = 1.0,
                   const double gammaRange = 0.1,
                   const int bins = 128);
  ~ResidualsAligner() = default;

  std::string name() const;
  void execute(const Storage::Event& event);
  void finalize();

  Mechanics::Geometry updatedGeometry() const;

private:
  struct SensorHists {
    Index sensorId;
    TH1D* corrU;
    TH1D* corrV;
    TH1D* corrGamma;
  };

  std::vector<SensorHists> m_hists;
  const Mechanics::Device& m_device;
  double m_damping;
};

} // namespace Alignment

#endif /* end of include guard: PT_RESIDUALSALIGNER_H */
