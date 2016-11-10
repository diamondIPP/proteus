/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_HITMAPPER_H
#define PT_HITMAPPER_H

#include <vector>

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
}
namespace Utils {
class EventLoop;
}

namespace Processors {

/** Map FE-I4 digital address to correct CCPDv4 sensor pixel address. */
class CCPDv4HitMapper : public Processor {
public:
  CCPDv4HitMapper(const std::vector<Index>& sensorIds);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  std::vector<Index> m_sensorIds;
};

void setupHitMappers(const Mechanics::Device& device, Utils::EventLoop& loop);

} // namespace Processors

#endif // PT_HITMAPPER_H
