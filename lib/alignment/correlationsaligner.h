// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <vector>

#include "aligner.h"

class TDirectory;

namespace proteus {

class Correlations;
class Device;

/** Align sensors in the xy-plane using only cluster correlations.
 *
 * This implictely assumes a straight track propagation with zero slope along
 * the z-axis.
 */
class CorrelationsAligner : public Aligner {
public:
  /**
   * \param device    The telescope device.
   * \param fixedId   Reference sensor that will be kept fixed.
   * \param alignIds  Sensors that should be aligned; must not contain fixedId.
   * \param dir       Histogram output directory.
   *
   * \warning This will add a `Correlations`-analyzer internally.
   */
  CorrelationsAligner(TDirectory* dir,
                      const Device& device,
                      const Index fixedId,
                      const std::vector<Index>& alignIds);
  ~CorrelationsAligner();

  std::string name() const;
  void execute(const Event& event);
  void finalize();

  Geometry updatedGeometry() const;

private:
  const Device& m_device;
  std::unique_ptr<Correlations> m_corr;
  std::vector<Index> m_backwardIds;
  std::vector<Index> m_forwardIds;
  Index m_fixedId;
};

} // namespace proteus
