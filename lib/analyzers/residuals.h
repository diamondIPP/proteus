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
                      const double rangeStd,
                      const int bins,
                      const std::string& name);

  void fill(const Storage::TrackState& state, const Storage::Cluster& cluster);
};

} // namespace detail

class Residuals : public Analyzer {
public:
  /** Construct a residual analyzer.
   *
   * \param dir       Where to create the output subdirectory
   * \param device    The device object
   * \param sensorIds Sensors for which residuals should be calculated
   * \param subdir    Name of the output subdirectory
   * \param rangeStd  Residual/ slope range in expected standard deviations
   * \param bins      Number of histogram bins
   */
  Residuals(TDirectory* dir,
            const Mechanics::Device& device,
            const std::vector<Index>& sensorIds,
            const std::string& subdir = std::string("residuals"),
            /* Histogram options */
            const double rangeStd = 5.0,
            const int bins = 128);

  std::string name() const;
  void analyze(const Storage::Event& refEvent);
  void finalize();

private:
  std::vector<detail::SensorResidualHists> m_hists;
};

} // namespace Analyzers

#endif // PT_RESIDUALS_H
