/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_CLUSTERIZER_H
#define PT_CLUSTERIZER_H

#include <set>
#include <string>

#include "processors/processor.h"
#include "utils/definitions.h"
#include "utils/logger.h"

namespace Mechanics {
class Device;
}
namespace Storage {
class Cluster;
}
namespace Utils {
class EventLoop;
}

namespace Processors {

/** Cluster neighboring hits and calculate cluster properties. */
class BaseClusterizer : public Processor {
public:
  std::string name() const;
  void process(Storage::Event& event) const;

protected:
  template <typename Container>
  BaseClusterizer(const std::string& name,
                  const Mechanics::Device& device,
                  const Container& sensorIds)
      : m_name(name)
      , m_device(device)
      , m_sensorIds(std::begin(sensorIds), std::end(sensorIds))
      , m_maxDistSquared(1)
  {
    using Utils::logger;
    DEBUG(m_name, " sensors:\n");
    for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id)
      DEBUG("  ", *id, '\n');
  }

private:
  virtual void estimateProperties(Storage::Cluster& cluster) const = 0;

  std::string m_name;
  const Mechanics::Device& m_device;
  std::set<Index> m_sensorIds;
  double m_maxDistSquared;
};

/** Cluster hits and average the position with equal weights for all hits.
 *
 * The fastest hit time is used as the cluster time.
 */
class BinaryClusterizer : public BaseClusterizer {
public:
  template <typename Container>
  BinaryClusterizer(const Mechanics::Device& device, const Container& sensorIds)
      : BaseClusterizer("BinaryClusterizer", device, sensorIds)
  {
  }

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

/** Cluster hits and calculate position by weighting each hit with its value.
 *
 * The fastest hit time is used as the cluster time.
 */
class ValueWeightedClusterizer : public BaseClusterizer {
public:
  template <typename Container>
  ValueWeightedClusterizer(const Mechanics::Device& device,
                           const Container& sensorIds)
      : BaseClusterizer("ValueWeightedClusterizer", device, sensorIds)
  {
  }

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

/** Cluster hits and take position and timing only from the fastest hit. */
class FastestHitClusterizer : public BaseClusterizer {
public:
  template <typename Container>
  FastestHitClusterizer(const Mechanics::Device& device,
                        const Container& sensorIds)
      : BaseClusterizer("FastestHitClusterizer", device, sensorIds)
  {
  }

private:
  void estimateProperties(Storage::Cluster& cluster) const;
};

void setupClusterizers(const Mechanics::Device& device, Utils::EventLoop& loop);

} // namespace Processors

#endif // PT_CLUSTERIZER_H
