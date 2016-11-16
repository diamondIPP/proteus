#include <TFile.h>
#include <TTree.h>

#include "analyzers/correlation.h"
#include "mechanics/device.h"
#include "processors/applyalignment.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"
#include "processors/trackfinder.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(align)

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Utils::Arguments args("align selected sensors");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  uint64_t skipEvents = args.get<uint64_t>("skip_events");
  uint64_t numEvents = args.get<uint64_t>("num_events");
  Device device = Device::fromFile(args.device());
  toml::Value cfg = Utils::Config::readConfig(args.config())["align"];
  auto sensorIds = cfg.get<std::vector<Index>>("sensor_ids");
  auto ignoreIds = cfg.get<std::vector<Index>>("ignore_ids");
  double distSigmaMax = 10.;

  Storage::StorageIO input(args.input(), Storage::INPUT, device.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");

  for (size_t step = 0; step < 2; ++step) {
    TDirectory* stepDir = hists.mkdir(("Step" + std::to_string(step)).c_str());

    INFO("starting step ", step);

    Utils::EventLoop loop(&input, skipEvents, numEvents);
    setupHitMappers(device, loop);
    setupClusterizers(device, loop);
    loop.addProcessor(std::make_shared<ApplyAlignment>(device));
    // coarse method w/o tracks using only cluster residuals

    loop.addAnalyzer(std::make_shared<Correlation>(&device, stepDir));

    // loop.addProcessor(std::make_shared<StraightTrackFitter>(device, locals));
    // loop.addAnalyzer(std::make_shared<TrackInfo>(&device, &hists));
    // loop.addAnalyzer(std::make_shared<Residuals>(&device, &hists));
    // loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, &hists));
    loop.run();
  }

  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
