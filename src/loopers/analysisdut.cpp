#include "analysisdut.h"

#include <cassert>
#include <vector>
#include <iostream>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../processors/trackmatcher.h"

//=========================================================
Loopers::AnalysisDut::AnalysisDut(Storage::StorageIO* refInput,
				  Storage::StorageIO* dutInput,
				  Processors::TrackMatcher* trackMatcher,
				  ULong64_t startEvent,
				  ULong64_t numEvents,
				  unsigned int eventSkip) :
  Looper(refInput, dutInput, startEvent, numEvents, eventSkip),
  _trackMatcher(trackMatcher)
{
  assert(refInput && dutInput && trackMatcher &&
	 "Looper: initialized with null object(s)");
}


//=========================================================
void Loopers::AnalysisDut::print() const {
  std::cout << "\n=== AnalysisDut === looper details: " << std::endl;
  Looper::print();
}

//=========================================================
void Loopers::AnalysisDut::loop() {
  if( _printLevel > 0 )
    std::cout << "## [AnalysisDut::loop]" << std::endl;

  // loop in events
  for (ULong64_t nevent=_startEvent; nevent<=_endEvent; nevent++){
    Storage::Event* refEvent = _refStorage->readEvent(nevent);
    Storage::Event* dutEvent = _dutStorage->readEvent(nevent);

    if( _printLevel > 0 ){
      std::cout << "\n========================" << std::endl
		<< " Event " << nevent << std::endl
		<< "========================\n";
      
      std::cout << "\n**** refEvent *** " << std::endl;
      refEvent->print();
      
      std::cout << "\n**** dutEvent **** " << std::endl;
      dutEvent->print();
    }

    
    // Match ref tracks to dut clusters (information stored in event)
    _trackMatcher->matchEvent(refEvent, dutEvent);

    
    // make single analyzers to process event
    for (unsigned int i=0; i<_numSingleAnalyzers; i++){
      if( _printLevel > 0 ){
	std::cout << "Single analyzer " << i << " processing ... " << std::endl;
	_singleAnalyzers.at(i)->print();
      }
      _singleAnalyzers.at(i)->processEvent(dutEvent);
    }
    
    // make dual analyzers to process event
    for (unsigned int i=0; i<_numDualAnalyzers; i++){
      if( _printLevel > 0 ){
	std::cout << "Dual analyzer " << i << " processing ... " << std::endl;
	_dualAnalyzers.at(i)->print();
      }
      _dualAnalyzers.at(i)->processEvent(refEvent,dutEvent);
    }
    
    progressBar(nevent);
    
    delete refEvent;
    delete dutEvent;

  } // end loop in events

  // single analyzers post-processing 
  for (unsigned int i=0; i<_numSingleAnalyzers; i++)
    _singleAnalyzers.at(i)->postProcessing();
  
  // dual analyzers post-processing
  for (unsigned int i=0; i<_numDualAnalyzers; i++)
    _dualAnalyzers.at(i)->postProcessing();
}

