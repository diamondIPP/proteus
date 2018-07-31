#ifndef PT_TRACKS_H
#define PT_TRACKS_H

#include <vector>

#include "loop/analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

class Tracks : public Loop::Analyzer {
public:
  /** Construct a tracks analyzer.
   *
   * \param numTracksMax    Upper limit for tracks/event distribution
   * \param reducedChi2Max  Upper limit for chi^2/d.o.f. distribution
   * \param slopeRangeStd   Slope range measured in standard deviations
   * \param bins            Number of histogram bins
   */
  Tracks(TDirectory* dir,
         const Mechanics::Device& device,
         /* Histogram options */
         const int numTracksMax = 16,
         const double reducedChi2Max = 10,
         const double slopeStdRange = 5.0,
         const int bins = 128);

  std::string name() const;
  void execute(const Storage::Event& event);

  const TH1D* histSlopeX() const { return m_slopeX; }
  const TH1D* histSlopeY() const { return m_slopeY; }

private:
  TH1D* m_nTracks;
  TH1D* m_size;
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
