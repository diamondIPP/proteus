#include <TFile.h>
#include <TTree.h>

#include "analyzers/distances.h"
#include "analyzers/matchexporter.h"
#include "analyzers/residuals.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/hitmapper.h"
#include "processors/matcher.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Utils::Arguments args("match tracks and clusters");
  args.addOption('g', "geometry", "use a separate geometry file", "");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  uint64_t skipEvents = args.get<uint64_t>("skip_events");
  uint64_t numEvents = args.get<uint64_t>("num_events");
  Device device = Device::fromFile(args.device());
  // override geometry if requested
  auto geoPath = args.get<std::string>("geometry");
  if (!geoPath.empty())
    device.setGeometry(Mechanics::Geometry::fromFile(geoPath));

  toml::Value cfgAll = Utils::Config::readConfig(args.config());
  toml::Value cfg = cfgAll["match"];
  std::vector<Index> sensorIds = cfg.get<std::vector<Index>>("sensor_ids");

  Storage::StorageIO input(args.input(), Storage::INPUT, device.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");
  TFile trees(args.makeOutput("trees.root").c_str(), "RECREATE");

  Utils::EventLoop loop(&input, skipEvents, numEvents);
  setupHitMappers(device, loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(device));
  loop.addProcessor(std::make_shared<StraightTrackFitter>(device, sensorIds));
  for (auto sensorId : sensorIds)
    loop.addProcessor(std::make_shared<Matcher>(device, sensorId));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, &hists));
  for (auto sensorId : sensorIds) {
    loop.addAnalyzer(std::make_shared<Distances>(device, sensorId, &hists));
    loop.addAnalyzer(std::make_shared<MatchExporter>(device, sensorId, &trees));
  }
  loop.run();

  hists.Write();
  hists.Close();
  trees.Write();
  trees.Close();

  return EXIT_SUCCESS;
}
