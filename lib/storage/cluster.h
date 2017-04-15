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

class Cluster {
public:
  using Area = Utils::Box<2, int>;

  Cluster(Index sensorId_, Index index_);

  void setPixel(const XYPoint& cr, const SymMatrix2& cov);
  void setPixel(float col, float row, float stdCol, float stdRow);
  void setTime(float time_) { m_time = time_; }
  void setValue(float value_) { m_value = value_; }
  /** Calculate local and global coordinates from the pixel coordinates. */
  void transform(const Mechanics::Sensor& sensor);
  void setTrack(Index track);
  void setMatchedState(Index state);

  Index sensorId() const { return m_sensorId; }
  Index region() const;
  Index index() const { return m_index; }

  const XYPoint& posPixel() const { return m_cr; }
  const XYPoint& posLocal() const { return m_uv; }
  const XYZPoint& posGlobal() const { return m_xyz; }
  const SymMatrix2& covPixel() const { return m_crCov; }
  const SymMatrix2& covLocal() const { return m_uvCov; }
  const SymMatrix3& covGlobal() const { return m_xyzCov; }
  float time() const { return m_time; }
  float value() const { return m_value; }

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

  bool isInTrack() const { return m_track != kInvalidIndex; }
  Index track() const { return m_track; }
  bool isMatched() const { return m_matchedState != kInvalidIndex; }
  Index matchedState() const { return m_matchedState; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  XYPoint m_cr;
  float m_time;  // The timing of the underlying hits
  float m_value; // The combined value of all its hits
  XYPoint m_uv;
  SymMatrix2 m_uvCov;
  SymMatrix2 m_crCov;
  XYZPoint m_xyz;
  SymMatrix3 m_xyzCov;

  std::vector<Hit*> m_hits; // List of hits composing the cluster

  Index m_index;
  Index m_sensorId;
  Index m_track;
  Index m_matchedState;
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
