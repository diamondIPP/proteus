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
#include "application.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/clusterizer.h"
#include "processors/clustermaker.h"
#include "processors/hitmapper.h"
#include "processors/trackfinder.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/eventloop.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Processors;

  toml::Table defaults = {{"distance_sigma_max", 5.},
                          {"num_points_min", 3},
                          {"reduced_chi2_max", -1.}};
  Application app("track", "preprocess, cluster, and track", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto distSigmaMax = app.config().get<double>("distance_sigma_max");
  auto numPointsMin = app.config().get<int>("num_points_min");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");

  // output
  Storage::StorageIO output(app.outputPath("data.root"), Storage::OUTPUT,
                            app.device().numSensors());
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");

  auto loop = app.makeEventLoop();
  loop.setOutput(&output);
  setupHitMappers(app.device(), loop);
  setupClusterizers(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  loop.addProcessor(std::make_shared<TrackFinder>(
      app.device(), sensorIds, distSigmaMax, numPointsMin, redChi2Max));
  loop.addAnalyzer(std::make_shared<EventInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<HitInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<ClusterInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<TrackInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Occupancy>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Correlation>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Residuals>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(app.device(), &hists));
  loop.run();

  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
