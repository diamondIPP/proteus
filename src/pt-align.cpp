#include <numeric>

#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>

#include "analyzers/correlation.h"
#include "mechanics/device.h"
#include "processors/applyalignment.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"
#include "processors/trackfinder.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(align)

/** Store sensor geometry parameter corrections for all steps. */
struct SensorCorrections {
  std::vector<double> offset0;
  std::vector<double> offset1;
  std::vector<double> offset2;
  std::vector<double> rotation0;
  std::vector<double> rotation1;
  std::vector<double> rotation2;

  void addStep(double off0,
               double off1,
               double off2,
               double rot0,
               double rot1,
               double rot2)
  {
    offset0.push_back(off0);
    offset1.push_back(off1);
    offset2.push_back(off2);
    rotation0.push_back(rot0);
    rotation1.push_back(rot1);
    rotation2.push_back(rot2);
  }
  void writeGraphs(const std::string& prefix, TDirectory* dir) const
  {
    auto makeGraph = [&](const std::string& name,
                         const std::vector<double>& y) {
      std::vector<double> x(y.size());
      std::iota(x.begin(), x.end(), 0);

      TGraph* g = new TGraph(x.size(), x.data(), y.data());
      g->SetName((prefix + "-Correction" + name).c_str());
      g->SetTitle("");
      g->GetXaxis()->SetTitle("Step");
      g->GetYaxis()->SetTitle(("Alignment Correction " + name).c_str());
      dir->WriteTObject(g);
    };

    makeGraph("Offset0", offset0);
    makeGraph("Offset1", offset1);
    makeGraph("Offset2", offset2);
    makeGraph("Rotation0", rotation0);
    makeGraph("Rotation1", rotation1);
    makeGraph("Rotation2", rotation2);
  }
};

struct Steps {
  std::map<Index, SensorCorrections> sensors;

  void addStep(Index sensorId,
               double off0,
               double off1,
               double off2,
               double rot0,
               double rot1,
               double rot2)
  {
    INFO("sensor ", sensorId, " alignment corrections:");
    INFO("  offset 0:  ", off0);
    INFO("  offset 1:  ", off1);
    INFO("  offset 2:  ", off2);
    INFO("  rotation 0:  ", rot0);
    INFO("  rotation 1:  ", rot1);
    INFO("  rotation 2:  ", rot2);
    sensors[sensorId].addStep(off0, off1, off2, rot0, rot1, rot2);
  }
  void writeGraphs(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
      Index sensorId = it->first;
      const auto& corrs = it->second;
      corrs.writeGraphs(device.getSensor(sensorId)->name(), dir);
    }
  }
};

int main(int argc, char const* argv[])
{
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Utils::Arguments args("align selected sensors");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  uint64_t skipEvents = args.get<uint64_t>("skip_events");
  uint64_t numEvents = args.get<uint64_t>("num_events");
  Device device = Device::fromFile(args.device());
  toml::Value cfg = Utils::Config::readConfig(args.config())["align"];

  // select alignment scope, i.e. the sensors that are considered as input and
  // the sensors that should be aligned
  auto sensorIds = cfg.get<std::vector<Index>>("sensor_ids");
  auto alignIds = cfg.get<std::vector<Index>>("align_ids");
  int numSteps = cfg.get<int>("num_steps");

  double distSigmaMax = 10.;

  Storage::StorageIO input(args.input(), Storage::INPUT, device.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");

  Steps corrections;

  for (int step = 0; step < numSteps; ++step) {
    TDirectory* stepDir = hists.mkdir(("Step" + std::to_string(step)).c_str());

    INFO("alignment step ", step, "/", numSteps);

    Utils::EventLoop loop(&input, skipEvents, numEvents);
    setupHitMappers(device, loop);
    setupClusterizers(device, loop);
    loop.addProcessor(std::make_shared<ApplyAlignment>(device));
    // coarse method w/o tracks using only cluster residuals

    auto corr = std::make_shared<Correlation>(device, sensorIds, stepDir);
    loop.addAnalyzer(corr);

    // loop.addProcessor(std::make_shared<StraightTrackFitter>(device, locals));
    // loop.addAnalyzer(std::make_shared<TrackInfo>(&device, &hists));
    // loop.addAnalyzer(std::make_shared<Residuals>(&device, &hists));
    // loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, &hists));
    loop.run();

    Alignment a = device.alignment();

    double deltaX = 0;
    double deltaY = 0;
    for (auto id = alignIds.begin(); id != alignIds.end(); ++id) {
      deltaX -= corr->getHistDiffX(*id - 1, *id)->GetMean();
      deltaY -= corr->getHistDiffY(*id - 1, *id)->GetMean();

      corrections.addStep(*id, deltaX, deltaY, 0, 0, 0, 0);
      a.correctOffset(*id, deltaX, deltaY, 0);
    }

    device.applyAlignment(a);
  }

  corrections.writeGraphs(device, &hists);

  device.alignment().writeFile(args.makeOutput("geo.toml"));
  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
