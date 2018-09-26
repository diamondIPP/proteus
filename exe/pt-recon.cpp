#include <TFile.h>
#include <TTree.h>

#include "analyzers/clusters.h"
#include "analyzers/correlations.h"
#include "analyzers/distances.h"
#include "analyzers/efficiency.h"
#include "analyzers/globaloccupancy.h"
#include "analyzers/hits.h"
#include "analyzers/residuals.h"
#include "analyzers/tracks.h"
#include "io/match.h"
#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/matcher.h"
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
  Utils::Application app("recon", "preprocess, cluster, and track", defaults);
  app.initialize(argc, argv);

  // configuration
  auto trackingIds = app.config().get<std::vector<Index>>("tracking_ids");
  auto extrapolationIds =
      app.config().get<std::vector<Index>>("extrapolation_ids");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto numPointsMin = app.config().get<int>("num_points_min");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");
  TFile trees(app.outputPath("trees.root").c_str(), "RECREATE");

  auto loop = app.makeEventLoop();

  // per-sensor processing
  setupHitPreprocessing(app.device(), loop);
  setupClusterizers(app.device(), loop);
  loop.addAnalyzer(std::make_shared<Hits>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<Clusters>(&hists, app.device()));
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));

  // geometry analyzers
  loop.addAnalyzer(std::make_shared<GlobalOccupancy>(&hists, app.device()));
  loop.addAnalyzer(std::make_shared<Correlations>(&hists, app.device()));

  // tracking
  loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
      app.device(), trackingIds, numPointsMin, searchSigmaMax, redChi2Max));
  loop.addProcessor(std::make_shared<Tracking::StraightFitter>(app.device()));
  loop.addAnalyzer(std::make_shared<Tracks>(&hists, app.device()));
  loop.addAnalyzer(
      std::make_shared<Residuals>(&hists, app.device(), trackingIds));

  // matching
  for (auto sensorId : extrapolationIds) {
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
    const auto& sensor = *app.device().getSensor(sensorId);
    loop.addAnalyzer(std::make_shared<Distances>(&hists, sensor));
    loop.addAnalyzer(std::make_shared<Matching>(&hists, sensor));
    loop.addAnalyzer(std::make_shared<Efficiency>(&hists, sensor));
    loop.addWriter(std::make_shared<Io::MatchWriter>(&trees, sensor));
  }

  loop.run();

  hists.Write(nullptr, TFile::kOverwrite);
  hists.Close();
  trees.Write(nullptr, TFile::kOverwrite);
  trees.Close();

  return EXIT_SUCCESS;
}
