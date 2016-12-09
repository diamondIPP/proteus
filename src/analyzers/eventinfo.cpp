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

PT_SETUP_LOCAL_LOGGER(EventInfo)

Analyzers::EventInfo::EventInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const char* suffix,
                                unsigned int maxTracks)
    : SingleAnalyzer(device, dir, suffix, "EventInfo")
    , m_eventsTimestamp(NULL)
    , m_tracksTimestamps(NULL)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("EventInfo");

  auto h1 = [&](std::string name, int nbins, double x0, double y0) -> TH1D* {
    TH1D* h = new TH1D(name.c_str(), "", nbins, x0, y0);
    h->SetDirectory(plotDir);
    return h;
  };
  const int nTrigMax = device->readoutWindow();
  const int nTracksMax = 10;
  const int nHitsMax = 32;
  const int nBinsTimestamp = 1024;

  m_triggerOffset = h1("TriggerOffset", nTrigMax, -0.5, nTrigMax - 0.5);
  m_triggerOffset->SetXTitle("Trigger offset");
  m_triggerPhase = h1("TriggerPhase", nTrigMax, -0.5, nTrigMax - 0.5);
  m_triggerPhase->SetXTitle("Trigger phase");
  m_tracks = h1("Tracks", nTracksMax, -0.5, nTracksMax - 0.5);
  m_tracks->SetXTitle("Tracks / event");

  // device must know the timestamp to real time conversion
  if (device->timestampStart() < device->timestampEnd()) {
    DEBUG("timestamp range: ", device->timestampStart(), " -> ",
          device->timestampEnd());

    // avoid missing events from round-off errors in binning
    // integer division rounds towards zero. +1 increase fits all timestamps
    const uint64_t dur = device->timestampEnd() - device->timestampStart() + 1;
    const uint64_t timestampsPerBin = (dur / nBinsTimestamp) + 1;
    const uint64_t ts0 = device->timestampStart();
    const uint64_t ts1 = ts0 + nBinsTimestamp * timestampsPerBin;

    m_eventsTimestamp = h1("Events_Timestamp", nBinsTimestamp, ts0, ts1);
    m_eventsTimestamp->SetXTitle("Timestamp");
    m_eventsTimestamp->SetYTitle("Mean event rate / timestamp");
    m_tracksTimestamps = h1("Tracks_Timestamp", nBinsTimestamp, ts0, ts1);
    m_tracksTimestamps->SetXTitle("Timestamp");
    m_tracksTimestamps->SetYTitle("Mean track rate / event");
  }

  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device->getSensor(isensor);

    SensorHists hs;
    hs.hits = h1(sensor.name() + "-Hits", nHitsMax, -0.5, nHitsMax - 0.5);
    hs.hits->SetXTitle(("Hits on " + sensor.name() + " / event").c_str());
    hs.clusters =
        h1(sensor.name() + "-Clusters", nHitsMax, -0.5, nHitsMax - 0.5);
    hs.clusters->SetXTitle(
        ("Clusters on " + sensor.name() + " / event").c_str());
    m_sensorHists.push_back(hs);
  }
}

void Analyzers::EventInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);
  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  m_triggerOffset->Fill(event->triggerOffset());
  m_triggerPhase->Fill(event->triggerPhase());
  m_tracks->Fill(event->numTracks());
  if (m_eventsTimestamp) {
    m_eventsTimestamp->Fill(event->timestamp());
    m_tracksTimestamps->Fill(event->timestamp(), event->numTracks());
  }

  for (Index iplane = 0; iplane < event->numPlanes(); ++iplane) {
    const Storage::Plane& plane = *event->getPlane(iplane);
    const SensorHists& sensorHists = m_sensorHists[iplane];

    sensorHists.hits->Fill(plane.numHits());
    sensorHists.clusters->Fill(plane.numClusters());
  }
}

void Analyzers::EventInfo::postProcessing()
{
  if (_postProcessed)
    return;
  if (m_eventsTimestamp) {
    for (Int_t bin = 1; bin <= m_eventsTimestamp->GetNbinsX(); ++bin) {
      const double eventsInBin = m_eventsTimestamp->GetBinContent(bin);
      const double tracksInBin = m_tracksTimestamps->GetBinContent(bin);
      if (0 < eventsInBin) {
        m_tracksTimestamps->SetBinContent(bin, tracksInBin / eventsInBin);
      }
    }
  }
  _postProcessed = true;
}
