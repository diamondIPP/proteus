#ifndef PT_CLUSTER_H
#define PT_CLUSTER_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"
#include "utils/interval.h"

namespace Mechanics {
class Sensor;
}

namespace Storage {

class Hit;
class Track;
class TrackState;
class Plane;

class Cluster {
public:
  using Area = Utils::Box<2, int>;

  Index index() const { return m_index; }
  Index sensorId() const;
  Index region() const;

  void setPixel(const XYPoint& cr, const SymMatrix2& cov);
  void setPixel(double col, double row, double stdCol, double stdRow);
  void setTime(double time_) { m_time = time_; }
  /** Calculate local and global coordinates from the pixel coordinates. */
  void transform(const Mechanics::Sensor& sensor);

  const XYPoint& posPixel() const { return m_cr; }
  const XYPoint& posLocal() const { return m_uv; }
  const XYZPoint& posGlobal() const { return m_xyz; }
  const SymMatrix2& covPixel() const { return m_crCov; }
  const SymMatrix2& covLocal() const { return m_uvCov; }
  const SymMatrix3& covGlobal() const { return m_xyzCov; }
  double time() const { return m_time; }
  double value() const { return m_value; }

  /** The area enclosing the cluster in pixel coordinates.
   *
   * \return Returns an empty area for an empty cluster.
   */
  Area areaPixel() const;
  int size() const { return static_cast<int>(m_hits.size()); }
  int sizeCol() const;
  int sizeRow() const;

  void addHit(Hit* hit);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i); }
  const Hit* getHit(Index i) const { return m_hits.at(i); }

  void setTrack(const Track* track);
  const Track* track() const { return m_track; }
  /** \deprecated Use `track()` instead. */
  const Track* getTrack() const { return m_track; }

  void setMatchedState(const TrackState* state) { m_matchedState = state; }
  const TrackState* matchedState() const { return m_matchedState; }

  void setMatchedTrack(const Track* track) { m_matched = track; }
  bool hasMatchedTrack() const { return (m_matched != NULL); }
  const Track* matchedTrack() const { return m_matched; }
  /** \deprecated Use `matchedTrack()` instead. */
  const Track* getMatchedTrack() const { return m_matched; }

  Index getNumHits() const { return m_hits.size(); }
  double getPixX() const { return m_cr.x(); }
  double getPixY() const { return m_cr.y(); }
  double getPixErrX() const { return std::sqrt(m_crCov(0, 0)); }
  double getPixErrY() const { return std::sqrt(m_crCov(1, 1)); }
  double getPosX() const { return m_xyz.x(); }
  double getPosY() const { return m_xyz.y(); }
  double getPosZ() const { return m_xyz.z(); }
  double getPosErrX() const { return std::sqrt(m_xyzCov(0, 0)); }
  double getPosErrY() const { return std::sqrt(m_xyzCov(1, 1)); }
  double getPosErrZ() const { return std::sqrt(m_xyzCov(2, 2)); }
  double getTiming() const { return time(); }
  double getValue() const { return value(); }
  void setMatchDistance(double value) { m_matchDistance = value; }
  double getMatchDistance() const { return m_matchDistance; }
  int getIndex() const { return m_index; }

  /** \deprecated Access via `Plane` already provides that information. */
  Plane* getPlane() const { return m_plane; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Cluster(); // The Event class manages the memory, not the user

  XYPoint m_cr;
  double m_time;  // The timing of the underlying hits
  double m_value; // The combined value of all its hits
  XYPoint m_uv;
  SymMatrix2 m_uvCov;
  SymMatrix2 m_crCov;
  XYZPoint m_xyz;
  SymMatrix3 m_xyzCov;

  std::vector<Hit*> m_hits; // List of hits composing the cluster

  int m_index;
  Plane* m_plane;       //<! The plane containing the cluster
  const Track* m_track; // The track containing this cluster
  const TrackState* m_matchedState;
  const Track* m_matched; // Track matched to this cluster in DUT analysis (not
                          // stored)
  double m_matchDistance; // Distance to matched track

  friend class Plane;     // Needs to use the set plane method
  friend class Event;     // Needs access the constructor and destructor
  friend class StorageIO; // Needs access to the track index
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
