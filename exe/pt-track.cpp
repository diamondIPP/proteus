#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "analyzers/clusterinfo.h"
#include "analyzers/correlations.h"
#include "analyzers/eventinfo.h"
#include "analyzers/hitinfo.h"
#include "analyzers/residuals.h"
#include "analyzers/trackinfo.h"
#include "io/rceroot.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/setupsensors.h"
#include "storage/event.h"
#include "tracking/straightfitter.h"
#include "tracking/trackfinder.h"
#include "utils/application.h"
#include "utils/eventloop.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Processors;

  toml::Table defaults = {{"num_points_min", 3},
                          {"search_sigma_max", 5.},
                          {"reduced_chi2_max", -1.}};
  Utils::Application app("track", "preprocess, cluster, and track", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto numPointsMin = app.config().get<int>("num_points_min");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");

  auto loop = app.makeEventLoop();
  setupHitPreprocessing(app.device(), loop);
  setupClusterizers(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
      app.device(), sensorIds, numPointsMin, searchSigmaMax, redChi2Max));
  loop.addAnalyzer(std::make_shared<EventInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Hits>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Clusters>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Correlations>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<TrackInfo>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<Residuals>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(app.device(), &hists));
  loop.addWriter(std::make_shared<Io::RceRootWriter>(
      app.outputPath("data.root"), app.device().numSensors()));
  loop.run();

  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
