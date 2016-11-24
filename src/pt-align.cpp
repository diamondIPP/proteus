#include <numeric>

#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>

#include "alignment/correlationaligner.h"
#include "alignment/residualsaligner.h"
#include "analyzers/correlation.h"
#include "analyzers/residuals.h"
#include "analyzers/trackinfo.h"
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

enum class Method { CORRELATION, RESIDUALS };

int main(int argc, char const* argv[])
{
  using namespace Alignment;
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  Utils::Arguments args("align selected sensors");
  args.addOption('g', "geometry", "use a separate geometry file", "");

  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  uint64_t skipEvents = args.get<uint64_t>("skip_events");
  uint64_t numEvents = args.get<uint64_t>("num_events");
  Device device = Device::fromFile(args.device());
  // override geometry if requested
  auto geoPath = args.get<std::string>("geometry");
  if (!geoPath.empty()) {
    auto geo = Mechanics::Alignment::fromFile(geoPath);
    device.applyAlignment(geo);
  }

  toml::Value cfg = Utils::Config::readConfig(args.config())["align"];

  // select alignment scope, i.e. the sensors that are considered as input and
  // the sensors that should be aligned
  auto sensorIds = cfg.get<std::vector<Index>>("sensor_ids");
  auto alignIds = cfg.get<std::vector<Index>>("align_ids");
  auto methodName = cfg.get<std::string>("method");
  Method method = Method::CORRELATION;
  int numSteps = 0;
  double distSigmaMax = -1;
  double redChi2Max = -1;

  if (methodName == "correlation") {
    method = Method::CORRELATION;
    numSteps = 1;
  } else if (methodName == "residuals") {
    method = Method::RESIDUALS;
    numSteps = cfg.get<int>("num_steps");
    distSigmaMax = cfg.get<double>("distance_sigma_max");
    redChi2Max = cfg.get<double>("reduced_chi2_max");
  } else {
    throw std::runtime_error("alignment method '" + methodName +
                             "' is unknown");
  }

  Storage::StorageIO input(args.input(), Storage::INPUT, device.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");
  // Steps corrections;

  for (int step = 0; step < numSteps; ++step) {
    TDirectory* stepDir = hists.mkdir(("Step" + std::to_string(step)).c_str());

    INFO("alignment step ", step + 1, "/", numSteps);

    // common event loop elements for all alignment methods
    Utils::EventLoop loop(&input, skipEvents, numEvents);
    setupHitMappers(device, loop);
    setupClusterizers(device, loop);
    loop.addProcessor(std::make_shared<ApplyAlignment>(device));
    // setup aligment method specific loop logic
    std::shared_ptr<Aligner> aligner;
    if (method == Method::CORRELATION) {
      // coarse method w/o tracks using only cluster correlations

      auto corr = std::make_shared<Correlation>(device, sensorIds, stepDir);
      aligner = std::make_shared<CorrelationAligner>(device, alignIds, corr);
      loop.addAnalyzer(corr);
      loop.addAnalyzer(aligner);

    } else if (method == Method::RESIDUALS) {
      // use unbiased residuals of tracks to align

      loop.addProcessor(std::make_shared<TrackFinder>(
          device, sensorIds, sensorIds.size(), distSigmaMax));
      loop.addAnalyzer(std::make_shared<TrackInfo>(&device, stepDir));
      loop.addAnalyzer(std::make_shared<Residuals>(&device, stepDir));
      loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, stepDir));
      aligner = std::make_shared<ResidualsAligner>(device, alignIds, stepDir);
      loop.addAnalyzer(aligner);
    }

    loop.run();
    device.applyAlignment(aligner->updatedGeometry());
  }

  // corrections.writeGraphs(device, &hists);

  device.alignment().writeFile(args.makeOutput("geo.toml"));
  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
