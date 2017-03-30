#include "trackinfo.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::TrackInfo::TrackInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const double reducedChi2Max,
                                const double slopeMax,
                                const int bins)
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* sub = makeDir(dir, "TrackInfo");

  // estimate bounding box of all sensor projections into the xy-plane
  auto active = device->getSensor(0)->projectedEnvelopeXY();
  for (Index isensor = 1; isensor < device->numSensors(); ++isensor) {
    active = Utils::boundingBox(
        active, device->getSensor(isensor)->projectedEnvelopeXY());
  }

  HistAxis axNClusters(0, device->numSensors(), "Clusters on track");
  HistAxis axChi2(0, reducedChi2Max, bins, "#chi^2 / degrees of freedom");
  HistAxis axOffX(active.interval(0), bins, "Track offset x");
  HistAxis axOffY(active.interval(1), bins, "Track offset y");
  HistAxis axSlopeX(-slopeMax, slopeMax, bins, "Track slope x");
  HistAxis axSlopeY(-slopeMax, slopeMax, bins, "Track slope y");

  m_numClusters = makeH1(sub, "NumClusters", axNClusters);
  m_reducedChi2 = makeH1(sub, "ReducedChi2", axChi2);
  m_offsetXY = makeH2(sub, "OffsetXY", axOffX, axOffY);
  m_offsetX = makeH1(sub, "OffsetX", axOffX);
  m_offsetY = makeH1(sub, "OffsetY", axOffY);
  m_slopeXY = makeH2(sub, "SlopeXY", axSlopeX, axSlopeY);
  m_slopeX = makeH1(sub, "SlopeX", axSlopeX);
  m_slopeY = makeH1(sub, "SlopeY", axSlopeY);
}

std::string Analyzers::TrackInfo::name() const { return "TrackInfo"; }

void Analyzers::TrackInfo::analyze(const Storage::Event& event)
{
  for (Index itrack = 0; itrack < event.numTracks(); itrack++) {
    const Storage::Track& track = *event.getTrack(itrack);
    const Storage::TrackState& state = track.globalState();

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

void Analyzers::TrackInfo::finalize() {}
