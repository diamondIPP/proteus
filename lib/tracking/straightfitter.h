/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#pragma once

#include <vector>

#include "loop/processor.h"

namespace proteus {

class Device;
class Geometry;

/** Estimate local track parameters using a straight line fit.
 *
 * This calculates global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes.
 */
class Straight3dFitter : public Processor {
public:
  Straight3dFitter(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Geometry& m_geo;
};

/** Estimate local track parameters including time using a straight line fit.
 *
 * This calculates global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes.
 */
class Straight4dFitter : public Processor {
public:
  Straight4dFitter(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Geometry& m_geo;
};

/** Estimate local track parameters without local information.
 *
 * This calculates new global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes. If the track has any
 * measurement information on a sensor, this measurement is ignored when
 * estimating the local track parameters on that sensor.
 */
class UnbiasedStraight3dFitter : public Processor {
public:
  UnbiasedStraight3dFitter(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Geometry& m_geo;
};

/** Estimate local track parameters including time without local information.
 *
 * This calculates new global track parameters and global goodness-of-fit and
 * the local track parameters on the all sensor planes. If the track has any
 * measurement information on a sensor, this measurement is ignored when
 * estimating the local track parameters on that sensor.
 */
class UnbiasedStraight4dFitter : public Processor {
public:
  UnbiasedStraight4dFitter(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Geometry& m_geo;
};

} // namespace proteus
