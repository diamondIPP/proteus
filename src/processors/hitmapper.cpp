/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#include "hitmapper.h"

#include <set>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/eventloop.h"

std::string Processors::CCPDv4HitMapper::name() const
{
  return "CCPDv4HitMapper";
}

void Processors::CCPDv4HitMapper::process(Storage::Event& event) const
{
  for (auto id = m_sensorIds.begin(); id != m_sensorIds.end(); ++id) {
    Storage::Plane* plane = event.getPlane(*id);
    for (Index ihit = 0; ihit < plane->numHits(); ++ihit) {
      Storage::Hit* hit = plane->getHit(ihit);

      // two hits in digital column correspond to two hits in a sensor row.
      // * lower digital hit -> left sensor hit
      // * upper digital hit -> right sensor hit
      // translated from the mapping.cc ROOT script
      // TODO 2016-10-13 msmk: assumes correct mapping, i.e. even to even
      Index fei4Col = hit->col();
      Index fei4Row = hit->row();
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
      hit->setAddress(col, row);
    }
  }
}

void Processors::setupHitMapper(const Mechanics::Device& device,
                                Utils::EventLoop& loop)
{
  using Mechanics::Sensor;

  std::set<Index> ccpdv4;

  for (Index isensor = 0; isensor < device.numSensors(); ++isensor) {
    const Sensor* sensor = device.getSensor(isensor);
    if (sensor->measurement() == Sensor::CCPDV4_BINARY)
      ccpdv4.insert(isensor);
  }

  if (!ccpdv4.empty())
    loop.addProcessor(std::make_shared<CCPDv4HitMapper>(ccpdv4));
}
