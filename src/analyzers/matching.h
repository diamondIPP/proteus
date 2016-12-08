#ifndef MATCHING_H
#define MATCHING_H

#include <string>
#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "analyzers/analyzer.h"
#include "dualanalyzer.h"
#include "utils/definitions.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class Distances : public Analyzer {
public:
  Distances(const Mechanics::Device& device, Index sensorId, TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct Hists {
    TH1D* deltaU;
    TH1D* deltaV;
    TH1D* dist;
    TH1D* d2;

    Hists();
    Hists(TDirectory* dir,
          std::string prefix,
          double rangeDist,
          double rangeD2,
          int numBins);
    void fill(const XYVector& delta);
    void fill(const XYVector& delta, const SymMatrix2& cov);
  };

  Index m_sensorId;
  Hists m_trackTrack;
  Hists m_trackCluster;
  Hists m_match;
};

class Matching : public DualAnalyzer {
public:
  Matching(const Mechanics::Device* refDevice,
           const Mechanics::Device* dutDevice,
           TDirectory* dir = 0,
           const char* suffix = "",
           double pixelExtension = 3,
           /* Histogram options */
           double maxMatchingDist = 30,
           double sigmaBins = 10,
           unsigned int pixBinsX = 20,
           unsigned int pixBinsY = 20);

  void processEvent(const Storage::Event* refEvent,
                    const Storage::Event* dutDevent);

  void postProcessing();

private:
  std::vector<TH1D*> _matchedTracks;
  std::vector<TH1D*> _matchDist;
  std::vector<TH1D*> _matchDistX;
  std::vector<TH1D*> _matchDistY;
  std::vector<TH2D*> _inPixelTracks;
  std::vector<TH2D*> _inPixelTot;

  TDirectory* _plotDir;

}; // end of class
} // end of namespace

#endif // MATCHING_H
