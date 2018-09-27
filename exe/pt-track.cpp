#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "analyzers/clusters.h"
#include "analyzers/correlations.h"
#include "analyzers/globaloccupancy.h"
#include "analyzers/hits.h"
#include "analyzers/residuals.h"
#include "analyzers/tracks.h"
#include "io/rceroot.h"
#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/setupsensors.h"
#include "storage/event.h"
#include "tracking/straightfitter.h"
#include "tracking/trackfinder.h"
#include "utils/application.h"

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
  loop.addProcessor(std::make_shared<Tracking::StraightFitter>(app.device()));
  loop.addAnalyzer(std::make_shared<Hits>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<Clusters>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<GlobalOccupancy>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<Correlations>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<Tracks>(&hists, app.device()));
  loop.addAnalyzer(
      std::make_shared<Residuals>(&hists, app.device(), sensorIds));
  loop.addWriter(std::make_shared<Io::RceRootWriter>(
      app.outputPath("data.root"), app.device().numSensors()));
  loop.run();

  hists.Write(nullptr, TFile::kOverwrite);
  hists.Close();

  return EXIT_SUCCESS;
}
