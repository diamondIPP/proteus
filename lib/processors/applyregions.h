/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-02
 */

#ifndef PT_APPLYREGIONS_H
#define PT_APPLYREGIONS_H

#include "loop/processor.h"

namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics

namespace Loop {
class EventLoop;
}

namespace Processors {

/** Assign region ids to hits using the sensor region information. */
class ApplyRegions : public Loop::Processor {
public:
  ApplyRegions(const Mechanics::Sensor& sensor);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Sensor& m_sensor;
};

void setupRegions(const Mechanics::Device& device, Loop::EventLoop& loop);

} // namespace Processors

#endif // PT_APPLYREGIONS_H
