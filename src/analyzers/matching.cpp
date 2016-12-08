#include "matching.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>

#include <TDirectory.h>
#include <TF1.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "processors/processors.h"
#include "storage/event.h"
#include "storage/plane.h"
#include "utils/root.h"

Analyzers::Distances::Hists::Hists()
    : deltaU(NULL), deltaV(NULL), dist(NULL), d2(NULL)
{
}

Analyzers::Distances::Hists::Hists(TDirectory* dir,
                                   std::string prefix,
                                   double rangeDist,
                                   double rangeD2,
                                   int numBins)
    : Hists()
{
  auto h1 = [&](std::string name, double x0, double x1) -> TH1D* {
    TH1D* h = new TH1D((prefix + name).c_str(), "", numBins, x0, x1);
    h->SetDirectory(dir);
    return h;
  };
  deltaU = h1("DeltaU", -rangeDist, rangeDist);
  deltaV = h1("DeltaV", -rangeDist, rangeDist);
  dist = h1("Dist", 0, rangeDist);
  if (0 < rangeD2)
    d2 = h1("D2", 0, rangeD2);
}

void Analyzers::Distances::Hists::fill(const XYVector& delta)
{
  deltaU->Fill(delta.x());
  deltaV->Fill(delta.y());
  dist->Fill(delta.r());
}
void Analyzers::Distances::Hists::fill(const XYVector& delta,
                                       const SymMatrix2& cov)
{
  fill(delta);
  d2->Fill(mahalanobisSquared(cov, delta));
}

Analyzers::Distances::Distances(const Mechanics::Device& device,
                                Index sensorId,
                                TDirectory* dir)
    : m_sensorId(sensorId)
{
  const Mechanics::Sensor& sensor = *device.getSensor(sensorId);
  auto area = sensor.sensitiveAreaLocal();
  double rangeTrack = std::hypot(area.axes[0].length(), area.axes[1].length());
  double rangeDist = std::hypot(sensor.pitchCol(), sensor.pitchRow());
  double rangeD2 = 10;
  int numBins = 256;
  TDirectory* sub = Utils::makeDir(dir, "Distances");

  m_trackTrack =
      Hists(sub, sensor.name() + "-TrackTrack-", rangeTrack, -1, numBins);
  m_trackCluster = Hists(sub, sensor.name() + "-TrackCluster-", 4 * rangeDist,
                         rangeD2, numBins);
  m_match =
      Hists(sub, sensor.name() + "-Match-", 1.5 * rangeDist, rangeD2, numBins);
}

std::string Analyzers::Distances::name() const
{
  return "Distances(" + std::to_string(m_sensorId) + ')';
}

void Analyzers::Distances::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  // combinatorics: all tracks to all other tracks
  for (Index istate0 = 0; istate0 < plane.numStates(); ++istate0) {
    for (Index istate1 = 0; istate1 < plane.numStates(); ++istate1) {
      if (istate0 == istate1)
        continue;
      m_trackTrack.fill(plane.getState(istate1).offset() -
                        plane.getState(istate0).offset());
    }
  }
  // combinatorics: all clusters to all tracks
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::TrackState& state = plane.getState(istate);
      const Storage::Cluster& cluster = *plane.getCluster(icluster);
      m_trackCluster.fill(cluster.posLocal() - state.offset(),
                          cluster.covLocal() + state.covOffset());
    }
  }
  // matched pairs
  for (Index istate = 0; istate < plane.numStates(); ++istate) {
    const Storage::TrackState& state = plane.getState(istate);
    if (state.matchedCluster()) {
      m_match.fill(state.matchedCluster()->posLocal() - state.offset(),
                   state.matchedCluster()->covLocal() + state.covOffset());
    }
  }
}

void Analyzers::Distances::finalize()
{
  // nothing to do
}

//=========================================================
Analyzers::Matching::Matching(const Mechanics::Device* refDevice,
                              const Mechanics::Device* dutDevice,
                              TDirectory* dir,
                              const char* suffix,
                              double pixelExtension,
                              double maxMatchingDist,
                              double sigmaBins,
                              unsigned int pixBinsX,
                              unsigned int pixBinsY)
    : DualAnalyzer(refDevice, dutDevice, dir, suffix), _plotDir(0)
{
  assert(refDevice && dutDevice &&
         "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  _plotDir = makeGetDirectory("Matching");

  std::stringstream name;  // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  unsigned int numBins = maxMatchingDist * sigmaBins;
  if (!numBins)
    numBins = 1;

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++) {
    const Mechanics::Sensor* sensor = _dutDevice->getSensor(nsens);

    name.str("");
    title.str("");
    name << sensor->getName() << "Distance" << _nameSuffix;
    title << sensor->getName() << " Matched Cluster Distance"
          << ";Radial distance [standard deviation]"
          << ";Tracks / " << std::setprecision(2)
          << maxMatchingDist / (double)numBins << " standard deviation";
    TH1D* matchDist = new TH1D(name.str().c_str(), title.str().c_str(), numBins,
                               0, maxMatchingDist);
    matchDist->SetDirectory(_plotDir);
    _matchDist.push_back(matchDist);

    const double maxDistX =
        maxMatchingDist *
        sqrt(pow(sensor->getPitchX() / sqrt(12.0), 2) +
             pow(_refDevice->getSensor(0)->getPitchX() / sqrt(12.0), 2));
    const double maxDistY =
        maxMatchingDist *
        sqrt(pow(sensor->getPitchY() / sqrt(12.0), 2) +
             pow(_refDevice->getSensor(0)->getPitchY() / sqrt(12.0), 2));

    name.str("");
    title.str("");
    name << sensor->getName() << "DistanceX" << _nameSuffix;
    title << sensor->getName() << " Matched Cluster Distance"
          << ";Distance in X [" << _dutDevice->getSpaceUnit() << "]"
          << ";Tracks / " << std::setprecision(2)
          << 2 * maxDistX / (2 * numBins) << " " << _dutDevice->getSpaceUnit();
    TH1D* matchDistX = new TH1D(name.str().c_str(), title.str().c_str(),
                                2 * numBins, -maxDistX, maxDistX);
    matchDistX->SetDirectory(_plotDir);
    _matchDistX.push_back(matchDistX);

    name.str("");
    title.str("");
    name << sensor->getName() << "DistanceY" << _nameSuffix;
    title << sensor->getName() << " Matched Cluster Distance"
          << ";Distance in Y [" << _dutDevice->getSpaceUnit() << "]"
          << ";Tracks / " << std::setprecision(2)
          << 2 * maxDistY / (2 * numBins) << " " << _dutDevice->getSpaceUnit();
    TH1D* matchDistY = new TH1D(name.str().c_str(), title.str().c_str(),
                                2 * numBins, -maxDistY, maxDistY);
    matchDistY->SetDirectory(_plotDir);
    _matchDistY.push_back(matchDistY);

    const unsigned int binsX = pixBinsX * pixelExtension;
    const unsigned int binsY = pixBinsY * pixelExtension;

    const double pixHalfWidth = sensor->getPitchX() / 2.0 * pixelExtension;
    const double pixHalfHeight = sensor->getPitchY() / 2.0 * pixelExtension;

    name.str("");
    title.str("");
    name << sensor->getName() << "InPixelTracks" << _nameSuffix;
    title << sensor->getName() << " In Pixel Track Occupancy"
          << ";X position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Tracks";
    TH2D* inPixTracks =
        new TH2D(name.str().c_str(), title.str().c_str(), binsX, -pixHalfWidth,
                 pixHalfWidth, binsY, -pixHalfHeight, pixHalfHeight);
    inPixTracks->SetDirectory(_plotDir);
    _inPixelTracks.push_back(inPixTracks);

    name.str("");
    title.str("");
    name << sensor->getName() << "InPixelTot" << _nameSuffix;
    title << sensor->getName() << " In Pixel Average Cluster ToT"
          << ";X position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Average Cluster ToT";
    TH2D* inPixTot =
        new TH2D(name.str().c_str(), title.str().c_str(), binsX, -pixHalfWidth,
                 pixHalfWidth, binsY, -pixHalfHeight, pixHalfHeight);
    inPixTot->SetDirectory(_plotDir);
    _inPixelTot.push_back(inPixTot);
  }
}

//=========================================================
void Analyzers::Matching::processEvent(const Storage::Event* refEvent,
                                       const Storage::Event* dutEvent)
{
  assert(refEvent && dutEvent && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(refEvent, dutEvent);

  // Check if the event passes the cuts
  if (!checkCuts(refEvent))
    return;

  /** Loop in all reconstructed tracls*/
  for (unsigned int ntrack = 0; ntrack < refEvent->getNumTracks(); ntrack++) {
    const Storage::Track* track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    if (!checkCuts(track))
      continue;

    // Make a list of the planes with their matched cluster
    std::vector<Storage::Cluster*> matches;
    for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
      matches.push_back(0); // No matches

    // Get the matches from the track
    // NOTE 2016-11-15 msmk: disabled for now due to incompatible api changes
    // for(unsigned int nmatch=0; nmatch<track->getNumMatchedClusters();
    // nmatch++){
    //   Storage::Cluster* cluster = track->getMatchedCluster(nmatch);
    //
    //   // Check if this cluster passes the cuts
    //   if (!checkCuts(cluster))
    //     continue;
    //
    //   matches.at(cluster->getPlane()->sensorId()) = cluster;
    // }

    for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++) {
      const Mechanics::Sensor* sensor = _dutDevice->getSensor(nsens);
      Storage::Cluster* match = matches.at(nsens);

      if (!match)
        continue;

      _matchDist.at(nsens)->Fill(match->getMatchDistance());

      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      const double distX = tx - match->getPosX();
      const double distY = ty - match->getPosY();

      _matchDistX.at(nsens)->Fill(distX);
      _matchDistY.at(nsens)->Fill(distY);

      double px = 0, py = 0;
      sensor->spaceToPixel(tx, ty, tz, px, py);

      // Find the distance of this track to all pixels in its matched cluster
      for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++) {
        const Storage::Hit* hit = match->getHit(nhit);
        const double hitX = hit->getPixX() + 0.5;
        const double hitY = hit->getPixY() + 0.5;

        const double pixDistX = sensor->getPitchX() * (px - hitX);
        const double pixDistY = sensor->getPitchY() * (py - hitY);

        _inPixelTracks.at(nsens)->Fill(pixDistX, pixDistY);
        _inPixelTot.at(nsens)->Fill(pixDistX, pixDistY, hit->getValue());
      }
    } // loop in sensors

  } // end loop in tracks
}

//=========================================================
void Analyzers::Matching::postProcessing()
{
  for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++) {
    const Mechanics::Sensor* sensor = _dutDevice->getSensor(nsens);

    TH2D* tot = _inPixelTot.at(nsens);
    TH2D* hits = _inPixelTracks.at(nsens);

    assert(hits->GetNbinsX() == tot->GetNbinsX() &&
           hits->GetNbinsY() == tot->GetNbinsY() &&
           "Matching: histograms should have the same number of bins");

    for (int nx = 1; nx <= tot->GetNbinsX(); nx++) {
      for (int ny = 1; ny <= tot->GetNbinsY(); ny++) {
        const unsigned int num = hits->GetBinContent(nx, ny);
        if (num == 0)
          continue;
        const double average = tot->GetBinContent(nx, ny) / (double)num;
        tot->SetBinContent(nx, ny, average);
      }
    }

    for (unsigned int axis = 0; axis < 2; axis++) {
      TH1D* dist = axis ? _matchDistX.at(nsens) : _matchDistY.at(nsens);

      double pixelWidth = axis ? sensor->getPitchX() : sensor->getPitchY();
      double beamSigma = axis ? _refDevice->getSensor(0)->getPitchX()
                              : _refDevice->getSensor(0)->getPitchY();

      TF1* fit = Processors::fitPixelBeam(dist, pixelWidth, beamSigma, false);

      std::stringstream ss;
      ss << _dutDevice->getName() << sensor->getName() << "PixelBeamFit"
         << (axis ? "X" : "Y") << _nameSuffix;
      _plotDir->WriteObject(fit, ss.str().c_str());
    }
  }
}
