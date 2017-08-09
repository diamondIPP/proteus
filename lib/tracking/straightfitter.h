/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_STRAIGHTFITTER_H
#define PT_STRAIGHTFITTER_H

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics

namespace Tracking {

/** Estimate local track parameters using a straight line track model.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the all sensor planes.
 */
class StraightFitter : public Processors::Processor {
public:
  StraightFitter(const Mechanics::Device& device);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
};

} // namespace Tracking

#endif // PT_STRAIGHTFITTER_H
