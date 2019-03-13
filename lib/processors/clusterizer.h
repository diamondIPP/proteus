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
namespace Processors {

/** Cluster hits and average the position with equal weights for all hits.
 *
 * The fastest hit time is used as the cluster time.
 */
class BinaryClusterizer : public Loop::Processor {
public:
  BinaryClusterizer(const Mechanics::Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Sensor& m_sensor;
};

/** Cluster hits and calculate position by weighting each hit with its value.
 *
 * The fastest hit time is used as the cluster time.
 */
class ValueWeightedClusterizer : public Loop::Processor {
public:
  ValueWeightedClusterizer(const Mechanics::Sensor& sensor) : m_sensor(sensor)
  {
  }

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Sensor& m_sensor;
};

/** Cluster hits and take position and timing only from the fastest hit. */
class FastestHitClusterizer : public Loop::Processor {
public:
  FastestHitClusterizer(const Mechanics::Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Sensor& m_sensor;
};

} // namespace Processors

#endif // PT_CLUSTERIZER_H
