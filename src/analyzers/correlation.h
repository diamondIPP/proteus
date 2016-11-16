#ifndef PT_CORRELATION_H
#define PT_CORRELATION_H

#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "analyzers/singleanalyzer.h"

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

  TH1D* getAlignmentPlotX(unsigned int nsensor);
  TH1D* getAlignmentPlotY(unsigned int nsensor);

private:
  std::vector<TH2D*> m_corrX; // Correlation in X
  std::vector<TH2D*> m_corrY;
  std::vector<TH1D*> m_diffX; // Alignment (correlation "residual") in X
  std::vector<TH1D*> m_diffY;

  // This analyzer uses two directories which need to be accessed by
  // initializeHist
  TDirectory* _corrDir;
  TDirectory* _alignDir;

  // Shared function to initialze any correlation hist between two sensors
  void initializeHist(const Mechanics::Sensor* sensor0,
                      const Mechanics::Sensor* sensor1);
};

} // namespace Analyzers

#endif // PT_CORRELATION_H
