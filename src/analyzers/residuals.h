#ifndef PT_RESIDUALS_H
#define PT_RESIDUALS_H

#include <vector>

#include "analyzers/analyzer.h"
#include "analyzers/singleanalyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Cluster;
class TrackState;
}
namespace Mechanics {
class Device;
class Sensor;
}

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
                      const int bins);

  void fill(const Storage::TrackState& state, const Storage::Cluster& cluster);
};

} // namespace detail

class Residuals : public SingleAnalyzer {
public:
  /**
   * \param pixelRange Residual histogram range in number of pixels
   * \param slopeRange Track slope histogram range in radian
   * \param bins Number of histogram bins
   */
  Residuals(
      const Mechanics::Device* device,
      TDirectory* dir = 0,
      const char* suffix = "",
      /* Histogram options */
      const double pixelRange = 2.0,
      const double slopeRange = 0.001,
      const int bins = 128); // Number of bins for the vertical in AB plots

  void processEvent(const Storage::Event* refEvent);
  void postProcessing();

  TH1D* getResidualX(Index sensorId);
  TH1D* getResidualY(Index sensorId);
  TH2D* getResidualXX(Index sensorId);
  TH2D* getResidualXY(Index sensorId);
  TH2D* getResidualYY(Index sensorId);
  TH2D* getResidualYX(Index sensorId);

private:
  std::vector<detail::SensorResidualHists> m_hists;
};

class UnbiasedResiduals : public Analyzer {
public:
  /**
   * \param pixelRange Residual histogram range in number of pixels
   * \param slopeRange Track slope histogram range in radian
   * \param bins Number of histogram bins
   */
  UnbiasedResiduals(const Mechanics::Device& device,
                    TDirectory* dir,
                    const double pixelRange = 2.0,
                    const double slopeRange = 0.001,
                    const int bins = 128);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  const TH2D* getResidualUV(Index sensorId) const;

private:
  const Mechanics::Device& m_device;
  std::vector<detail::SensorResidualHists> m_hists;
};

} // namespace Analyzers

#endif // PT_RESIDUALS_H
