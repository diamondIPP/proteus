#include "tracking.h"

#include "mechanics/geometry.h"
#include "storage/cluster.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Tracking)

/** Linear weighted regression in one dimension.
 *
 * Straight from Numerical Recipes with offset = a and slope = b.
 */
struct LineFitter1D {
  double s, sx, sy, sxx, sxy, syy; // weighted sums of inputs
  double cxx;                      // (unscaled) input variance

  LineFitter1D() : s(0), sx(0), sy(0), sxx(0), sxy(0), syy(0), cxx(0) {}

  void addPoint(double x, double y, double w = 1)
  {
    s += w;
    sx += w * x;
    sy += w * y;
    sxx += w * x * x;
    sxy += w * x * y;
    syy += w * y * y;
  }
  void fit() { cxx = (s * sxx - sx * sx); }

  double offset() const { return (sy * sxx - sx * sxy) / cxx; }
  double slope() const { return (s * sxy - sx * sy) / cxx; }
  double varOffset() const { return sxx / cxx; }
  double varSlope() const { return s / cxx; }
  double cov() const { return -sx / cxx; }
  double chi2() const
  {
    return syy + (sxy * (2 * sx * sy - s * sxy) - sxx * sy * sy) / cxx;
  }
};

/** Fit a 3D straight line assuming a propagation along the third dimension. */
struct SimpleStraightFitter {
  LineFitter1D u, v;

  XYPoint offset() const { return XYPoint(u.offset(), v.offset()); }
  XYVector slope() const { return XYVector(u.slope(), v.slope()); }
  Storage::TrackState state() const
  {
    Storage::TrackState s(u.offset(), v.offset(), u.slope(), v.slope());
    s.setCovU(u.varOffset(), u.varSlope(), u.cov());
    s.setCovV(v.varOffset(), v.varSlope(), v.cov());
    return s;
  }
  double chi2() const { return u.chi2() + v.chi2(); }

  void addPoint(const XYZPoint& pos, double wu = 1, double wv = 1)
  {
    u.addPoint(pos.z(), pos.x(), wu);
    v.addPoint(pos.z(), pos.y(), wv);
  }
  void addPoint(const XYZPoint& pos, const SymMatrix3& cov)
  {
    addPoint(pos, 1 / cov(0, 0), 1 / cov(1, 1));
  }
  void addPoint(const XYZPoint& pos, const SymMatrix2& cov)
  {
    addPoint(pos, 1 / cov(0, 0), 1 / cov(1, 1));
  }
  void fit()
  {
    u.fit();
    v.fit();
  }
};

// Calculate chi2 value for a simple straight track fit.
static inline double straightChi2(Storage::Track& track)
{
  const Storage::TrackState& state = track.globalState();

  double chi2 = 0;
  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    // xy residual at the z-position of the cluster
    XYPoint trk(state.offset() + state.slope() * cluster.posGlobal().z());
    Vector2 res(cluster.posGlobal().x() - trk.x(),
                cluster.posGlobal().y() - trk.y());
    // z-covariance is ignored in simple straight fit anyways
    chi2 += mahalanobisSquared(cluster.covGlobal().Sub<SymMatrix2>(0, 0), res);
  }
  return chi2;
}

static inline XYZPoint refPosition(const XYPoint& pos,
                                   const Transform3D& localToGlobal,
                                   const Transform3D& globalToReference)
{
  return globalToReference * localToGlobal * XYZPoint(pos.x(), pos.y(), 0);
}

static inline SymMatrix2 refCovariance(const SymMatrix2& cov,
                                       const Transform3D& localToGlobal,
                                       const Transform3D& globalToReference)
{
  // ROOT GenVector does not want to play with SMatrix
  Rotation3D tmp = globalToReference.Rotation() * localToGlobal.Rotation();
  Matrix3 jac;
  tmp.GetRotationMatrix(jac);
  return Similarity(jac.Sub<Matrix2>(0, 0), cov);
}

void Tracking::fitTrackGlobal(Storage::Track& track)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    fit.addPoint(cluster.posGlobal(), cluster.covGlobal());
  }
  fit.fit();
  track.setGlobalState(fit.state());
  track.setGoodnessOfFit(fit.chi2(), 2 * (track.numClusters() - 2));
}

Storage::TrackState Tracking::fitTrackLocal(const Storage::Track& track,
                                            const Mechanics::Geometry& geo,
                                            Index referenceId)
{
  SimpleStraightFitter fit;
  auto globalToRef = geo.getLocalToGlobal(referenceId).Inverse();

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    auto localToGlobal = geo.getLocalToGlobal(cluster.sensorId());
    auto pos = refPosition(cluster.posLocal(), localToGlobal, globalToRef);
    auto cov = refCovariance(cluster.covLocal(), localToGlobal, globalToRef);
    fit.addPoint(pos, cov);
  }
  fit.fit();
  return fit.state();
}

Storage::TrackState
Tracking::fitTrackLocalUnbiased(const Storage::Track& track,
                                const Mechanics::Geometry& geo,
                                Index referenceId)
{
  SimpleStraightFitter fit;
  auto globalToRef = geo.getLocalToGlobal(referenceId).Inverse();

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    if (cluster.sensorId() == referenceId)
      continue;
    auto localToGlobal = geo.getLocalToGlobal(cluster.sensorId());
    auto pos = refPosition(cluster.posLocal(), localToGlobal, globalToRef);
    auto cov = refCovariance(cluster.covLocal(), localToGlobal, globalToRef);
    fit.addPoint(pos, cov);
  }
  fit.fit();
  return fit.state();
}
