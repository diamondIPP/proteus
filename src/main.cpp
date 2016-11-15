// #include <iostream>
#include <vector>
#include <string.h>
#include <iomanip>
#include <sys/time.h>

#include <TFile.h>
#include <TApplication.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TColor.h>

#include "storage/storageio.h"
#include "storage/event.h"
#include "storage/plane.h"
#include "storage/cluster.h"
#include "storage/hit.h"
#include "converters/kartelconvert.h"
#include "mechanics/device.h"
#include "processors/configprocessors.h"
#include "processors/eventdepictor.h"
#include "processors/clustermaker.h"
#include "processors/trackmaker.h"
#include "processors/trackmatcher.h"
#include "processors/processors.h"
#include "analyzers/configanalyzers.h"
#include "loopers/analysis.h"
#include "loopers/analysisdut.h"
#include "loopers/coarsealign.h"
#include "loopers/coarsealigndut.h"
#include "loopers/finealign.h"
#include "loopers/finealigndut.h"
#include "loopers/applymask.h"
#include "loopers/noisescan.h"
#include "loopers/processevents.h"
#include "loopers/synchronize.h"
#include "loopers/configloopers.h"
#include "utils/configparser.h"
#include "inputargs.h"

using namespace std;

//=========================================================
void convert(const char* input,
	     const char* output,
	     Long64_t triggers,
             const char* deviceCfg="")
{
  try {
		if (strlen(deviceCfg)) {
			auto device = Mechanics::Device::fromFile(deviceCfg);
			Converters::KartelConvert convert(input, output, &device);
			convert.processFile(triggers);
		} else {
			Converters::KartelConvert convert(input, output, nullptr);
			convert.processFile(triggers);
		}
  }
   catch(const std::exception &e){
    cout << e.what() << endl;
   }
   catch (const char* e){
     cout << "ERR :: " <<  e << endl;
   }
}

//=========================================================
void synchronize(const char* refInputName,
		 const char* dutInputName,
                 const char* refOutputName,
		 const char* dutOutputName,
                 ULong64_t startEvent,
		 ULong64_t numEvents,
                 const char* refDeviceCfg,
		 const char* dutDeviceCfg,
                 const char* tbCfg)
{
  try {
    Mechanics::Device refDevice = Mechanics::Device::fromFile(refDeviceCfg);
    Mechanics::Device dutDevice = Mechanics::Device::fromFile(dutDeviceCfg);

    ConfigParser runConfig(tbCfg);

    Storage::StorageIO refInput(refInputName, Storage::INPUT);
    Storage::StorageIO dutInput(dutInputName, Storage::INPUT);

    Storage::StorageIO refOutput(refOutputName, Storage::OUTPUT, refInput.getNumPlanes());
    Storage::StorageIO dutOutput(dutOutputName, Storage::OUTPUT, dutInput.getNumPlanes());

    Loopers::Synchronize looper(&refDevice, &dutDevice, &refOutput, &dutOutput,
				&refInput, &dutInput, startEvent, numEvents);
    Loopers::configSynchronize(runConfig, looper);
    looper.loop();
  }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e) {
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void applyMask(const char* inputName,
	       const char* outputName,
               ULong64_t startEvent,
	       ULong64_t numEvents,
               const char* deviceCfg,
	       const char* tbCfg,
	       const std::vector<int> runs,
	       int printLevel)
{
  try {
    Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);

    // [SGS] not used...
    // ConfigParser runConfig(tbCfg);

    unsigned int inMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO input(inputName, Storage::INPUT, 0, inMask, device.getSensorMask());

    Storage::StorageIO output(outputName, Storage::OUTPUT, device.getNumSensors(), inMask, 0);
    output.setNoiseMaskData(device.noiseMask());
    output.setRuns(runs);

    Loopers::ApplyMask looper(&device, &output, &input, startEvent, numEvents);
    const Storage::Event* start = input.readEvent(looper.getStartEvent());
    const Storage::Event* end = input.readEvent(looper.getEndEvent());
    device.setTimeStampRange(start->getTimeStamp(), end->getTimeStamp());
    delete start;
    delete end;

    looper.loop();
  }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void noiseScan(const char* inputName,
	       ULong64_t startEvent,
	       ULong64_t numEvents,
	       const char* deviceCfg,
               const char* tbCfg,
	       int printLevel)
{
  try {
    ConfigParser runConfig(tbCfg);

    Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);
    Storage::StorageIO input(inputName, Storage::INPUT, device.getNumSensors());

    Loopers::NoiseScan looper(&device, &input, startEvent, numEvents);
    looper.setPrintLevel(printLevel);

    Loopers::configNoiseScan(runConfig, looper);
    if(printLevel>0) looper.print();
    looper.loop();
  }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " <<  e << endl;
  }
}

//=========================================================
void coarseAlign(const char* inputName,
		 ULong64_t startEvent,
		 ULong64_t numEvents,
                 const char* deviceCfg,
		 const char* tbCfg,
		 int printLevel)
{
  try {
    ConfigParser runConfig(tbCfg);
    Processors::ClusterMaker* clusterMaker = Processors::generateClusterMaker(runConfig);
    Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);

    unsigned int treeMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO input(inputName, Storage::INPUT, 0, treeMask);

    Loopers::CoarseAlign looper(&device, clusterMaker, &input, startEvent, numEvents);
    Loopers::configCoarseAlign(runConfig, looper);
    if(printLevel>0) looper.print();
    looper.loop();

    delete clusterMaker;
  }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void coarseAlignDUT(const char* refInputName,
		    const char* dutInputName,
                    ULong64_t startEvent,
		    ULong64_t numEvents,
                    const char* refDeviceCfg,
		    const char* dutDeviceCfg,
                    const char* tbCfg,
		    int printLevel)
{
  cout << "coarseAlignDUT"  << endl;
  try
  {
    ConfigParser runConfig(tbCfg);
    Processors::ClusterMaker* clusterMaker = Processors::generateClusterMaker(runConfig);

    Mechanics::Device refDevice = Mechanics::Device::fromFile(refDeviceCfg);
    Mechanics::Device dutDevice = Mechanics::Device::fromFile(dutDeviceCfg);

    unsigned int treeMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO refInput(refInputName, Storage::INPUT, 0, treeMask);
    Storage::StorageIO dutInput(dutInputName, Storage::INPUT, 0, treeMask);

    Loopers::CoarseAlignDut looper(&refDevice, &dutDevice, clusterMaker,
                                   &refInput, &dutInput, startEvent, numEvents);
    Loopers::configCoarseAlign(runConfig, looper);
    if(printLevel>0) looper.print();
    looper.loop();
  }
   catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void fineAlign(const char* inputName,
	       ULong64_t startEvent,
	       ULong64_t numEvents,
               const char* deviceCfg,
	       const char* tbCfg,
	       int printLevel)
{
  try
  {
    ConfigParser runConfig(tbCfg);
    Processors::ClusterMaker* clusterMaker = Processors::generateClusterMaker(runConfig);
    Processors::TrackMaker* trackMaker = Processors::generateTrackMaker(runConfig, true);
    Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);

    unsigned int treeMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO input(inputName, Storage::INPUT, 0, treeMask, 0);

    Loopers::FineAlign looper(&device, clusterMaker, trackMaker, &input,
                              startEvent, numEvents);
    Loopers::configFineAlign(runConfig, looper);
    if(printLevel>0) looper.print();
    looper.loop();

    delete trackMaker;
    delete clusterMaker;
  }
   catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void fineAlignDUT(const char* refInputName,
		  const char* dutInputName,
                  ULong64_t startEvent,
		  ULong64_t numEvents,
                  const char* refDeviceCfg,
		  const char* dutDeviceCfg,
                  const char* tbCfg,
		  int printLevel)
{
  try
  {
    ConfigParser runConfig(tbCfg);
    Processors::ClusterMaker* clusterMaker = Processors::generateClusterMaker(runConfig);
    Processors::TrackMaker* trackMaker = Processors::generateTrackMaker(runConfig, true);
    Mechanics::Device refDevice = Mechanics::Device::fromFile(refDeviceCfg);
    Mechanics::Device dutDevice = Mechanics::Device::fromFile(dutDeviceCfg);

    unsigned int treeMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO refInput(refInputName, Storage::INPUT, 0, treeMask);
    Storage::StorageIO dutInput(dutInputName, Storage::INPUT, 0, treeMask);

    Loopers::FineAlignDut looper(&refDevice, &dutDevice, clusterMaker, trackMaker,
                                 &refInput, &dutInput, startEvent, numEvents);
    Loopers::configFineAlign(runConfig, looper);
    if(printLevel>0) looper.print();
    looper.loop();

    delete trackMaker;
    delete clusterMaker;
  }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void process(const char* inputName,
	     const char* outputName,
             ULong64_t startEvent,
	     ULong64_t numEvents,
             const char* deviceCfg,
	     const char* tbCfg,
             const char* resultsName,
	     int printLevel)
{
  try
  {
    Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);

    ConfigParser runConfig(tbCfg);
    Processors::ClusterMaker* clusterMaker = Processors::generateClusterMaker(runConfig);
    Processors::TrackMaker* trackMaker=0;
    if(device.getNumSensors() > 2)
      trackMaker = Processors::generateTrackMaker(runConfig);

    // input file
    unsigned int inMask = Storage::Flags::TRACKS | Storage::Flags::CLUSTERS;
    Storage::StorageIO input(inputName, Storage::INPUT, 0, inMask, 0);

    // output file
    unsigned int outMask = 0;
    if(device.getNumSensors() <= 2) outMask = Storage::Flags::TRACKS;
    Storage::StorageIO output(outputName, Storage::OUTPUT, device.getNumSensors(), outMask, 0);

    Loopers::ProcessEvents looper(&device, &output, clusterMaker, trackMaker, &input, startEvent, numEvents);
    const Storage::Event* start = input.readEvent(looper.getStartEvent());
    const Storage::Event* end = input.readEvent(looper.getEndEvent());
    device.setTimeStampRange(start->getTimeStamp(), end->getTimeStamp());
    delete start;
    delete end;

    // output results file
    TFile* results = 0;
    if(strlen(resultsName))
      results = new TFile(resultsName, "RECREATE");

    // add analyzers to looper and then loop
    Analyzers::configLooper(runConfig, &looper, &device, 0, results);
    if(printLevel>0) looper.print();
    looper.loop();

    if(results){
      results->Write();
      delete results;
    }

    delete clusterMaker;
    if(trackMaker) delete trackMaker;
  }
   catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e) {
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void analysis(const char* inputName,
              ULong64_t startEvent,
	      ULong64_t numEvents,
              const char* deviceCfg,
              const char* tbCfg,
	      const char* resultsName,
	      int printLevel){
  try
    {
      Mechanics::Device device = Mechanics::Device::fromFile(deviceCfg);

      ConfigParser runConfig(tbCfg);

      Storage::StorageIO input(inputName, Storage::INPUT);

      Loopers::Analysis looper(&input, startEvent, numEvents);
      const Storage::Event* start = input.readEvent(looper.getStartEvent());
      const Storage::Event* end = input.readEvent(looper.getEndEvent());
      device.setTimeStampRange(start->getTimeStamp(), end->getTimeStamp());
      delete start;
      delete end;

      TFile* results = 0;
      if(strlen(resultsName))
	results = new TFile(resultsName, "RECREATE");

      // add analyzers to looper and then loop
      Analyzers::configLooper(runConfig, &looper, &device, 0, results);
      if(printLevel>0) looper.print();
      looper.loop();

      if (results){
	results->Write();
	delete results;
      }
    }
   catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch (const char* e){
      cout << "ERR :: " << e << endl;
    }
}

//=========================================================
void analysisDUT(const char* refInputName,
		 const char* dutInputName,
                 ULong64_t startEvent,
		 ULong64_t numEvents,
                 const char* refDeviceCfg,
		 const char* dutDeviceCfg,
                 const char* tbCfg,
		 const char* resultsName,
		 int printLevel)
{
  try
    {
      Mechanics::Device refDevice = Mechanics::Device::fromFile(refDeviceCfg);
      Mechanics::Device dutDevice = Mechanics::Device::fromFile(dutDeviceCfg);

      ConfigParser runConfig(tbCfg);

      Processors::TrackMatcher* trackMatcher = new Processors::TrackMatcher(&dutDevice);

      Storage::StorageIO refInput(refInputName, Storage::INPUT);
      Storage::StorageIO dutInput(dutInputName, Storage::INPUT);

      // create looper instance
      Loopers::AnalysisDut looper(&refInput, &dutInput, trackMatcher, startEvent, numEvents);
      looper.setPrintLevel(printLevel);

      const Storage::Event* start = refInput.readEvent(looper.getStartEvent());
      const Storage::Event* end = refInput.readEvent(looper.getEndEvent());
      refDevice.setTimeStampRange(start->getTimeStamp(), end->getTimeStamp());
      delete start;
      delete end;

      TFile* results = 0;
      if(strlen(resultsName))
	results = new TFile(resultsName, "RECREATE");

      // add analyzers to looper and then loop
      Analyzers::configLooper(runConfig, &looper, &refDevice, &dutDevice, results);
      if(printLevel>1) looper.print();
      looper.loop();

      if(results){
	results->Write();
	delete results;
      }

      delete trackMatcher;
    }
  catch(const std::exception &e){
    cout << e.what() << endl;
  }
  catch(const char* e){
    cout << "ERR :: " << e << endl;
  }
}

//=========================================================
void printDevice(const char* configName)
{
  try
  {
    Mechanics::Device device = Mechanics::Device::fromFile(configName);
    device.print(std::cout);
  }
  catch (const char* e)
  {
    cout << "ERR :: " <<  e << endl;
  }
}

//=========================================================
int main(int argc, char** argv){
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  TApplication app("App", 0, 0);
  gStyle->SetOptStat("mre");

  InputArgs inArgs;
  if( inArgs.parseArgs(&argc, argv) )
    return 0;

  cout << "\nStarting Judith\n" << endl;
  inArgs.printArgs();

  // Static variables
  if (inArgs.getNoBar()) Loopers::Looper::noBar = true;

  if ( !inArgs.getCommand().compare("convert") )
    {
      convert( // converts kartel->ROOT using a noise mask specified in cfg file
	      // if cfg file is not present --> no noise is subtracted
	      // convert NumEvents (-1,0=ALL)
	      inArgs.getInputRef().c_str(),
	      inArgs.getOutputRef().c_str(),
	      inArgs.getNumEvents() ? inArgs.getNumEvents() : -1,
	      inArgs.getCfgRef().c_str());
    }
  else if ( !inArgs.getCommand().compare("synchronize") )
    {
      synchronize( // synchronizes DUT with Ref (2 inputs, 2 outputs)
		  // start at EvOffset, do NumEvents (0=ALL)
		  // 2 configs (Ref and DUT) synched data goes to
		  // OutputRef/DUT
		  inArgs.getInputRef().c_str(),
		  inArgs.getInputDUT().c_str(),
		  inArgs.getOutputRef().c_str(),
		  inArgs.getOutputDUT().c_str(),
		  inArgs.getEventOffset(),
		  inArgs.getNumEvents(),
		  inArgs.getCfgRef().c_str(),
		  inArgs.getCfgDUT().c_str(),
		  inArgs.getCfgTestbeam().c_str());
    }
  else if ( !inArgs.getCommand().compare("applyMask") )
    {
      applyMask(
                inArgs.getInputRef().c_str(),
                inArgs.getOutputRef().c_str(),
                inArgs.getEventOffset(),
                inArgs.getNumEvents(),
                inArgs.getCfgRef().c_str(),
                inArgs.getCfgTestbeam().c_str(),
		inArgs.getRuns(),
		inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("noiseScan") )
    {
      noiseScan( // produce a noise mask with a specified cut (prompted for)
                inArgs.getInputRef().c_str(),
		inArgs.getEventOffset(),
                inArgs.getNumEvents(),
                inArgs.getCfgRef().c_str(),
                inArgs.getCfgTestbeam().c_str(),
		inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("coarseAlign") )
    {
      coarseAlign( // coarse align a detector (Ref or DUT)
		  inArgs.getInputRef().c_str(),
		  inArgs.getEventOffset(),
		  inArgs.getNumEvents(),
		  inArgs.getCfgRef().c_str(),
		  inArgs.getCfgTestbeam().c_str(),
		  inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("coarseAlignDUT") )
    {
      coarseAlignDUT( // coarse align DUT to Ref detecor
		     inArgs.getInputRef().c_str(),
		     inArgs.getInputDUT().c_str(),
		     inArgs.getEventOffset(),
		     inArgs.getNumEvents(),
		     inArgs.getCfgRef().c_str(),
		     inArgs.getCfgDUT().c_str(),
		     inArgs.getCfgTestbeam().c_str(),
		     inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("fineAlign") )
    {
      fineAlign( // fine align Ref detector
		inArgs.getInputRef().c_str(),
                inArgs.getEventOffset(),
                inArgs.getNumEvents(),
                inArgs.getCfgRef().c_str(),
                inArgs.getCfgTestbeam().c_str(),
		inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("fineAlignDUT") )
    {
      fineAlignDUT( // fine align DUT to Ref detector
		   inArgs.getInputRef().c_str(),
		   inArgs.getInputDUT().c_str(),
		   inArgs.getEventOffset(),
		   inArgs.getNumEvents(),
		   inArgs.getCfgRef().c_str(),
		   inArgs.getCfgDUT().c_str(),
		   inArgs.getCfgTestbeam().c_str(),
		   inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("process") )
    {
      process( // makes clusters and tracks in detector planes (DUT and Red).
	      // If no. of planes < 3, no tracks are made
	      inArgs.getInputRef().c_str(),
	      inArgs.getOutputRef().c_str(),
	      inArgs.getEventOffset(),
	      inArgs.getNumEvents(),
	      inArgs.getCfgRef().c_str(),
	      inArgs.getCfgTestbeam().c_str(),
	      inArgs.getResults().c_str(),
	      inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("analysis") )
    {
      analysis( // runs over events and stores histos in OutputRef
	       // (runs over one device: DUT OR ref)
	       inArgs.getInputRef().c_str(),
	       inArgs.getEventOffset(),
	       inArgs.getNumEvents(),
	       inArgs.getCfgRef().c_str(),
	       inArgs.getCfgTestbeam().c_str(),
	       inArgs.getResults().c_str(),
	       inArgs.getPrintLevel());
    }
  else if ( !inArgs.getCommand().compare("analysisDUT") )
    {
      analysisDUT( // fills all the histograms: correletans, efficiencied, tracks,
		  // alignments plots, residuals,...
		  inArgs.getInputRef().c_str(),
		  inArgs.getInputDUT().c_str(),
		  inArgs.getEventOffset(),
		  inArgs.getNumEvents(),
		  inArgs.getCfgRef().c_str(),
		  inArgs.getCfgDUT().c_str(),
		  inArgs.getCfgTestbeam().c_str(),
		  inArgs.getResults().c_str(),
		  inArgs.getPrintLevel());
    }
  else if (inArgs.getCommand().size())
    {
      inArgs.usage();
      std::cout << "Unknown command! " << inArgs.getCommand() << std::endl;
    }

  gettimeofday(&t1, NULL);
  double fullTime = t1.tv_sec - t0.tv_sec + 1e-6*(t1.tv_usec-t0.tv_usec);
  cout << "\nTotal time [s] = " << fullTime << endl;

  cout << "\nEnding Judith\n" << endl;

  return 0;
}
