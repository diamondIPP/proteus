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

Storage::TrackState::TrackState(const Vector4& position,
                                const SymMatrix4& positionCov,
                                const Vector2& slope,
                                const SymMatrix2& slopeCov)
    : TrackState(Vector6::Zero(), SymMatrix6::Zero())
{
  m_params[kLoc0] = position[kU];
  m_params[kLoc1] = position[kV];
  m_params[kTime] = position[kS];
  m_params[kSlopeLoc0] = slope[0];
  m_params[kSlopeLoc1] = slope[1];
  m_cov(kLoc0, kLoc0) = positionCov(kU, kU);
  m_cov(kLoc1, kLoc0) = m_cov(kLoc0, kLoc1) = positionCov(kV, kU);
  m_cov(kTime, kLoc0) = m_cov(kLoc0, kTime) = positionCov(kS, kU);
  m_cov(kLoc1, kLoc1) = positionCov(kV, kV);
  m_cov(kTime, kLoc1) = m_cov(kLoc1, kTime) = positionCov(kS, kV);
  m_cov(kTime, kTime) = positionCov(kS, kS);
  m_cov(kSlopeLoc0, kSlopeLoc0) = slopeCov(0, 0);
  m_cov(kSlopeLoc1, kSlopeLoc0) = m_cov(kSlopeLoc0, kSlopeLoc1) = slopeCov(1, 0);
  m_cov(kSlopeLoc1, kSlopeLoc1) = slopeCov(1, 1);
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "loc=" << format(state.location());
  os << " time=" << state.time();
  os << " slope=" << format(state.slope());
  os << " dtime=" << state.slopeTime();
  return os;
}
