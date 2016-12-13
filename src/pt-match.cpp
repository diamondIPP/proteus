#include <TFile.h>
#include <TTree.h>

#include "analyzers/distances.h"
#include "analyzers/matchexporter.h"
#include "analyzers/residuals.h"
#include "application.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/hitmapper.h"
#include "processors/matcher.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/eventloop.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Application app("match", "match tracks and clusters", {});
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");
  TFile trees(app.outputPath("trees.root").c_str(), "RECREATE");

  auto loop = app.makeEventLoop();
  setupHitMappers(app.device(), loop);
  loop.addProcessor(std::make_shared<ApplyGeometry>(app.device()));
  loop.addProcessor(
      std::make_shared<StraightTrackFitter>(app.device(), sensorIds));
  for (auto sensorId : sensorIds)
    loop.addProcessor(std::make_shared<Matcher>(app.device(), sensorId));
  loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(app.device(), &hists));
  for (auto sensorId : sensorIds) {
    loop.addAnalyzer(
        std::make_shared<Distances>(app.device(), sensorId, &hists));
    loop.addAnalyzer(
        std::make_shared<MatchExporter>(app.device(), sensorId, &trees));
  }
  loop.run();

  hists.Write();
  hists.Close();
  trees.Write();
  trees.Close();

  return EXIT_SUCCESS;
}
