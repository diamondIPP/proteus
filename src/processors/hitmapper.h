/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_HITMAPPER_H
#define PT_HITMAPPER_H

#include "processors/processor.h"
#include "utils/definitions.h"
#include "utils/logger.h"

#include <set>

namespace Mechanics {
class Device;
}
namespace Utils {
class EventLoop;
}

namespace Processors {

/** Map FEI4 digital address to correct CCPDv4 sensor pixel address. */
class CCPDv4HitMapper : public Processor {
public:
  template <typename Container>
  CCPDv4HitMapper(const Container& sensorIds)
      : m_sensorIds(std::begin(sensorIds), std::end(sensorIds))
  {
    using Utils::logger;
    DEBUG("FEI4 to CCPDv4 mapping sensors:\n");
    for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id)
      DEBUG("  ", *id, '\n');
  }

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  std::set<Index> m_sensorIds;
};

void setupHitMappers(const Mechanics::Device& device, Utils::EventLoop& loop);

} // namespace Processors

#endif // PT_HITMAPPER_H
