/**
 * @file run noise scan to find and mask noisy pixels
 * @author Moritz Kiehn <msmk@cern.ch>
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "analyzers/eventprinter.h"
#include "loopers/looper.h"
#include "processors/processors.h"
#include "processors/clustermaker.h"
#include "storage/storageio.h"
#include "utils/logger.h"
#include "utils/eventloop.h"

void run_noise(const std::string& input_path,
               const std::string& output_path)
{
  Storage::StorageIO input(input_path.c_str(), Storage::INPUT);
  Utils::EventLoop loop(&input);
  
  Processors::Processor* clusterer = new Processors::ClusterMaker(1, 1, 2);
  Analyzers::SingleAnalyzer* printer = new Analyzers::EventPrinter();
  
  loop.addProcessor(std::unique_ptr<Processors::Processor>(clusterer));
  loop.addAnalyzer(std::unique_ptr<Analyzers::SingleAnalyzer>(printer));
  
  Utils::globalLogger().setLevel(Utils::Logger::DEBUG);
  loop.run();
}

int main(int argc, char const* argv[])
{
  namespace po = boost::program_options;
  using std::string;

  string argInput;
  string argOutput;
  string argDevice;
  string argGlobal;

  po::options_description options("Judith noise scan");
  po::positional_options_description positional;
  // clang-format off
  options.add_options()
    ("help,h", "show this help")
    ("input,i", po::value<string>(&argInput)->required(), "input path")
    ("output,o", po::value<string>(&argOutput)->required(), "output path");
    // ("device,d", po::value<string>(&argDevice)->required(), "path to the device configuration")
    // ("global,g", po::value<string>(&argGlobal)->required(), "path to the global configuration");
  // clang-format on
  positional.add("output", 1).add("input", 1);

  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(options)
                  .positional(positional)
                  .run(),
              vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cout << options << std::endl;
      return EXIT_SUCCESS;
    }
    
    run_noise(argInput, argOutput);
    
  } catch (po::error& err) {
    std::cerr << err.what() << "\n\n" << options << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
