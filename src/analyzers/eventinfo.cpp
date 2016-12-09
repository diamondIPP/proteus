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
    , m_eventsTime(NULL)
    , m_tracksTime(NULL)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("EventInfo");

  auto h1 = [&](std::string name, int nbins, double x0, double y0) -> TH1D* {
    TH1D* h = new TH1D(name.c_str(), "", nbins, x0, y0);
    h->SetDirectory(plotDir);
    return h;
  };
  const int nTrigMax = _device->readoutWindow();
  const int nTracksMax = 10;
  const int nHitsMax = 32;
  const int nBinsTime = 1024;

  m_triggerOffset = h1("TriggerOffset", nTrigMax, -0.5, nTrigMax - 0.5);
  m_triggerOffset->SetXTitle("Trigger offset");
  m_triggerPhase = h1("TriggerPhase", nTrigMax, -0.5, nTrigMax - 0.5);
  m_triggerPhase->SetXTitle("Trigger phase");
  m_tracks = h1("Tracks", nTracksMax, -0.5, nTracksMax - 0.5);
  m_tracks->SetXTitle("Tracks / event");

  // device must know the timestamp to real time conversion
  if (_device->timeStampStart() < _device->timeStampEnd()) {
    DEBUG("timestamp range: ", _device->timeStampStart(), " -> ",
          _device->timeStampEnd());
    DEBUG("time range: ", _device->tsToTime(_device->timeStampStart()), " -> ",
          _device->tsToTime(_device->timeStampEnd()));

    // avoid missing events from round-off errors in binning
    const uint64_t start = _device->timeStampStart();
    const uint64_t dur = _device->timeStampEnd() - start + 1;
    // integer division rounds towards zero. +1 increase fits all timestamps
    const uint64_t timeStampsPerBin = (dur / nBinsTime) + 1;
    const double x0 = _device->tsToTime(start);
    const double x1 = _device->tsToTime(start + nBinsTime * timeStampsPerBin);

    m_eventsTime = h1("Events_Time", nBinsTime, x0, x1);
    m_eventsTime->SetXTitle("Time");
    m_eventsTime->SetYTitle("Mean event rate / time");
    m_tracksTime = h1("Tracks_Time", nBinsTime, x0, x1);
    m_tracksTime->SetXTitle("Time");
    m_tracksTime->SetYTitle("Mean track rate / event");
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
  if (m_eventsTime) {
    m_eventsTime->Fill(_device->tsToTime(event->timeStamp()));
    m_tracksTime->Fill(_device->tsToTime(event->timeStamp()),
                       event->numTracks());
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
  if (m_eventsTime) {
    for (Int_t bin = 1; bin <= m_eventsTime->GetNbinsX(); ++bin) {
      const double eventsInBin = m_eventsTime->GetBinContent(bin);
      const double tracksInBin = m_tracksTime->GetBinContent(bin);
      if (0 < eventsInBin) {
        m_tracksTime->SetBinContent(bin, tracksInBin / eventsInBin);
      }
    }
  }
  _postProcessed = true;
}
