/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-02
 */

#pragma once

#include "loop/processor.h"

namespace proteus {

class Device;
class EventLoop;
class Sensor;

/** Assign region ids to hits using the sensor region information. */
class ApplyRegions : public Processor {
public:
  ApplyRegions(const Sensor& sensor);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Sensor& m_sensor;
};

void setupRegions(const Device& device, EventLoop& loop);

} // namespace proteus
