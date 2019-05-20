// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include <Compression.h>
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
#include "tracking/setupfitter.h"
#include "tracking/trackfinder.h"
#include "utils/application.h"
#include "utils/logger.h"
#include "utils/root.h"

int main(int argc, char const* argv[])
{
  using namespace proteus;

  toml::Table defaults = {
      {"search_spatial_sigma_max", 5.},
      // disabled by default for backward compatibility
      {"search_temporal_sigma_max", -1.},
      {"num_points_min", 3},
      {"reduced_chi2_max", -1.},
      {"track_fitter", "straight3d"},
  };
  Application app("recon", "preprocess, cluster, and track", defaults);
  app.initialize(argc, argv);

  // configuration
  const auto& cfg = app.config();
  auto trackingIds = cfg.get<std::vector<Index>>("tracking_ids");
  auto extrapolationIds = cfg.get<std::vector<Index>>("extrapolation_ids");
  auto searchSpatialSigmaMax = cfg.get<double>("search_spatial_sigma_max");
  if (cfg.has("search_sigma_max")) {
    WARN("The `search_sigma_max` setting is deprecated. Use "
         "`search_spatial_sigma_max` instead.");
    searchSpatialSigmaMax = cfg.get<double>("search_sigma_max");
  }
  auto searchTemporalSigmaMax = cfg.get<double>("search_temporal_sigma_max");
  auto numPointsMin = cfg.get<int>("num_points_min");
  auto redChi2Max = cfg.get<double>("reduced_chi2_max");
  auto fitter = cfg.get<std::string>("track_fitter");

  // output
  auto hists = openRootWrite(app.outputPath("hists.root"));
  auto trees = openRootWrite(app.outputPath("trees.root"));

  auto loop = app.makeEventLoop();

  // local per-sensor processing
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
  loop.addProcessor(std::make_shared<TrackFinder>(
      app.device(), trackingIds, searchSpatialSigmaMax, searchTemporalSigmaMax,
      numPointsMin, redChi2Max));
  setupTrackFitter(app.device(), fitter, loop);
  loop.addAnalyzer(std::make_shared<Tracks>(hists.get(), app.device()));
  loop.addAnalyzer(
      std::make_shared<Residuals>(hists.get(), app.device(), trackingIds));

  // matching
  for (auto sensorId : extrapolationIds) {
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
    const auto& sensor = app.device().getSensor(sensorId);
    loop.addAnalyzer(std::make_shared<Distances>(hists.get(), sensor));
    loop.addAnalyzer(std::make_shared<Matching>(hists.get(), sensor));
    loop.addAnalyzer(std::make_shared<Efficiency>(hists.get(), sensor));
    loop.addWriter(std::make_shared<MatchWriter>(trees.get(), sensor));
  }

  loop.run();

  return EXIT_SUCCESS;
}
