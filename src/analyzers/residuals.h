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
                      double slopeMax = 0.01);

  void fill(const Storage::TrackState& state, const Storage::Cluster& cluster);
};

} // namespace detail

class Residuals : public SingleAnalyzer {
public:
  Residuals(
      const Mechanics::Device* refDevice,
      TDirectory* dir = 0,
      const char* suffix = "",
      /* Histogram options */
      unsigned int nPixX = 20,  // Number of pixels in the residual plots
      double binsPerPix = 10,   // Hist bins per pixel
      unsigned int binsY = 20); // Number of bins for the vertical in AB plots

  void processEvent(const Storage::Event* refEvent);
  void postProcessing();

  TH1D* getResidualX(unsigned int nsensor);
  TH1D* getResidualY(unsigned int nsensor);
  TH2D* getResidualXX(unsigned int nsensor);
  TH2D* getResidualXY(unsigned int nsensor);
  TH2D* getResidualYY(unsigned int nsensor);
  TH2D* getResidualYX(unsigned int nsensor);

private:
  std::vector<TH1D*> _residualsX;
  std::vector<TH1D*> _residualsY;
  std::vector<TH2D*> _residualsXX;
  std::vector<TH2D*> _residualsXY;
  std::vector<TH2D*> _residualsYY;
  std::vector<TH2D*> _residualsYX;
};

class UnbiasedResiduals : public Analyzer {
public:
  UnbiasedResiduals(const Mechanics::Device& device, TDirectory* dir);

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
