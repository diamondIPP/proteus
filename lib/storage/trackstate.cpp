#include "trackstate.h"

#include <cassert>
#include <limits>

Storage::TrackState::TrackState()
    : TrackState(std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::quiet_NaN())
{
}

Storage::TrackState::TrackState(const Vector2& offset, const Vector3& direction)
    : TrackState(offset[0],
                 offset[1],
                 direction[0] / direction[2],
                 direction[1] / direction[2])
{
}

Storage::TrackState::TrackState(float u, float v, float dU, float dV)
    : m_matchedCluster(kInvalidIndex)
{
  m_params[U] = u;
  m_params[V] = v;
  m_params[Du] = dU;
  m_params[Dv] = dV;
}

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

void Storage::TrackState::setCovOffset(const SymMatrix2& covOffset)
{
  m_cov(U, U) = covOffset(0, 0);
  m_cov(V, V) = covOffset(1, 1);
  m_cov(U, V) = m_cov(V, U) = covOffset(0, 1);
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  auto o = state.offset();
  auto s = state.slope();
  os << "offset=[" << o[0] << "," << o[1] << "]";
  os << " slope=[" << s[0] << "," << s[1] << "]";
  return os;
}
