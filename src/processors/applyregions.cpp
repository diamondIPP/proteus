/*
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-02
 */

#include "applyregions.h"

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/eventloop.h"

Processors::ApplyRegions::ApplyRegions(const Mechanics::Sensor& sensor)
    : m_sensor(sensor)
{
}

std::string Processors::ApplyRegions::name() const
{
  return "ApplyRegions(" + m_sensor.name() + ")";
}

void Processors::ApplyRegions::process(Storage::Event& event) const
{
  Storage::Plane& sensorEvent = *event.getPlane(m_sensor.id());

  // TODO 2017-02 msmk: check whether is better (faster) to iterate first
  //                    over hits or first over regions
  for (Index ihit = 0; ihit < sensorEvent.numHits(); ++ihit) {
    Storage::Hit& hit = *sensorEvent.getHit(ihit);

    for (Index iregion = 0; iregion < m_sensor.regions().size(); ++iregion) {
      const auto& region = m_sensor.regions()[iregion];
      if (region.areaPixel.isInside(hit.col(), hit.row())) {
        hit.setRegion(iregion);
        // regions are exclusive and each hit can only belong to one region
        break;
      }
    }
  }
}

void Processors::setupRegions(const Mechanics::Device& device,
                              Utils::EventLoop& loop)
{
  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    const Mechanics::Sensor& sensor = *device.getSensor(isensor);
    if (sensor.hasRegions()) {
      loop.addProcessor(std::make_shared<ApplyRegions>(sensor));
    }
  }
}