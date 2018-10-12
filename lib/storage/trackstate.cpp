#include "trackstate.h"

#include <ostream>

Storage::TrackState::TrackState()
    : TrackState(Vector4::Constant(std::numeric_limits<double>::quiet_NaN()),
                 std::numeric_limits<float>::quiet_NaN())
{
}

Storage::TrackState::TrackState(
    float u, float v, float dU, float dV, float time)
    : TrackState(Vector4(u, v, dU, dV), time)
{
}

Storage::TrackState::TrackState(const Vector2& offset,
                                const Vector2& slope,
                                float time)
    : TrackState(Vector4(offset[0], offset[1], slope[0], slope[1]), time)
{
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "offset=[" << state.offset().transpose() << "]";
  os << " slope=[" << state.slope().transpose() << "]";
  os << " time=" << state.time();
  return os;
}
