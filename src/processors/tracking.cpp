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
  double offsetVar() const { return sxx * d_inv; }
  double slope() const { return (s * sxy - sx * sy) * d_inv; }
  double slopeVar() const { return s * d_inv; }
  double cov() const { return -sx * d_inv; }

  LineFitter1D()
      : s(0)
      , sx(0)
      , sy(0)
      , sxx(0)
      , sxy(0)
      , d_inv(0)
  {
  }
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

struct SimpleStraightFitter {
  LineFitter1D u, v;

  XYPoint offset() const { return XYPoint(u.offset(), v.offset()); }
  XYVector slope() const { return XYVector(u.slope(), v.slope()); }
  Storage::TrackState state() const
  {
    Storage::TrackState s(u.offset(), v.offset(), u.slope(), v.slope());
    s.setErrU(std::sqrt(u.offsetVar()), std::sqrt(u.slopeVar()), u.cov());
    s.setErrV(std::sqrt(v.offsetVar()), std::sqrt(v.slopeVar()), v.cov());
    return s;
  }

  void addPoint(const XYZPoint& pos, const XYZVector& err)
  {
    u.addPoint(pos.z(), pos.x(), 1 / (err.x() * err.x()));
    v.addPoint(pos.z(), pos.y(), 1 / (err.y() * err.y()));
  }
  void fit()
  {
    u.fit();
    v.fit();
  }
};

void Processors::fitTrack(Storage::Track& track)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    fit.addPoint(cluster.posGlobal(), cluster.errGlobal());
  }
  fit.fit();

  // compute chi2 for fitted track
  Storage::TrackState state = fit.state();
  double chi2 = 0;
  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    XYPoint t = state.offset() + state.slope() * cluster.posGlobal().z();
    double resX = (cluster.posGlobal().x() - t.x()) / cluster.errGlobal().x();
    double resY = (cluster.posGlobal().y() - t.y()) / cluster.errGlobal().y();
    chi2 += resX * resX + resY * resY;
  }

  // update track in-place
  track.setGlobalState(state);
  track.setGoodnessOfFit(chi2, 2 * (track.numClusters() - 2));
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
    fit.addPoint(pos, cluster->errGlobal());
  }
  fit.fit();
  return fit.state();
}

Storage::TrackState
Processors::fitTrackLocalUnbiased(const Storage::Track& track,
                                  const Mechanics::Sensor& reference,
                                  Index referenceId)
{
  SimpleStraightFitter fit;

  for (Index icluster = 0; icluster < track.numClusters(); ++icluster) {
    const Storage::Cluster& cluster = *track.getCluster(icluster);
    if (cluster.sensorId() == referenceId)
      continue;
    XYZPoint pos = reference.globalToLocal() * cluster.posGlobal();
    // TODO 2016-10-13 msmk: also transform error to local frame
    fit.addPoint(pos, cluster.errGlobal());
  }
  fit.fit();
  return fit.state();
}
