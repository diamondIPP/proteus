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
#include "processors/trackfitter.h"
#include "processors/trackmaker.h"
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

  auto device = Mechanics::Device::fromFile(args.device());
  Storage::StorageIO events(args.input(), Storage::INPUT);
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");

  std::vector<int> locals = {0, 6, 7};

  Utils::EventLoop loop(&events,
                        args.get<uint64_t>("skip_events"),
                        args.get<uint64_t>("num_events"));
  setupHitMapper(device, loop);
  setupClusterizer(device, loop);
  loop.addProcessor(std::make_shared<ApplyAlignment>(device));
  loop.addProcessor(std::make_shared<TrackMaker>(10));
  loop.addProcessor(std::make_shared<StraightTrackFitter>(device, locals));
  loop.addAnalyzer(std::make_shared<EventInfo>(&device, &hists));
  loop.addAnalyzer(std::make_shared<HitInfo>(&device, &hists));
  loop.addAnalyzer(std::make_shared<ClusterInfo>(&device, &hists));
  loop.addAnalyzer(std::make_shared<TrackInfo>(&device, &hists));
  loop.addAnalyzer(std::make_shared<Occupancy>(&device, &hists));
  loop.addAnalyzer(std::make_shared<Correlation>(&device, &hists));
  loop.addAnalyzer(std::make_shared<Residuals>(&device, &hists));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, &hists));
  loop.run();

  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
