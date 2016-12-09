#include "trackinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"

Analyzers::TrackInfo::TrackInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const char* suffix,
                                const double reducedChi2Max,
                                const double slopeMax)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "TrackInfo")
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("TrackInfo");

  typedef Mechanics::Sensor::Area Area;
  typedef Area::Axis Interval;

  // fixed histogram parameters
  const int numClustersMax = device->numSensors();
  const int bins1D = 256;
  const int bins2D = 128;
  const Area arSlope(Interval(-slopeMax, slopeMax),
                     Interval(-slopeMax, slopeMax));
  // estimate bounding box of all sensor projections into the xy-plane
  Area arOffset(Interval(0, 0), Interval(0, 0));
  for (Index isensor = 0; isensor < device->numSensors(); ++isensor) {
    arOffset = Utils::bounding(
        arOffset, device->getSensor(isensor)->projectedEnvelopeXY());
  }

  auto h1 = [&](std::string name, Interval x, int nx = -1) -> TH1D* {
    if (nx < 0)
      nx = bins1D;
    TH1D* h = new TH1D(name.c_str(), "", nx, x.min, x.max);
    h->SetDirectory(plotDir);
    return h;
  };
  auto h2 = [&](std::string name, Area xy) -> TH2D* {
    TH2D* h = new TH2D(name.c_str(), "", bins2D, xy.axes[0].min, xy.axes[0].max,
                       bins2D, xy.axes[1].min, xy.axes[1].max);
    h->SetDirectory(plotDir);
    return h;
  };
  m_numClusters =
      h1("NumClusters", Interval(-0.5, numClustersMax - 0.5), numClustersMax);
  m_reducedChi2 = h1("ReducedChi2", Interval(0, reducedChi2Max));
  m_offsetXY = h2("OffsetXY", arOffset);
  m_offsetX = h1("OffsetX", arOffset.axes[0]);
  m_offsetY = h1("OffsetY", arOffset.axes[1]);
  m_slopeXY = h2("SlopeXY", arSlope);
  m_slopeX = h1("SlopeX", arSlope.axes[0]);
  m_slopeY = h1("SlopeY", arSlope.axes[1]);
}

void Analyzers::TrackInfo::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for (Index itrack = 0; itrack < event->numTracks(); itrack++) {
    const Storage::Track& track = *event->getTrack(itrack);
    const Storage::TrackState& state = track.globalState();

    // Check if the track passes the cuts
    if (!checkCuts(&track))
      continue;

    m_numClusters->Fill(track.numClusters());
    m_reducedChi2->Fill(track.reducedChi2());
    m_offsetXY->Fill(state.offset().x(), state.offset().y());
    m_offsetX->Fill(state.offset().x());
    m_offsetY->Fill(state.offset().y());
    m_slopeXY->Fill(state.slope().x(), state.slope().y());
    m_slopeX->Fill(state.slope().x());
    m_slopeY->Fill(state.slope().y());
  }
}

void Analyzers::TrackInfo::postProcessing() {}
