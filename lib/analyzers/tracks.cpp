#include "tracks.h"

#include <cassert>
#include <cmath>
#include <sstream>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Tracks::Tracks(TDirectory* dir,
                          const Mechanics::Device& device,
                          const int numTracksMax,
                          const double reducedChi2Max,
                          const double slopeRangeStd,
                          const int bins)
{
  using namespace Utils;

  // estimate bounding box of all sensor projections into the xy-plane
  auto active = device.getSensor(0)->projectedEnvelope();
  for (auto isensor : device.sensorIds())
    active = Utils::boundingBox(active,
                                device.getSensor(isensor)->projectedEnvelope());

  Vector3 direction = device.geometry().beamDirection();
  Vector2 slope(direction[0] / direction[2], direction[1] / direction[2]);
  Vector2 slopeMin = slope - slopeRangeStd * device.geometry().beamDivergence();
  Vector2 slopeMax = slope + slopeRangeStd * device.geometry().beamDivergence();

  HistAxis axNTracks(0, numTracksMax, "Tracks / event");
  HistAxis axSize(0, device.numSensors() + 1, "Clusters on track");
  HistAxis axChi2(0, reducedChi2Max, bins, "#chi^2 / degrees of freedom");
  HistAxis axOffX(active.interval(0), bins, "Track offset x");
  HistAxis axOffY(active.interval(1), bins, "Track offset y");
  HistAxis axSlopeX(slopeMin[0], slopeMax[0], bins, "Track slope x");
  HistAxis axSlopeY(slopeMin[1], slopeMax[1], bins, "Track slope y");

  TDirectory* sub = makeDir(dir, "tracks");
  m_nTracks = makeH1(sub, "ntracks", axNTracks);
  m_size = makeH1(sub, "size", axSize);
  m_reducedChi2 = makeH1(sub, "reduced_chi2", axChi2);
  m_offsetX = makeH1(sub, "offset_x", axOffX);
  m_offsetY = makeH1(sub, "offset_y", axOffY);
  m_offsetXY = makeH2(sub, "offset_xy", axOffX, axOffY);
  m_slopeX = makeH1(sub, "slope_x", axSlopeX);
  m_slopeY = makeH1(sub, "slope_y", axSlopeY);
  m_slopeXY = makeH2(sub, "slope_xy", axSlopeX, axSlopeY);
}

std::string Analyzers::Tracks::name() const { return "Tracks"; }

void Analyzers::Tracks::analyze(const Storage::Event& event)
{
  m_nTracks->Fill(event.numTracks());

  for (Index itrack = 0; itrack < event.numTracks(); itrack++) {
    const Storage::Track& track = event.getTrack(itrack);
    const Storage::TrackState& state = track.globalState();

    m_size->Fill(track.size());
    m_reducedChi2->Fill(track.reducedChi2());
    m_offsetXY->Fill(state.offset().x(), state.offset().y());
    m_offsetX->Fill(state.offset().x());
    m_offsetY->Fill(state.offset().y());
    m_slopeXY->Fill(state.slope().x(), state.slope().y());
    m_slopeX->Fill(state.slope().x());
    m_slopeY->Fill(state.slope().y());
  }
}

void Analyzers::Tracks::finalize() {}