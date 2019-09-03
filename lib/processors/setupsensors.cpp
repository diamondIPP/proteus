// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#include "setupsensors.h"

#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applylocaltransform.h"
#include "processors/applyregions.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"

namespace proteus {

namespace {
void setupSensor(Index sensorId, const Sensor& sensor, EventLoop& loop)
{
  // hit mapper
  if (sensor.measurement() == Sensor::Measurement::Ccpdv4Binary) {
    loop.addSensorProcessor(sensorId, std::make_shared<CCPDv4HitMapper>());
  }
  // sensor regions
  if (sensor.hasRegions()) {
    loop.addSensorProcessor(sensorId, std::make_shared<ApplyRegions>(sensor));
  }
  // clusterizer
  switch (sensor.measurement()) {
  case Sensor::Measurement::PixelBinary:
  case Sensor::Measurement::Ccpdv4Binary:
    loop.addSensorProcessor(sensorId,
                            std::make_shared<BinaryClusterizer>(sensor));
    break;
  case Sensor::Measurement::PixelTot:
    loop.addSensorProcessor(sensorId,
                            std::make_shared<ValueWeightedClusterizer>(sensor));
  }
  // digital-to-local transform
  loop.addSensorProcessor(
      sensorId, std::make_shared<ApplyLocalTransformCartesian>(sensor));
}
} // namespace

void setupPerSensorProcessing(const Device& device, EventLoop& loop)
{
  for (auto isensor : device.sensorIds()) {
    setupSensor(isensor, device.getSensor(isensor), loop);
  }
}

} // namespace proteus
