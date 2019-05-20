// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

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
  auto sensorIds = cfg.get<std::vector<Index>>("sensor_ids");
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
      app.device(), sensorIds, searchSpatialSigmaMax, searchTemporalSigmaMax,
      numPointsMin, redChi2Max));
  setupTrackFitter(app.device(), fitter, loop);
  loop.addAnalyzer(std::make_shared<Tracks>(hists.get(), app.device()));
  loop.addAnalyzer(
      std::make_shared<Residuals>(hists.get(), app.device(), sensorIds));

  // data writing
  loop.addWriter(std::make_shared<RceRootWriter>(app.outputPath("data.root"),
                                                 app.device().numSensors()));

  loop.run();

  return EXIT_SUCCESS;
}
