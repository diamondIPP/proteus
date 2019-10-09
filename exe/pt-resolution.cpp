// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \brief Estimate reconstruction resolution using a virtual GBL fit.
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2019-09-10
 */

#include <algorithm>
#include <iostream>

#include "mechanics/device.h"
#include "storage/event.h"
#include "tracking/gblfitter.h"
#include "tracking/propagation.h"
#include "utils/arguments.h"
#include "utils/logger.h"

using namespace proteus;

/// Build a single cluster with a given local position
///
/// This assumes that the cluster is a single-hit cluster with a spatial
/// uncertainty determined purely by its pitch. This should be a conservative
/// over-estimation.
Cluster buildCluster(const Sensor& sensor, const Vector4& local)
{
  constexpr Scalar pixVar = 1.0 / 12.0;

  // local position in digital pixel coordinates
  Vector4 pixel = sensor.transformLocalToPixel(local);
  auto col = pixel[kU];
  auto row = pixel[kV];
  auto timestamp = pixel[kS];
  auto value = Scalar(0);

  // local covariance in digital pixel coordinates assuming binary pixels
  SymMatrix4 pixelCov = Vector4::Constant(pixVar).asDiagonal();
  // local variance in metric coordinates
  SymMatrix4 localCov =
      transformCovariance(sensor.pitch().asDiagonal(), pixelCov);

  Cluster cluster(col, row, timestamp, value, pixVar, pixVar, pixVar);
  cluster.setLocal(local, localCov);
  return cluster;
}

/// Build an event with a single track undisturbed by material or measurements.
Event buildSingleTrackEvent(const Device& device,
                            const std::vector<Index>& trackingIds)
{
  // initial global track state based on beam information
  // TODO use actual beam position or center of the tracking sensors
  TrackState globalState(Vector4::Zero(), SymMatrix4::Identity(),
                         device.geometry().beamSlope(),
                         device.geometry().beamSlopeCovariance());
  Track track(globalState);
  Event event(device.numSensors());

  // build the track w/ clusters on each tracking sensor
  for (auto sensorId : trackingIds) {
    auto& sensorEvent = event.getSensorEvent(sensorId);
    const auto& sensor = device.getSensor(sensorId);
    const auto& localPlane = device.geometry().getPlane(sensorId);

    auto localState = propagateTo(globalState, Plane(), localPlane);

    sensorEvent.addCluster(buildCluster(sensor, localState.position()));
    // TODO use generated cluster index once addCluster returns it
    track.addCluster(sensorId, 0);
  }
  event.addTrack(track);

  return event;
}

/// Print the track state uncertainty as resolution.
void printResolutionTable(const Device& device,
                          const Event& event,
                          std::ostream& os)
{
  os.precision(3);

  os << "# id, name: sensor id and name\n";
  os << "# z: sensor position along the beam\n";
  os << "# res_{u,v}: resolution on the two sensor plane coordinates\n";
  os << "# res_{du,dv}: resolution in track slope relative to plane normal\n";
  os << "#\n";
  os << "id\tname\tz\tres_u\tres_v\tres_du\tres_dv\n";

  auto sensorIds = sortedAlongBeam(device.geometry(), device.sensorIds());
  for (auto sensorId : sensorIds) {
    const auto& sensor = device.getSensor(sensorId);
    const auto& plane = device.geometry().getPlane(sensorId);
    const auto& sensorEvent = event.getSensorEvent(sensorId);
    const auto& localState = sensorEvent.getLocalState(0);
    const auto& stddev = extractStdev(localState.cov());

    os << sensor.id();
    os << '\t' << sensor.name();
    os << '\t' << plane.origin()[kZ];
    for (auto idx : {kLoc0, kLoc1, kSlopeLoc0, kSlopeLoc1}) {
      os << '\t' << stddev[idx];
    }
    os << '\n';
  }

  os.flush();
}

int main(int argc, char const* argv[])
{
  globalLogger().setMinimalLevel(Logger::Level::Warning);

  // To avoid having unused command line options, argument parsing is
  // implemented manually here w/ a limited amount of options
  Arguments args("compute expected tracking resolution");
  args.addOption('d', "device", "device configuration file", "device.toml");
  args.addOption('g', "geometry", "use a different geometry file");
  args.addOptionMulti('i', "ignore-sensor",
                      "do not use the sensor for tracking");

  // parse should print help automatically
  if (args.parse(argc, argv)) {
    std::exit(EXIT_FAILURE);
  }

  // load device w/ optional geometry override
  auto pathDev = args.get("device");
  auto pathGeo = (args.has("geometry") ? args.get("geometry") : std::string());
  auto device = Device::fromFile(pathDev, pathGeo);

  // determine which sensors are to be used for tracking
  std::vector<Index> trackingIds;
  if (args.has("ignore-sensor")) {
    auto sensorIds = device.sensorIds();
    auto ignoreIds = args.get<std::vector<Index>>("ignore-sensor");
    std::sort(sensorIds.begin(), sensorIds.end());
    std::sort(ignoreIds.begin(), ignoreIds.end());
    std::set_difference(sensorIds.begin(), sensorIds.end(), ignoreIds.begin(),
                        ignoreIds.end(), std::back_inserter(trackingIds));
  } else {
    // every sensor is used by default
    trackingIds = device.sensorIds();
  }

  // setup perfect fake track
  auto event = buildSingleTrackEvent(device, trackingIds);
  // fit track to get uncertainties.
  // for the GBL fit the uncertainties depend only on the measurement
  // and scattering covariances and the geometry, but not on the actual values
  // of the measurements. the resulting fit uncertainties from fitting a
  // perfect track are therefore a reasonable estimator of the expected
  // reconstruction uncertainties.
  GblFitter fitter(device);
  fitter.execute(event);

  printResolutionTable(device, event, std::cout);

  return EXIT_SUCCESS;
}
