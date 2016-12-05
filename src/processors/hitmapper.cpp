/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "hitmapper.h"

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Processors::CCPDv4HitMapper::CCPDv4HitMapper(Index sensorId)
    : m_sensorId(sensorId)
{
  DEBUG("map FE-I4 to CCPDv4 for sensor ", sensorId);
}

std::string Processors::CCPDv4HitMapper::name() const
{
  return "CCPDv4HitMapper(sensorId=" + std::to_string(m_sensorId) + ")";
}

void Processors::CCPDv4HitMapper::process(Storage::Event& event) const
{
  Storage::Plane* plane = event.getPlane(m_sensorId);
  for (Index ihit = 0; ihit < plane->numHits(); ++ihit) {
    Storage::Hit& hit = *plane->getHit(ihit);

    // two hits in digital column correspond to two hits in a sensor row.
    // * lower digital hit -> left sensor hit
    // * upper digital hit -> right sensor hit
    // translated from the mapping.cc ROOT script
    // TODO 2016-10-13 msmk: assumes correct mapping, i.e. even to even
    Index fei4Col = hit.digitalCol();
    Index fei4Row = hit.digitalRow();
    Index col, row;
    if ((fei4Col % 2) == 0) {
      if ((fei4Row % 2) != 0)
        col = 2 * fei4Col;
      if ((fei4Row % 2) == 0)
        col = 2 * fei4Col + 1;
    } else {
      if ((fei4Row % 2) != 0)
        col = 2 * fei4Col + 1;
      if ((fei4Row % 2) == 0)
        col = 2 * fei4Col;
    }
    row = fei4Row / 2;
    hit.setPhysicalAddress(col, row);
  }
}

void Processors::setupHitMappers(const Mechanics::Device& device,
                                 Utils::EventLoop& loop)
{
  for (Index sensorId = 0; sensorId < device.numSensors(); ++sensorId) {
    const Mechanics::Sensor* sensor = device.getSensor(sensorId);
    if (sensor->measurement() == Mechanics::Sensor::Measurement::Ccpdv4Binary) {
      loop.addProcessor(std::make_shared<CCPDv4HitMapper>(sensorId));
    }
  }
}
