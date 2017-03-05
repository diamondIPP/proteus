#include "eventinfo.h"

#include <cassert>
#include <cmath>
#include <cstdint>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(EventInfo)

Analyzers::EventInfo::EventInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const int hitsMax,
                                const int tracksMax,
                                const int binsTimestamps)
    : m_timestampEvents(nullptr), m_timestampTracks(nullptr)
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  const int triggerMax = device->readoutWindow();
  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeDir(dir, "EventInfo");

  HistAxis axTrigOff(0, triggerMax, "Trigger offset");
  HistAxis axTrigPhase(0, triggerMax, "Trigger phase");
  HistAxis axHits(0, hitsMax, "Hits / event");
  HistAxis axClusters(0, hitsMax, "Clusters / event");
  HistAxis axTracks(0, tracksMax, "Tracks / event");

  m_triggerOffset = makeH1(sub, "TriggerOffset", axTrigOff);
  m_triggerPhase = makeH1(sub, "TriggerPhase", axTrigPhase);
  m_tracks = makeH1(sub, "Tracks", axTracks);

  // can only plot timestamp distribution if range is known
  if (device->timestampStart() < device->timestampEnd()) {
    DEBUG("timestamp range: ", device->timestampStart(), " -> ",
          device->timestampEnd());

    // avoid missing events from round-off errors in binning
    // integer division rounds towards zero. +1 increase fits all timestamps
    const uint64_t dur = device->timestampEnd() - device->timestampStart() + 1;
    const uint64_t timestampsPerBin = (dur / binsTimestamps) + 1;
    const uint64_t ts0 = device->timestampStart();
    const uint64_t ts1 = ts0 + binsTimestamps * timestampsPerBin;

    HistAxis axTimestamp(ts0, ts1, binsTimestamps, "Timestamp");

    m_timestampEvents = makeH1(sub, "Events_Timestamp", axTimestamp);
    m_timestampTracks = makeH1(sub, "Tracks_Timestamp", axTimestamp);
  }

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);

    SensorHists hs;
    hs.hits = makeH1(sub, sensor.name() + "-Hits", axHits);
    hs.clusters = makeH1(sub, sensor.name() + "-Clusters", axClusters);
    m_sensorHists.push_back(hs);
  }
}

std::string Analyzers::EventInfo::name() const { return "EventInfo"; }

void Analyzers::EventInfo::analyze(const Storage::Event& event)
{
  m_triggerOffset->Fill(event.triggerOffset());
  m_triggerPhase->Fill(event.triggerPhase());
  m_tracks->Fill(event.numTracks());
  if (m_timestampEvents) {
    m_timestampEvents->Fill(event.timestamp());
    m_timestampTracks->Fill(event.timestamp(), event.numTracks());
  }

  for (Index iplane = 0; iplane < event.numPlanes(); ++iplane) {
    const Storage::Plane& plane = *event.getPlane(iplane);
    const SensorHists& sensorHists = m_sensorHists[iplane];

    sensorHists.hits->Fill(plane.numHits());
    sensorHists.clusters->Fill(plane.numClusters());
  }
}

void Analyzers::EventInfo::finalize()
{
  if (m_timestampEvents) {
    for (Int_t bin = 1; bin <= m_timestampEvents->GetNbinsX(); ++bin) {
      const double eventsInBin = m_timestampEvents->GetBinContent(bin);
      const double tracksInBin = m_timestampTracks->GetBinContent(bin);
      if (0 < eventsInBin) {
        m_timestampTracks->SetBinContent(bin, tracksInBin / eventsInBin);
      }
    }
  }
}
