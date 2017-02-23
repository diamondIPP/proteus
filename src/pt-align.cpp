#include <algorithm>
#include <iterator>
#include <numeric>

#include <TFile.h>
#include <TGraphErrors.h>
#include <TTree.h>

#include "alignment/correlationsaligner.h"
#include "alignment/residualsaligner.h"
#include "analyzers/correlations.h"
#include "analyzers/residuals.h"
#include "analyzers/trackinfo.h"
#include "application.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/clusterizer.h"
#include "processors/hitmapper.h"
#include "processors/trackfinder.h"
#include "processors/trackfitter.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(align)

/** Store sensor geometry parameters for multiple steps. */
struct SensorStepsGraphs {
  std::vector<double> off0;
  std::vector<double> off1;
  std::vector<double> off2;
  std::vector<double> rot0;
  std::vector<double> rot1;
  std::vector<double> rot2;
  std::vector<double> errOff0;
  std::vector<double> errOff1;
  std::vector<double> errOff2;
  std::vector<double> errRot0;
  std::vector<double> errRot1;
  std::vector<double> errRot2;

  void addStep(const Vector6& delta, const SymMatrix6& cov)
  {
    off0.push_back(delta[0]);
    off1.push_back(delta[1]);
    off2.push_back(delta[2]);
    rot0.push_back(delta[3]);
    rot1.push_back(delta[4]);
    rot2.push_back(delta[5]);
    // errors
    errOff0.push_back(std::sqrt(cov(0, 0)));
    errOff1.push_back(std::sqrt(cov(1, 1)));
    errOff2.push_back(std::sqrt(cov(2, 2)));
    errRot0.push_back(std::sqrt(cov(3, 3)));
    errRot1.push_back(std::sqrt(cov(4, 4)));
    errRot2.push_back(std::sqrt(cov(5, 5)));
  }
  void writeGraphs(const std::string& sensorName, TDirectory* dir) const
  {
    auto makeGraph = [&](const std::string& paramName,
                         const std::vector<double>& yval,
                         const std::vector<double>& yerr) {
      std::vector<double> x(yval.size());
      std::iota(x.begin(), x.end(), 0);
      TGraphErrors* g =
          new TGraphErrors(x.size(), x.data(), yval.data(), NULL, yerr.data());
      g->SetName((sensorName + "-" + paramName).c_str());
      g->SetTitle("");
      g->GetXaxis()->SetTitle("Alignment step");
      g->GetYaxis()->SetTitle(
          (sensorName + " alignment correction " + paramName).c_str());
      dir->WriteTObject(g);
    };
    makeGraph("Offset0", off0, errOff0);
    makeGraph("Offset1", off1, errOff1);
    makeGraph("Offset2", off2, errOff2);
    makeGraph("Rotation0", rot0, errRot0);
    makeGraph("Rotation1", rot1, errRot1);
    makeGraph("Rotation2", rot2, errRot2);
  }
};

struct StepsGraphs {
  std::map<Index, SensorStepsGraphs> graphs;

  void addStep(const std::vector<Index>& sensorIds,
               const Mechanics::Geometry& geo)
  {
    for (auto id : sensorIds)
      graphs[id].addStep(geo.getParams(id), geo.getParamsCov(id));
  }
  void writeGraphs(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (const auto& g : graphs)
      g.second.writeGraphs(device.getSensor(g.first)->name(), dir);
  }
};

enum class Method { Correlations, Residuals };

Method stringToMethod(const std::string& name)
{
  if (name == "correlations") {
    return Method::Correlations;
  } else if (name == "residuals") {
    return Method::Residuals;
  } else {
    throw std::runtime_error("unknown alignment method '" + name + '\'');
  }
}

int main(int argc, char const* argv[])
{
  using namespace Alignment;
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  toml::Table defaults = {{"num_steps", 1},
                          {"search_sigma_max", 5.},
                          {"reduced_chi2_max", -1.},
                          {"damping", 0.9}};
  Application app("align", "align selected sensors", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto alignIds = app.config().get<std::vector<Index>>("align_ids");
  auto method = stringToMethod(app.config().get<std::string>("method"));
  auto numSteps = app.config().get<int>("num_steps");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");
  auto damping = app.config().get<double>("damping");

  // check sensor selection
  std::vector<Index> sortedSensorIds = sortedByZ(app.device(), sensorIds);
  std::vector<Index> sortedAlignIds = sortedByZ(app.device(), alignIds);
  std::vector<Index> fixedSensorIds;
  // all sensors not in the align set are kept fixed
  std::set_difference(sortedSensorIds.begin(), sortedSensorIds.end(),
                      sortedAlignIds.begin(), sortedAlignIds.end(),
                      std::back_inserter(fixedSensorIds),
                      Mechanics::CompareSensorIdZ{app.device()});
  INFO("fixed sensors: ", fixedSensorIds);
  INFO("align sensors: ", sortedAlignIds);
  if (!std::includes(sortedSensorIds.begin(), sortedSensorIds.end(),
                     sortedAlignIds.begin(), sortedAlignIds.end(),
                     Mechanics::CompareSensorIdZ{app.device()})) {
    ERROR("set of align sensor is not a subset of the input sensor set");
    return EXIT_FAILURE;
  }
  if (fixedSensorIds.empty()) {
    ERROR("no fixed sensors are given");
    return EXIT_FAILURE;
  }

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");
  StepsGraphs steps;
  // add initial geometry to alignment graphs
  steps.addStep(alignIds, app.device().geometry());

  // copy device to allow modifications after each alignment step
  auto dev = app.device();

  for (int step = 1; step <= numSteps; ++step) {
    TDirectory* stepDir = hists.mkdir(("Step" + std::to_string(step)).c_str());

    INFO("alignment step ", step, "/", numSteps);

    // common event loop elements for all alignment methods
    auto loop = app.makeEventLoop();
    setupHitMappers(dev, loop);
    setupClusterizers(dev, loop);
    loop.addProcessor(std::make_shared<ApplyGeometry>(dev));

    // setup aligment method specific loop logic
    std::shared_ptr<Aligner> aligner;
    if (method == Method::Correlations) {
      // coarse method w/o tracks using only cluster correlations
      // use the first sensor that is not in the align set as reference
      aligner = std::make_shared<CorrelationsAligner>(
          dev, fixedSensorIds.front(), alignIds, stepDir);

    } else if (method == Method::Residuals) {
      // use (unbiased) track residuals to align
      loop.addProcessor(std::make_shared<TrackFinder>(
          dev, sensorIds, searchSigmaMax, sensorIds.size(), redChi2Max));
      loop.addAnalyzer(std::make_shared<TrackInfo>(&dev, stepDir));
      loop.addAnalyzer(std::make_shared<Residuals>(&dev, stepDir));
      aligner =
          std::make_shared<ResidualsAligner>(dev, alignIds, stepDir, damping);
    }
    loop.addAnalyzer(aligner);
    loop.run();

    Mechanics::Geometry newGeo = aligner->updatedGeometry();
    steps.addStep(alignIds, newGeo);
    dev.setGeometry(newGeo);
  }

  dev.geometry().writeFile(app.outputPath("geo.toml"));
  steps.writeGraphs(dev, &hists);
  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
