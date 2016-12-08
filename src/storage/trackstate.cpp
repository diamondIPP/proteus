#include "trackstate.h"

Storage::TrackState::TrackState(double u, double v, double dU, double dV)
    : m_offset(u, v), m_slope(dU, dV)
{
}

Storage::TrackState::TrackState(const XYPoint& offset, const XYVector& slope)
    : TrackState(offset.x(), offset.y(), slope.x(), slope.y())
{
}

Storage::TrackState::TrackState() : TrackState(0, 0, 0, 0) {}

void Storage::TrackState::setCovU(double varOffset, double varSlope, double cov)
{
  m_cov(U, U) = varOffset;
  m_cov(U, V) = 0;
  m_cov(U, Du) = cov;
  m_cov(U, Dv) = 0;
  m_cov(Du, V) = 0;
  m_cov(Du, Du) = varSlope;
  m_cov(Du, Dv) = 0;
}

void Storage::TrackState::setCovV(double varOffset, double varSlope, double cov)
{
  m_cov(V, U) = 0;
  m_cov(V, V) = varOffset;
  m_cov(V, Du) = 0;
  m_cov(V, Dv) = cov;
  m_cov(Dv, U) = 0;
  m_cov(Dv, Du) = 0;
  m_cov(Dv, Dv) = varSlope;
}

void Storage::TrackState::setErrU(double stdOffset, double stdSlope, double cov)
{
  setCovU(stdOffset * stdOffset, stdSlope * stdSlope, cov);
}

void Storage::TrackState::setErrV(double stdOffset, double stdSlope, double cov)
{
  setCovV(stdOffset * stdOffset, stdSlope * stdSlope, cov);
}

void Storage::TrackState::print(std::ostream& os,
                                const std::string& prefix) const
{
  os << prefix << "offset: " << offset() << '\n';
  os << prefix << "slope: " << slope() << '\n';
}

std::ostream& Storage::operator<<(std::ostream& os, const TrackState& state)
{
  os << "uv=" << state.offset() << " d(uv)=" << state.slope();
  return os;
}
