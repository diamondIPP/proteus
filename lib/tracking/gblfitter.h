/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_GBLFitter_H
#define PT_GBLFitter_H

#include <vector>

#include "processors/processor.h"
#include "utils/definitions.h"
#include "storage/event.h"
#include <Eigen/Dense>
#include <GblTrajectory.h>

namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics

namespace Tracking {

/** Estimate local track parameters using a straight line track model.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the selected sensor planes.
 */
class GBLFitter : public Processors::Processor {
public:
  GBLFitter(const Mechanics::Device& device);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
};

// For GBL track fitting
void fitTrackGBL(Storage::Track& track);

} // namespace Tracking

#endif // PT_GBLFitter_H
