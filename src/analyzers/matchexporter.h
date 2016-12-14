/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-11-09
 */

#ifndef PT_MATCHEXPORTER_H
#define PT_MATCHEXPORTER_H

#include <string>

#include <TDirectory.h>
#include <TTree.h>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"
#include "utils/statistics.h"

namespace Mechanics {
class Device;
class Sensor;
}

namespace Analyzers {

/** Export matched (and unmatched) tracks and clusters to a  TTree. */
class MatchExporter : public Analyzer {
public:
  MatchExporter(const Mechanics::Device& device,
                Index sensorId,
                TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct EventData {
    Int_t nTracks;
  };
  struct TrackData {
    Float_t u, v;
    Float_t col, row;
    Float_t redChi2;
  };
  struct ClusterData {
    Float_t u, v;
    Float_t col, row;
    Int_t size, sizeCol, sizeRow;
  };
  struct MatchData {
    Float_t d2;
  };

  const Mechanics::Sensor& m_sensor;
  Index m_sensorId;
  EventData m_ev;
  TrackData m_trk;
  MatchData m_mat;
  ClusterData m_cluMat;
  ClusterData m_cluUnm;
  TTree* m_treeTrk;
  TTree* m_treeClu;
  Utils::StatAccumulator<double> m_statMatTrkFraction;
  Utils::StatAccumulator<double> m_statMatCluFraction;
  Utils::StatAccumulator<double> m_statUnmTrk;
  Utils::StatAccumulator<double> m_statUnmClu;
  std::string m_name;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
