/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12-12
 */

#ifndef PT_APPLICATION_H
#define PT_APPLICATION_H

#include <cstdint>
#include <memory>
#include <string>

#include "io/io.h"
#include "utils/config.h"
#include "utils/eventloop.h"

namespace Mechanics {
class Device;
}

/** Common application class.
 *
 * Handles the command line parameters, configures loggins, reads the analysis
 * and device configuration including modifications from the command line,
 * and manages the input data. Output data must be handled separately for each
 * tool since the type of output data differs between them.
 */
class Application {
public:
  Application(const std::string& name,
              const std::string& description,
              const toml::Table& defaults = toml::Table());

  /** Parse command line arguments and setup configuration and input data.
   *
   * \warning This methods exits the program if anything goes wrong.
   */
  void initialize(int argc, char const* argv[]);

  /** Device setup w/ updated geometry. */
  const Mechanics::Device& device() const { return *m_dev; }
  /** Tool configuration w/ defaults. */
  const toml::Value& config() const { return m_cfg; }
  /** Output path for the given file name. */
  std::string outputPath(const std::string& name) const;

  /** Construct an event loop configured w/ input data from this application.
   *
   * \warning The event loop uses the input data store in this application
   *          object. Therefore, it is only valid for the life time of the
   *          application object.
   */
  Utils::EventLoop makeEventLoop() const;

private:
  std::string m_name;
  std::string m_desc;
  toml::Value m_cfg;
  std::unique_ptr<Mechanics::Device> m_dev;
  std::shared_ptr<Io::EventReader> m_reader;
  std::string m_outputPrefix;
  uint64_t m_skipEvents;
  uint64_t m_numEvents;
  bool m_showProgress;
};

#endif // PT_APPLICATION_H
