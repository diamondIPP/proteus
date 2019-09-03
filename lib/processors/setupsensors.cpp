// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#include "setupsensors.h"

#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applyregions.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"

namespace proteus {

void setupHitPreprocessing(const Device& device, EventLoop& loop)
{
  for (auto isensor : device.sensorIds()) {
    const Sensor& sensor = device.getSensor(isensor);

    // hit mapper
    if (sensor.measurement() == Sensor::Measurement::Ccpdv4Binary) {
      loop.addProcessor(std::make_shared<CCPDv4HitMapper>(isensor));
    }
    // sensor regions
    if (sensor.hasRegions()) {
      loop.addSensorProcessor(isensor, std::make_shared<ApplyRegions>(sensor));
    }
  }
}

void setupClusterizers(const Device& device, EventLoop& loop)
{
  for (auto isensor : device.sensorIds()) {
    const Sensor& sensor = device.getSensor(isensor);
    switch (sensor.measurement()) {
    case Sensor::Measurement::PixelBinary:
    case Sensor::Measurement::Ccpdv4Binary:
      loop.addProcessor(std::make_shared<BinaryClusterizer>(sensor));
      break;
    case Sensor::Measurement::PixelTot:
      loop.addProcessor(std::make_shared<ValueWeightedClusterizer>(sensor));
      break;
    }
  }
}

} // namespace proteus
