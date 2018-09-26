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

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Utils::Application app("match", "match tracks and clusters");
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");
  TFile trees(app.outputPath("trees.root").c_str(), "RECREATE");

  auto loop = app.makeEventLoop();
  setupHitPreprocessing(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  for (auto sensorId : sensorIds)
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
  loop.addAnalyzer(std::make_shared<Tracks>(&hists, app.device()));
  for (auto sensorId : sensorIds) {
    const auto& sensor = *app.device().getSensor(sensorId);
    loop.addAnalyzer(std::make_shared<Distances>(&hists, sensor));
    loop.addAnalyzer(std::make_shared<Matching>(&hists, sensor));
    loop.addAnalyzer(std::make_shared<Efficiency>(&hists, sensor));
    loop.addWriter(std::make_shared<Io::MatchWriter>(&trees, sensor));
  }
  loop.run();

  hists.Write();
  hists.Close();
  trees.Write();
  trees.Close();

  return EXIT_SUCCESS;
}
