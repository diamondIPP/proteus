// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/*
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-02
 */

#include "applyregions.h"

#include "mechanics/device.h"
#include "storage/sensorevent.h"

namespace proteus {

ApplyRegions::ApplyRegions(const Sensor& sensor) : m_sensor(sensor) {}

std::string ApplyRegions::name() const
{
  return "ApplyRegions(" + m_sensor.name() + ")";
}

void ApplyRegions::execute(SensorEvent& sensorEvent) const
{
  // TODO 2017-02 msmk: check whether is better (faster) to iterate first
  //                    over hits or first over regions
  for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
    Hit& hit = sensorEvent.getHit(ihit);

    for (Index iregion = 0; iregion < m_sensor.regions().size(); ++iregion) {
      const auto& region = m_sensor.regions()[iregion];
      if (region.colRow.isInside(hit.col(), hit.row())) {
        hit.setRegion(iregion);
        // regions are exclusive and each hit can only belong to one region
        break;
      }
    }
  }
}

} // namespace proteus
