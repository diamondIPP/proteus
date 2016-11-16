#ifndef PT_CORRELATION_H
#define PT_CORRELATION_H

#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "analyzers/singleanalyzer.h"
#include "utils/definitions.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Analyzers {

class Correlation : public SingleAnalyzer {
public:
  Correlation(const Mechanics::Device* device,
              TDirectory* dir,
              const char* suffix = "");

  void processEvent(const Storage::Event* event);
  void postProcessing();

  TH1D* getAlignmentPlotX(Index sensorId);
  TH1D* getAlignmentPlotY(Index sensorId);

private:
  // Shared function to initialize the correlation hist between two sensors
  void addHist(const Mechanics::Sensor& sensor0,
               const Mechanics::Sensor& sensor1,
               TDirectory* dir);

  struct CorrelationHists {
    Index id0, id1;
    TH2D* corrX;
    TH2D* corrY;
    TH1D* diffX;
    TH1D* diffY;
  };
  std::vector<CorrelationHists> m_hists;
};

} // namespace Analyzers

#endif // PT_CORRELATION_H
