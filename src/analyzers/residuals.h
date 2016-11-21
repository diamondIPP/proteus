#ifndef PT_RESIDUALS_H
#define PT_RESIDUALS_H

#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "analyzers/analyzer.h"
#include "analyzers/singleanalyzer.h"
#include "utils/definitions.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

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
  struct Hists {
    TH2D* res;
    TH2D* trackUResU;
    TH2D* trackUResV;
    TH2D* trackVResU;
    TH2D* trackVResV;
    TH2D* slopeUResU;
    TH2D* slopeUResV;
    TH2D* slopeVResU;
    TH2D* slopeVResV;
  };

  const Mechanics::Device& m_device;
  std::vector<Hists> m_hists;
};

} // namespace Analyzers

#endif // PT_RESIDUALS_H
