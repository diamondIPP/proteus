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
    , m_eventTime(NULL)
    , m_numTracksTime(NULL)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("EventInfo");

  auto h1 = [&](std::string name, int nbins, double x0, double y0) -> TH1D* {
    TH1D* h = new TH1D(name.c_str(), "", nbins, x0, y0);
    h->SetDirectory(plotDir);
    return h;
  };
  const int nBinsTrig = _device->readoutWindow();
  const int nBinsTrack = 10;
  const int nBinsTime = 1024;

  m_triggerOffset = h1("TriggerOffset", nBinsTrig, -0.5, nBinsTrig - 0.5);
  m_triggerOffset->SetXTitle("Trigger offset");
  m_triggerPhase = h1("TriggerPhase", nBinsTrig, -0.5, nBinsTrig - 0.5);
  m_triggerPhase->SetXTitle("Trigger phase");
  m_numTracks = h1("NumTracks", nBinsTrack, -0.5, nBinsTrack - 0.5);
  m_numTracks->SetXTitle("Tracks / event");

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

    m_eventTime = h1("Events_Time", nBinsTime, x0, x1);
    m_eventTime->SetXTitle("Time");
    m_eventTime->SetYTitle("Mean event rate / time");
    m_numTracksTime = h1("NumTracks_Time", nBinsTime, x0, x1);
    m_numTracksTime->SetXTitle("Time");
    m_numTracksTime->SetYTitle("Mean track rate / event");
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
  m_numTracks->Fill(event->numTracks());
  if (m_eventTime) {
    m_eventTime->Fill(_device->tsToTime(event->timeStamp()));
    m_numTracksTime->Fill(_device->tsToTime(event->timeStamp()),
                          event->numTracks());
  }
}

void Analyzers::EventInfo::postProcessing()
{
  if (_postProcessed)
    return;
  if (m_eventTime) {
    for (Int_t bin = 1; bin <= m_eventTime->GetNbinsX(); ++bin) {
      const double numEvents = m_eventTime->GetBinContent(bin);
      const double numTracks = m_numTracksTime->GetBinContent(bin);
      if (0 < numEvents) {
        m_numTracksTime->SetBinContent(bin, numTracks / numEvents);
      }
    }
  }
  _postProcessed = true;
}
