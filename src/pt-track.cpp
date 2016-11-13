#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "analyzers/clusterinfo.h"
#include "analyzers/correlation.h"
#include "analyzers/eventinfo.h"
#include "analyzers/hitinfo.h"
#include "analyzers/occupancy.h"
#include "analyzers/residuals.h"
#include "analyzers/trackinfo.h"
#include "mechanics/device.h"
#include "processors/applyalignment.h"
#include "processors/clusterizer.h"
#include "processors/clustermaker.h"
#include "processors/hitmapper.h"
#include "processors/trackfinder.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Processors;

  Utils::Arguments args("run proteus telescope processing");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  // setup configuration
  uint64_t skipEvents = args.get<uint64_t>("skip_events");
  uint64_t numEvents = args.get<uint64_t>("num_events");

  Mechanics::Device dev = Mechanics::Device::fromFile(args.device());

  toml::Value cfg = Utils::Config::readConfig(args.config());
  toml::Value cfgTrk = cfg["track"];
  auto sensorIds = cfgTrk.get<std::vector<Index>>("sensor_ids");
  auto numPointsMin = cfgTrk.get<int>("num_points_min");
  auto distSigmaMax = cfgTrk.get<double>("distance_sigma_max");

  Storage::StorageIO input(args.input(), Storage::INPUT, dev.numSensors());
  Storage::StorageIO output(args.makeOutput("data.root"), Storage::OUTPUT,
                            dev.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");

  Utils::EventLoop loop(&input, &output, skipEvents, numEvents);
  setupHitMappers(dev, loop);
  setupClusterizers(dev, loop);
  loop.addProcessor(std::make_shared<ApplyAlignment>(dev));
  loop.addProcessor(std::make_shared<TrackFinder>(dev, sensorIds, numPointsMin,
                                                  distSigmaMax));
  loop.addAnalyzer(std::make_shared<EventInfo>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<HitInfo>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<ClusterInfo>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<TrackInfo>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<Occupancy>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<Correlation>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<Residuals>(&dev, &hists));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(dev, &hists));
  loop.run();

  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
