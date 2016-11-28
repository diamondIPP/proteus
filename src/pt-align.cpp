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

/** Map radian value into equivalent value in [-pi, pi) range. */
inline double radian_sym(double val)
{
  if (val < -M_PI) {
    return val + 2 * M_PI;
  } else if (M_PI < val) {
    return val - 2 * M_PI;
  } else {
    return val;
  }
}

/** Store sensor geometry parameter corrections for multiple steps. */
struct SensorStepsGraphs {
  std::vector<double> offset0;
  std::vector<double> offset1;
  std::vector<double> offset2;
  std::vector<double> rotation0;
  std::vector<double> rotation1;
  std::vector<double> rotation2;

  void addStep(const Vector6& delta)
  {
    offset0.push_back(delta[0]);
    offset1.push_back(delta[1]);
    offset2.push_back(delta[2]);
    rotation0.push_back(radian_sym(delta[3]));
    rotation1.push_back(radian_sym(delta[4]));
    rotation2.push_back(radian_sym(delta[5]));
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

struct StepsGraphs {
  std::map<Index, SensorStepsGraphs> sensors;

  void addStep(const std::vector<Index>& sensorIds,
               const Mechanics::Alignment& before,
               const Mechanics::Alignment& after)
  {
    for (auto id = sensorIds.begin(); id != sensorIds.end(); ++id) {
      Vector6 delta = after.getParams(*id) - before.getParams(*id);
      sensors[*id].addStep(delta);
    }
  }
  void writeGraphs(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (auto it = sensors.begin(); it != sensors.end(); ++it)
      it->second.writeGraphs(device.getSensor(it->first)->name(), dir);
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
    device.setGeometry(geo);
  }

  toml::Value cfgAll = Utils::Config::readConfig(args.config());
  toml::Value defaults = toml::Table{
      {"num_steps", 10}, {"distance_sigma_max", 5.}, {"reduced_chi2_max", -1.}};
  toml::Value cfg = Utils::Config::withDefaults(cfgAll["align"], defaults);
  // select alignment scope, i.e. the sensors that are considered as input and
  // the sensors that should be aligned
  auto sensorIds = cfg.get<std::vector<Index>>("sensor_ids");
  auto alignIds = cfg.get<std::vector<Index>>("align_ids");
  auto methodName = cfg.get<std::string>("method");
  auto numSteps = cfg.get<int>("num_steps");
  double distSigmaMax = cfg.get<double>("distance_sigma_max");
  double redChi2Max = cfg.get<double>("reduced_chi2_max");

  Method method = Method::CORRELATION;
  if (methodName == "correlation") {
    method = Method::CORRELATION;
    numSteps = 1;
  } else if (methodName == "residuals") {
    method = Method::RESIDUALS;
  } else {
    throw std::runtime_error("unknown alignment method '" + methodName + '\'');
  }

  Storage::StorageIO input(args.input(), Storage::INPUT, device.numSensors());
  TFile hists(args.makeOutput("hists.root").c_str(), "RECREATE");
  StepsGraphs steps;

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
      // use (unbiased) track residuals to align

      loop.addProcessor(std::make_shared<TrackFinder>(
          device, sensorIds, distSigmaMax, sensorIds.size(), redChi2Max));
      loop.addAnalyzer(std::make_shared<TrackInfo>(&device, stepDir));
      loop.addAnalyzer(std::make_shared<Residuals>(&device, stepDir));
      loop.addAnalyzer(std::make_shared<UnbiasedResiduals>(device, stepDir));
      aligner = std::make_shared<ResidualsAligner>(device, alignIds, stepDir);
      loop.addAnalyzer(aligner);
    }

    loop.run();

    Mechanics::Alignment newGeo = aligner->updatedGeometry();
    steps.addStep(alignIds, device.geometry(), newGeo);
    device.setGeometry(newGeo);
  }

  // corrections.writeGraphs(device, &hists);

  device.geometry().writeFile(args.makeOutput("geo.toml"));
  steps.writeGraphs(device, &hists);
  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
