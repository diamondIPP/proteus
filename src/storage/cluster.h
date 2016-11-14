#ifndef PT_CLUSTER_H
#define PT_CLUSTER_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"
#include "utils/interval.h"

namespace Storage {

class Hit;
class Track;
class Plane;

class Cluster {
public:
  typedef Utils::Box<2, int> Area;

  void setPosPixel(const XYPoint& cr) { m_cr = cr; }
  void setPosPixel(double col, double row) { m_cr.SetXY(col, row); }
  void setCovPixel(const SymMatrix2& cov) { m_covCr = cov; }
  void setErrPixel(double stdCol, double stdRow);
  void setTiming(double timing) { m_timing = timing; }
  /** Set global position using the transform from pixel to global coords. */
  void transformToGlobal(const Transform3D& pixelToGlobal);

  const XYPoint& posPixel() const { return m_cr; }
  const XYZPoint& posGlobal() const { return m_xyz; }
  const SymMatrix2& covPixel() const { return m_covCr; }
  const SymMatrix3& covGlobal() const { return m_covXyz; }
  double timing() const { return m_timing; }
  double value() const { return m_value; }

  Index sensorId() const;

  /** The area enclosing the cluster.
   *
   * \return Returns an empty area for an empty cluster w/o hits.
   */
  Area area() const;
  int size() const { return static_cast<int>(m_hits.size()); }
  int sizeCol() const;
  int sizeRow() const;

  void addHit(Hit* hit);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i); }
  const Hit* getHit(Index i) const { return m_hits.at(i); }

  void setTrack(const Track* track);
  bool isInTrack() const { return m_track != NULL; }
  const Track* track() const { return m_track; }
  /** \deprecated Use `track()` instead. */
  const Track* getTrack() const { return m_track; }

  Index getNumHits() const { return m_hits.size(); }
  double getPixX() const { return m_cr.x(); }
  double getPixY() const { return m_cr.y(); }
  double getPixErrX() const { return std::sqrt(m_covCr(0, 0)); }
  double getPixErrY() const { return std::sqrt(m_covCr(1, 1)); }
  double getPosX() const { return m_xyz.x(); }
  double getPosY() const { return m_xyz.y(); }
  double getPosZ() const { return m_xyz.z(); }
  double getPosErrX() const { return std::sqrt(m_covXyz(0, 0)); }
  double getPosErrY() const { return std::sqrt(m_covXyz(1, 1)); }
  double getPosErrZ() const { return std::sqrt(m_covXyz(2, 2)); }
  double getTiming() const { return timing(); }
  double getValue() const { return value(); }
  double getMatchDistance() const { return m_matchDistance; }
  int getIndex() const { return m_index; }

  Track* getMatchedTrack() const { return m_matchedTrack; }
  void setMatchedTrack(Track* track) { m_matchedTrack = track; }
  void setMatchDistance(double value) { m_matchDistance = value; }

  /** \deprecated Access via `Plane` already provides that information. */
  Plane* getPlane() const { return m_plane; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Cluster(); // The Event class manages the memory, not the user

  XYPoint m_cr;
  SymMatrix2 m_covCr;
  double m_timing; // The timing of the underlying hits
  double m_value;  // The combined value of all its hits
  XYZPoint m_xyz;
  SymMatrix3 m_covXyz;

  std::vector<Hit*> m_hits; // List of hits composing the cluster

  int m_index;
  Plane* m_plane;         //<! The plane containing the cluster
  const Track* m_track;   // The track containing this cluster
  Track* m_matchedTrack;  // Track matched to this cluster in DUT analysis (not
                          // stored)
  double m_matchDistance; // Distance to matched track

  friend class Plane;     // Needs to use the set plane method
  friend class Event;     // Needs access the constructor and destructor
  friend class StorageIO; // Needs access to the track index
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
