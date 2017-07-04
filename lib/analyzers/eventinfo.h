#ifndef PT_EVENTINFO_H
#define PT_EVENTINFO_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

/** Overall event information, e.g. timing and hit and cluster rates. */
class EventInfo : public Analyzer {
public:
  EventInfo(const Mechanics::Device* device,
            TDirectory* dir,
            /* Histogram options */
            const int hitsMax = 32,
            const int tracksMax = 8,
            const int binsTimestamps = 1024);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct SensorHists {
    TH1D* hits;
    TH1D* clusters;
  };

  TH1D* m_triggerOffset;
  TH1D* m_triggerPhase;
  TH1D* m_tracks;
  TH1D* m_timestampEvents;
  TH1D* m_timestampTracks;
  std::vector<SensorHists> m_sensorHists;
};

} // namespace Analyzers

#endif // PT_EVENTINFO_H
