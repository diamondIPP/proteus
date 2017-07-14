#include <TFile.h>
#include <TTree.h>

#include "analyzers/basicefficiency.h"
#include "analyzers/distances.h"
#include "analyzers/residuals.h"
#include "analyzers/trackinfo.h"
#include "io/match.h"
#include "io/rceroot.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/matcher.h"
#include "processors/setupsensors.h"
#include "storage/event.h"
#include "tracking/straightfitter.h"
#include "utils/application.h"
#include "utils/eventloop.h"

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
  loop.addProcessor(
      std::make_shared<Tracking::StraightFitter>(app.device(), sensorIds));
  for (auto sensorId : sensorIds)
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
  loop.addAnalyzer(std::make_shared<Tracks>(&app.device(), &hists));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(app.device(), &hists));
  for (auto sensorId : sensorIds) {
    loop.addAnalyzer(
        std::make_shared<Distances>(app.device(), sensorId, &hists));
    loop.addAnalyzer(std::make_shared<BasicEfficiency>(
        *app.device().getSensor(sensorId), &hists));
    loop.addWriter(
        std::make_shared<Io::MatchWriter>(app.device(), sensorId, &trees));
  }
  loop.run();

  hists.Write();
  hists.Close();
  trees.Write();
  trees.Close();

  return EXIT_SUCCESS;
}
