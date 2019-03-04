#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <Compression.h>
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
#include "tracking/setupfitter.h"
#include "tracking/trackfinder.h"
#include "utils/application.h"
#include "utils/root.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Processors;
  using namespace Tracking;

  toml::Table defaults = {
      {"search_sigma_max", 5.},
      {"num_points_min", 3},
      {"reduced_chi2_max", -1.},
      {"track_fitter", "straight3d"},
  };
  Utils::Application app("track", "preprocess, cluster, and track", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto numPointsMin = app.config().get<int>("num_points_min");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");
  auto fitter = app.config().get<std::string>("track_fitter");
  // output
  auto hists = Utils::openRootWrite(app.outputPath("hists.root"));

  auto loop = app.makeEventLoop();

  // per-sensor processing
  setupHitPreprocessing(app.device(), loop);
  setupClusterizers(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  loop.addAnalyzer(std::make_shared<Hits>(hists.get(), app.device()));
  loop.addAnalyzer(std::make_shared<Clusters>(hists.get(), app.device()));

  // geometry analyzers
  loop.addAnalyzer(
      std::make_shared<GlobalOccupancy>(hists.get(), app.device()));
  loop.addAnalyzer(std::make_shared<Correlations>(hists.get(), app.device()));

  // tracking
  loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
      app.device(), sensorIds, numPointsMin, searchSigmaMax, redChi2Max));
  setupUnbiasedTrackFitter(app.device(), fitter, loop);
  loop.addAnalyzer(std::make_shared<Tracks>(hists.get(), app.device()));
  loop.addAnalyzer(
      std::make_shared<Residuals>(hists.get(), app.device(), sensorIds));

  // data writing
  loop.addWriter(std::make_shared<Io::RceRootWriter>(
      app.outputPath("data.root"), app.device().numSensors()));

  loop.run();

  return EXIT_SUCCESS;
}
