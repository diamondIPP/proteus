#include "trackstate.h"

#include <ostream>

#include "utils/logger.h"

Storage::TrackState::TrackState(Scalar location0,
                                Scalar location1,
                                Scalar slope0,
                                Scalar slope1)
    : TrackState()
{
  m_params[kLoc0] = location0;
  m_params[kLoc1] = location1;
  m_params[kSlopeLoc0] = slope0;
  m_params[kSlopeLoc1] = slope1;
}

Storage::TrackState::TrackState(const Vector2& location,
                                const Vector2& slope,
                                const SymMatrix2& locationCov,
                                const SymMatrix2& slopeCov)
    : TrackState(location[0], location[1], slope[0], slope[1])
{
  m_cov.block<2, 2>(kLoc0, kLoc0) = locationCov;
  m_cov.block<2, 2>(kSlopeLoc0, kSlopeLoc1) = slopeCov;
  m_cov.block<2, 2>(kLoc0, kSlopeLoc0).setZero();
  m_cov.block<2, 2>(kSlopeLoc0, kLoc0).setZero();
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "loc=" << format(state.location());
  os << " time=" << state.time();
  os << " slope=" << format(state.slope());
  os << " dtime=" << state.slopeTime();
  return os;
}
