/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_STRAIGHTFITTER_H
#define PT_STRAIGHTFITTER_H

#include <vector>

#include "loop/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
class Geometry;
} // namespace Mechanics
namespace Tracking {

/** Estimate local track parameters using a straight line track model.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the all sensor planes.
 */
class StraightFitter : public Loop::Processor {
public:
  StraightFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

/** Estimate local track parameters w/o the local information.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the all sensor planes.
 * If the track has cluster information on the local sensor, it is ignored
 * only for calculating the track parameters on that sensor.
 */
class UnbiasedStraightFitter : public Loop::Processor {
public:
  UnbiasedStraightFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

} // namespace Tracking

#endif // PT_STRAIGHTFITTER_H
