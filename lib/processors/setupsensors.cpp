/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#include "setupsensors.h"

#include "mechanics/device.h"
#include "processors/applyregions.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"
#include "utils/eventloop.h"

void Processors::setupHitPreprocessing(const Mechanics::Device& device,
                                       Utils::EventLoop& loop)
{
  using namespace Mechanics;

  for (auto isensor : device.sensorIds()) {
    const Sensor& sensor = *device.getSensor(isensor);

    // hit mapper
    if (sensor.measurement() == Sensor::Measurement::Ccpdv4Binary) {
      loop.addProcessor(std::make_shared<CCPDv4HitMapper>(isensor));
    }
    // sensor regions
    if (sensor.hasRegions()) {
      loop.addProcessor(std::make_shared<ApplyRegions>(sensor));
    }
  }
}

void Processors::setupClusterizers(const Mechanics::Device& device,
                                   Utils::EventLoop& loop)
{
  using namespace Mechanics;

  for (auto isensor : device.sensorIds()) {
    const Sensor& sensor = *device.getSensor(isensor);
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
