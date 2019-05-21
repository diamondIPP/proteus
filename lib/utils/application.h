// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12-12
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "loop/eventloop.h"
#include "utils/config.h"

namespace proteus {

class Device;

/** Common application class.
 *
 * Handles the command line parameters, configures logging, reads the analysis
 * and device configuration including modifications from the command line,
 * and manages the input data. Output data must be handled separately for each
 * tool since the type of output data differs between them.
 */
class Application {
public:
  Application(const std::string& name,
              const std::string& description,
              const toml::Table& defaults = toml::Table());

  /** Parse command line arguments and setup configuration.
   *
   * \warning This methods exits the program if anything goes wrong.
   */
  void initialize(int argc, char const* argv[]);

  /** Device setup w/ updated geometry. */
  const Device& device() const { return *m_dev; }
  /** Tool configuration w/ defaults. */
  const toml::Value& config() const { return m_cfg; }
  /** Generate the output path for the given file name. */
  std::string outputPath(const std::string& name) const;

  /** Construct an event loop configured w/ input data from this application.
   *
   * Automatically opens the input file and adds it to the event loop.
   */
  EventLoop makeEventLoop() const;

private:
  std::string m_name;
  std::string m_desc;
  std::unique_ptr<Device> m_dev;
  toml::Value m_cfg;
  std::string m_inputPath;
  std::string m_outputPrefix;
  uint64_t m_skipEvents;
  uint64_t m_numEvents;
  bool m_printEvents;
  bool m_showProgress;
};

} // namespace proteus
