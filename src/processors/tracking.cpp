#include "tracking.h"

#include "mechanics/sensor.h"
#include "storage/cluster.h"

/** Linear weighted regression in one dimension.
 *
 * Straight from Numerical Recipes with offset = a and slope = b.
 */
struct LineFitter1D {
  double s, sx, sy, sxx, sxy;
  double d_inv;

  double offset() const { return (sxx * sy - sx * sxy) * d_inv; }
  double slope() const { return (s * sxy - sx * sy) * d_inv; }
  double varOffset() const { return sxx * d_inv; }
  double varSlope() const { return s * d_inv; }
  double cov() const { return -sx * d_inv; }

  LineFitter1D() : s(0), sx(0), sy(0), sxx(0), sxy(0), d_inv(0) {}
  void addPoint(double x, double y, double w = 1)
  {
    s += w;
    sx += w * x;
    sy += w * y;
    sxx += w * x * x;
    sxy += w * x * y;
  }
  void fit() { d_inv = 1 / (s * sxx - sx * sx); }
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

  void addPoint(const XYZPoint& pos, double wu = 1, double wv = 1)
  {
    u.addPoint(pos.z(), pos.x(), wu);
    v.addPoint(pos.z(), pos.y(), wv);
  }
  void addPoint(const XYZPoint& pos, const SymMatrix3& cov)
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

void Processors::fitTrack(Storage::Track& track)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    fit.addPoint(cluster.posGlobal(), cluster.covGlobal());
  }
  fit.fit();
  track.setGlobalState(fit.state());
  track.setGoodnessOfFit(straightChi2(track), 2 * (track.numClusters() - 2));
}

Storage::TrackState
Processors::fitTrackLocal(const Storage::Track& track,
                          const Mechanics::Sensor& reference)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster* cluster = track.getCluster(icluster);
    XYZPoint pos = reference.globalToLocal() * cluster->posGlobal();
    // TODO 2016-10-13 msmk: also transform error to local frame
    fit.addPoint(pos, cluster->covGlobal());
  }
  fit.fit();
  return fit.state();
}

Storage::TrackState
Processors::fitTrackLocalUnbiased(const Storage::Track& track,
                                  const Mechanics::Sensor& reference)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    if (cluster.sensorId() == reference.id())
      continue;
    XYZPoint pos = reference.globalToLocal() * cluster.posGlobal();
    // TODO 2016-10-13 msmk: also transform error to local frame
    fit.addPoint(pos, cluster.covGlobal());
  }
  fit.fit();
  return fit.state();
}
