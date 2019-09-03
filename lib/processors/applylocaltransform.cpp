// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "applylocaltransform.h"

#include "mechanics/sensor.h"
#include "storage/sensorevent.h"

namespace proteus {

ApplyLocalTransformCartesian::ApplyLocalTransformCartesian(const Sensor& sensor)
    : m_sensor(sensor)
{
}

std::string ApplyLocalTransformCartesian::name() const
{
  return "ApplyLocalTransformCartesian";
}

void ApplyLocalTransformCartesian::execute(SensorEvent& sensorEvent) const
{
  DiagMatrix4 scalePitch = m_sensor.pitch().asDiagonal();

  for (Index icluster = 0; icluster < sensorEvent.numClusters(); icluster++) {
    Cluster& cluster = sensorEvent.getCluster(icluster);

    SymMatrix4 cov = SymMatrix4::Zero();
    cov(kU, kU) = cluster.colVar();
    cov(kV, kU) = cov(kU, kV) = cluster.colRowCov();
    cov(kV, kV) = cluster.rowVar();
    cov(kS, kS) = cluster.timestampVar();
    cluster.setLocal(m_sensor.transformPixelToLocal(
                         cluster.col(), cluster.row(), cluster.timestamp()),
                     transformCovariance(scalePitch, cov));
  }
}

} // namespace proteus
