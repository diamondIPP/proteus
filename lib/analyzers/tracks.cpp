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

namespace proteus {

Tracks::Tracks(TDirectory* dir,
               const Device& device,
               const int numTracksMax,
               const double reducedChi2Max,
               const double slopeRangeStd,
               const int bins)
{
  auto box = device.boundingBox();
  auto pitch = device.minimumPitch();
  Vector2 slope = device.geometry().beamSlope();
  Vector2 slopeStdev = extractStdev(device.geometry().beamSlopeCovariance());
  // ensure sensible histogram limits even for vanishing divergence
  slopeStdev =
      (slopeStdev.array() <= 0).select(Vector2::Constant(1.25e-3), slopeStdev);
  Vector2 slopeMin = slope - slopeRangeStd * slopeStdev;
  Vector2 slopeMax = slope + slopeRangeStd * slopeStdev;

  auto axNTracks = HistAxis::Integer(0, numTracksMax, "Tracks / event");
  auto axSize =
      HistAxis::Integer(0, device.numSensors() + 1, "Clusters on track");
  HistAxis axChi2(0, reducedChi2Max, bins, "#chi^2 / degrees of freedom");
  HistAxis axPosX(box.interval(kX), box.length(kX) / pitch[kX],
                  "Track position x");
  HistAxis axPosY(box.interval(kY), box.length(kY) / pitch[kY],
                  "Track position y");
  HistAxis axTime(box.interval(kT), box.length(kT) / pitch[kT],
                  "Track global time");
  HistAxis axSlopeX(slopeMin[0], slopeMax[0], bins, "Track slope x");
  HistAxis axSlopeY(slopeMin[1], slopeMax[1], bins, "Track slope y");

  TDirectory* sub = makeDir(dir, "tracks");
  m_nTracks = makeH1(sub, "ntracks", axNTracks);
  m_size = makeH1(sub, "size", axSize);
  m_reducedChi2 = makeH1(sub, "reduced_chi2", axChi2);
  m_prob = makeH1(sub, "probability", {0.0, 1.0, bins, "Track probability"});
  m_posX = makeH1(sub, "position_x", axPosX);
  m_posY = makeH1(sub, "position_y", axPosY);
  m_posXY = makeH2(sub, "position_xy", axPosX, axPosY);
  m_time = makeH1(sub, "time", axTime);
  m_slopeX = makeH1(sub, "slope_x", axSlopeX);
  m_slopeY = makeH1(sub, "slope_y", axSlopeY);
  m_slopeXY = makeH2(sub, "slope_xy", axSlopeX, axSlopeY);
}

std::string Tracks::name() const { return "Tracks"; }

void Tracks::execute(const Event& event)
{
  m_nTracks->Fill(event.numTracks());

  for (Index itrack = 0; itrack < event.numTracks(); itrack++) {
    const Track& track = event.getTrack(itrack);
    const TrackState& state = track.globalState();

    m_size->Fill(track.size());
    m_reducedChi2->Fill(track.reducedChi2());
    m_prob->Fill(track.probability());
    m_posX->Fill(state.loc0());
    m_posY->Fill(state.loc1());
    m_posXY->Fill(state.loc0(), state.loc1());
    m_time->Fill(state.time());
    m_slopeX->Fill(state.slopeLoc0());
    m_slopeY->Fill(state.slopeLoc1());
    m_slopeXY->Fill(state.slopeLoc0(), state.slopeLoc1());
  }
}

double Tracks::avgNumTracks() const { return m_nTracks->GetMean(); }

Vector2 Tracks::beamSlope() const
{
  return {m_slopeXY->GetMean(1), m_slopeXY->GetMean(2)};
}

Vector2 Tracks::beamDivergence() const
{
  return {m_slopeXY->GetStdDev(1), m_slopeXY->GetStdDev(2)};
}

} // namespace proteus
