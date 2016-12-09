#ifndef PT_EVENTINFO_H
#define PT_EVENTINFO_H

#include "singleanalyzer.h"

class TDirectory;
class TH1D;

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class EventInfo : public SingleAnalyzer {
public:
  EventInfo(const Mechanics::Device* device,
            TDirectory* dir,
            const char* suffix = "",
            /* Histogram options */
            unsigned int maxTracks = 10);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  TH1D* m_triggerOffset;
  TH1D* m_triggerPhase;
  TH1D* m_numTracks;
  TH1D* m_eventTime;
  TH1D* m_numTracksTime;
};

} // namespace Analyzers

#endif // PT_EVENTINFO_H
