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
#include "mechanics/sensor.h"
#include "processors/processors.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "storage/track.h"
#include "utils/root.h"

Analyzers::Distances::Distances(const Mechanics::Device& device,
                                Index sensorId,
                                TDirectory* dir)
    : m_sensorId(sensorId)
{
  const Mechanics::Sensor& sensor = *device.getSensor(sensorId);
  TDirectory* sub = Utils::makeDir(dir, "Distances");
  int numBins = 100;
  double rangeU = 2 * sensor.pitchCol();
  double rangeV = 2 * sensor.pitchRow();
  double rangeDist = std::max(rangeU, rangeV);
  double rangeMahalanobis = 10;
  double rangeTrackU = sensor.sensitiveAreaLocal().axes[0].length() / 2;
  double rangeTrackV = sensor.sensitiveAreaLocal().axes[1].length() / 2;
  double rangeTrackDist = std::hypot(rangeTrackU, rangeTrackV);

  auto makeH1 = [&](const char* name, double low, double high) -> TH1D* {
    TH1D* h =
        new TH1D((sensor.name() + "-" + name).c_str(), "", numBins, low, high);
    h->SetDirectory(sub);
    return h;
  };
  m_allDistU = makeH1("AllDistU", -rangeU, rangeU);
  m_allDistV = makeH1("AllDistV", -rangeV, rangeV);
  m_allDist = makeH1("AllDist", 0, rangeDist);
  m_allMahalanobis = makeH1("AllMahalanobis", 0, rangeMahalanobis);
  m_matchDistU = makeH1("MatchDistU", -rangeU, rangeU);
  m_matchDistV = makeH1("MatchDistV", -rangeV, rangeV);
  m_matchDist = makeH1("MatchDist", 0, rangeDist);
  m_matchMahalanobis = makeH1("MatchMahalanobis", 0, rangeMahalanobis);
  m_trackDistU = makeH1("TrackDistU", -rangeTrackU, rangeTrackU);
  m_trackDistV = makeH1("TrackDistV", -rangeTrackV, rangeTrackV);
  m_trackDist = makeH1("TrackDist", 0, rangeTrackDist);
}

std::string Analyzers::Distances::name() const { return "Distances"; }

void Analyzers::Distances::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  for (Index itrack = 0; itrack < event.numTracks(); ++itrack) {
    const Storage::Track& track = *event.getTrack(itrack);
    if (!track.hasLocalState(m_sensorId))
      continue;

    const Storage::TrackState& state = track.getLocalState(m_sensorId);

    // full combinatorics: all clusters to all tracks
    for (Index icluster = 0; icluster < plane.numClusters(); ++icluster) {
      const Storage::Cluster& cluster = *plane.getCluster(icluster);

      XYVector delta = cluster.posLocal() - state.offset();
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      m_allDistU->Fill(delta.x());
      m_allDistV->Fill(delta.y());
      m_allDist->Fill(delta.r());
      m_allMahalanobis->Fill(mahalanobis(cov, delta));
    }

    // full combinatorics: all tracks to all other tracks
    for (Index itrack2 = 0; itrack2 < event.numTracks(); ++itrack2) {
      if (itrack2 == itrack)
        continue;
      if (!event.getTrack(itrack)->hasLocalState(m_sensorId))
        continue;

      const Storage::TrackState& state2 =
          event.getTrack(itrack2)->getLocalState(m_sensorId);
      XYVector delta = state2.offset() - state.offset();
      m_trackDistU->Fill(delta.x());
      m_trackDistV->Fill(delta.y());
      m_trackDist->Fill(delta.r());
    }

    // matching distances
    if (track.hasMatchedCluster(m_sensorId)) {
      const Storage::Cluster& cluster = *track.getMatchedCluster(m_sensorId);

      XYVector delta = cluster.posLocal() - state.offset();
      SymMatrix2 cov = cluster.covLocal() + state.covOffset();
      m_matchDistU->Fill(delta.x());
      m_matchDistV->Fill(delta.y());
      m_matchDist->Fill(delta.r());
      m_matchMahalanobis->Fill(mahalanobis(cov, delta));
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
