#ifndef PT_RESIDUALS_H
#define PT_RESIDUALS_H

#include <vector>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Cluster;
class TrackState;
} // namespace Storage
namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics

namespace Analyzers {
namespace detail {

struct SensorResidualHists {
  TH1D* resU;
  TH1D* resV;
  TH2D* resUV;
  TH2D* trackUResU;
  TH2D* trackUResV;
  TH2D* trackVResU;
  TH2D* trackVResV;
  TH2D* slopeUResU;
  TH2D* slopeUResV;
  TH2D* slopeVResU;
  TH2D* slopeVResV;

  SensorResidualHists() = default;
  SensorResidualHists(TDirectory* dir,
                      const Mechanics::Sensor& sensor,
                      const double pixelRange,
                      const double slopeRange,
                      const int bins,
                      const std::string& name = std::string("residuals"));

  void fill(const Storage::TrackState& state, const Storage::Cluster& cluster);
};

} // namespace detail

class Residuals : public Analyzer {
public:
  /**
   * \param pixelRange Residual histogram range in number of pixels
   * \param slopeRange Track slope histogram range in radian
   * \param bins Number of histogram bins
   */
  Residuals(TDirectory* dir,
            const Mechanics::Device& device,
            /* Histogram options */
            const double pixelRange = 2.0,
            const double slopeRange = 0.001,
            const int bins = 128);

  std::string name() const;
  void analyze(const Storage::Event& refEvent);
  void finalize();

private:
  const Mechanics::Device& m_device;
  std::vector<detail::SensorResidualHists> m_hists;
};

} // namespace Analyzers

#endif // PT_RESIDUALS_H
