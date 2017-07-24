#ifndef PT_TRACKS_H
#define PT_TRACKS_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

class Tracks : public Analyzer {
public:
  Tracks(const Mechanics::Device* device,
         TDirectory* dir,
         /* Histogram options */
         const double reducedChi2Max = 10,
         const double slopeMax = 0.01,
         const int bins = 128);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  const TH1D* histSlopeX() const { return m_slopeX; }
  const TH1D* histSlopeY() const { return m_slopeY; }

private:
  TH1D* m_nTracks;
  TH1D* m_size;;
  TH1D* m_reducedChi2;
  TH2D* m_offsetXY;
  TH1D* m_offsetX;
  TH1D* m_offsetY;
  TH2D* m_slopeXY;
  TH1D* m_slopeX;
  TH1D* m_slopeY;
};

} // namespace Analyzers

#endif // PT_TRACKS_H
