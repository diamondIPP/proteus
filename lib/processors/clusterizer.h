/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_CLUSTERIZER_H
#define PT_CLUSTERIZER_H

#include "loop/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Sensor;
}
namespace Storage {
class Cluster;
}

namespace Processors {

/** Cluster neighboring hits and calculate cluster properties. */
class BaseClusterizer : public Loop::Processor {
public:
  std::string name() const;
  void process(Storage::Event& event) const;

protected:
  BaseClusterizer(const std::string& namePrefix,
                  const Mechanics::Sensor& sensor);

private:
  virtual void estimateProperties(Storage::Cluster& cluster) const = 0;

  const Mechanics::Sensor& m_sensor;
  std::string m_name;
};

/** Cluster hits and average the position with equal weights for all hits.
 *
 * The fastest hit time is used as the cluster time.
 */
class BinaryClusterizer : public BaseClusterizer {
public:
  BinaryClusterizer(const Mechanics::Sensor& sensor);

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

/** Cluster hits and calculate position by weighting each hit with its value.
 *
 * The fastest hit time is used as the cluster time.
 */
class ValueWeightedClusterizer : public BaseClusterizer {
public:
  ValueWeightedClusterizer(const Mechanics::Sensor& sensor);

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

/** Cluster hits and take position and timing only from the fastest hit. */
class FastestHitClusterizer : public BaseClusterizer {
public:
  FastestHitClusterizer(const Mechanics::Sensor& sensor);

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

} // namespace Processors

#endif // PT_CLUSTERIZER_H
