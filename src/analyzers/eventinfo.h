#ifndef PT_EVENTINFO_H
#define PT_EVENTINFO_H

#include <vector>

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

/** Overall event information, e.g. timing and hit and cluster rates. */
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
  struct SensorHists {
    TH1D* hits;
    TH1D* clusters;
  };

  TH1D* m_triggerOffset;
  TH1D* m_triggerPhase;
  TH1D* m_tracks;
  TH1D* m_eventsTimestamp;
  TH1D* m_tracksTimestamps;
  std::vector<SensorHists> m_sensorHists;
};

} // namespace Analyzers

#endif // PT_EVENTINFO_H
