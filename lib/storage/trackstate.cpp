#include "trackstate.h"

#include <cassert>

Storage::TrackState::TrackState(float u, float v, float dU, float dV)
    : m_matchedCluster(kInvalidIndex)
{
  m_params[U] = u;
  m_params[V] = v;
  m_params[Du] = dU;
  m_params[Dv] = dV;
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

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "offset=" << state.offset() << " slope=" << state.slope();
  return os;
}
