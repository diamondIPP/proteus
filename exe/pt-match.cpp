// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include <Compression.h>
#include <TFile.h>
#include <TTree.h>

#include "analyzers/distances.h"
#include "analyzers/efficiency.h"
#include "analyzers/residuals.h"
#include "analyzers/tracks.h"
#include "io/match.h"
#include "io/rceroot.h"
#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/matcher.h"
#include "processors/setupsensors.h"
#include "storage/event.h"
#include "tracking/straightfitter.h"
#include "utils/application.h"
#include "utils/root.h"

int main(int argc, char const* argv[])
{
  using namespace proteus;

  Application app("match", "match tracks and clusters");
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  // output
  auto hists = openRootWrite(app.outputPath("hists.root"));
  auto trees = openRootWrite(app.outputPath("trees.root"));

  auto loop = app.makeEventLoop();
  setupPerSensorProcessing(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  for (auto sensorId : sensorIds)
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
  loop.addAnalyzer(std::make_shared<Tracks>(hists.get(), app.device()));
  for (auto sensorId : sensorIds) {
    const auto& sensor = app.device().getSensor(sensorId);
    loop.addAnalyzer(std::make_shared<Distances>(hists.get(), sensor));
    loop.addAnalyzer(std::make_shared<Matching>(hists.get(), sensor));
    loop.addAnalyzer(std::make_shared<Efficiency>(hists.get(), sensor));
    loop.addWriter(std::make_shared<MatchWriter>(trees.get(), sensor));
  }
  loop.run();

  return EXIT_SUCCESS;
}
