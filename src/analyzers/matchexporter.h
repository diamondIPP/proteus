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
  struct TrackTree {
    Float_t trkRedChi2;
    Float_t trkU, trkV;
    Float_t dist;
    Float_t cluU, cluV, cluCol, cluRow;
    Int_t cluSize, cluSizeCol, cluSizeRow;
    TTree* tree;
  };
  struct ClusterTree {
    Float_t u, v;
    Float_t col, row;
    Int_t size, sizeCol, sizeRow;
    TTree* tree;
  };

  const Mechanics::Sensor& m_sensor;
  Index m_sensorId;
  TrackTree m_ttrack;
  ClusterTree m_tcluster;
  std::string m_name;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
