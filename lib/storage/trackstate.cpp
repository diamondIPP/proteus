#include "trackstate.h"

#include <cassert>

Storage::TrackState::TrackState(float u, float v, float dU, float dV)
    : m_offset(u, v)
    , m_slope(dU, dV)
    , m_index(kInvalidIndex)
    , m_track(kInvalidIndex)
    , m_matchedCluster(kInvalidIndex)
{
}

Storage::TrackState::TrackState(const XYPoint& offset, const XYVector& slope)
    : TrackState(offset.x(), offset.y(), slope.x(), slope.y())
{
}

Storage::TrackState::TrackState() : TrackState(0, 0, 0, 0) {}

void Storage::TrackState::setCovU(float varOffset, float varSlope, float cov)
{
  m_cov(U, U) = varOffset;
  m_cov(U, V) = 0;
  m_cov(U, Du) = cov;
  m_cov(U, Dv) = 0;
  m_cov(Du, V) = 0;
  m_cov(Du, Du) = varSlope;
  m_cov(Du, Dv) = 0;
}

void Storage::TrackState::setCovV(float varOffset, float varSlope, float cov)
{
  m_cov(V, U) = 0;
  m_cov(V, V) = varOffset;
  m_cov(V, Du) = 0;
  m_cov(V, Dv) = cov;
  m_cov(Dv, U) = 0;
  m_cov(Dv, Du) = 0;
  m_cov(Dv, Dv) = varSlope;
}

void Storage::TrackState::setTrack(Index track)
{
  assert((m_track == kInvalidIndex) &&
         "track state can only belong to one track");
  m_track = track;
}

void Storage::TrackState::setMatchedCluster(Index cluster)
{
  assert((m_matchedCluster == kInvalidIndex) &&
         "track state can only be matched to one cluster");
  m_matchedCluster = cluster;
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "offset=" << state.offset() << " slope=" << state.slope();
  return os;
}
