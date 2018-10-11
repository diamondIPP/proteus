#include "trackstate.h"

#include <cassert>
#include <limits>

Storage::TrackState::TrackState()
    : m_params(Vector4::Constant(std::numeric_limits<double>::quiet_NaN()))
    , m_cov(SymMatrix4::Zero())
    , m_matchedCluster(kInvalidIndex)
{
}

Storage::TrackState::TrackState(float u, float v, float dU, float dV)
    : m_params(u, v, dU, dV)
    , m_cov(SymMatrix4::Zero())
    , m_matchedCluster(kInvalidIndex)
{
}

Storage::TrackState::TrackState(const Vector2& offset, const Vector2& slope)
    : TrackState(offset[0], offset[1], slope[0], slope[1])
{
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "offset=[" << state.offset().transpose() << "]";
  os << " slope=[" << state.slope().transpose() << "]";
  return os;
}
