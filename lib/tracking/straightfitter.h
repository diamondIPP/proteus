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

/** Estimate local track parameters using a straight line fit.
 *
 * This calculates global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes.
 */
class Straight3dFitter : public Loop::Processor {
public:
  Straight3dFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

/** Estimate local track parameters including time using a straight line fit.
 *
 * This calculates global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes.
 */
class Straight4dFitter : public Loop::Processor {
public:
  Straight4dFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

/** Estimate local track parameters without local information.
 *
 * This calculates new global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes. If the track has any
 * measurement information on a sensor, this measurement is ignored when
 * estimating the local track parameters on that sensor.
 */
class UnbiasedStraight3dFitter : public Loop::Processor {
public:
  UnbiasedStraight3dFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

/** Estimate local track parameters including time without local information.
 *
 * This calculates new global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes. If the track has any
 * measurement information on a sensor, this measurement is ignored when
 * estimating the local track parameters on that sensor.
 */
class UnbiasedStraight4dFitter : public Loop::Processor {
public:
  UnbiasedStraight4dFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Geometry& m_geo;
};

} // namespace Tracking

#endif // PT_STRAIGHTFITTER_H
