// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include "loop/processor.h"

namespace proteus {

class Device;

/** Use device geometry to set global positions for clusters.
 *
 * \warning This does **not** update existing track parameters. This must be
 *          updated by refitting the track.
 */
class ApplyGeometry : public Processor {
public:
  ApplyGeometry(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Device& m_device;
};

} // namespace proteus
