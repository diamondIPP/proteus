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
  TH1D* _triggerOffset;
  TH1D* _trackInTime;
  TH1D* _numTracks;
  TH1D* _eventsVsTime;
  TH1D* _tracksVsTime;
  TH1D* _clustersVsTime;
};

} // namespace Analyzers

#endif // PT_EVENTINFO_H
