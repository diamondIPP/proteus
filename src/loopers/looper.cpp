#include "looper.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <iomanip>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"

using std::cout;
using std::endl;
using std::flush;

bool Loopers::Looper::noBar = false;

//=========================================================
Loopers::Looper::Looper(Storage::StorageIO *refStorage,
			Storage::StorageIO *dutStorage,
			ULong64_t startEvent,
			ULong64_t numEvents,
			unsigned int eventSkip,
			int printLevel) :
  _refStorage(refStorage),
  _dutStorage(dutStorage),
  _startEvent(startEvent),
  _numEvents(numEvents),
  _eventSkip(eventSkip),
  _totalEvents(0),
  _endEvent(0),
  _numSingleAnalyzers(0),
  _numDualAnalyzers(0),
  _printLevel(printLevel)
{
  assert(refStorage && "Looper: null ref. storage passed");
  
  _totalEvents = _refStorage->getNumEvents();
  if (_dutStorage && (Long64_t)_totalEvents > _dutStorage->getNumEvents())
    _totalEvents = _dutStorage->getNumEvents();
  
  if (_eventSkip < 1)
    throw "Looper: event skip can't be smaller than 1";
  
  if (_startEvent >= _totalEvents)
    throw "Looper: start event exceeds storage file";
  
  if (_numEvents == 0) _numEvents = _totalEvents - _startEvent;
  
  _endEvent = _startEvent + _numEvents - 1;
  if (_endEvent >= _totalEvents)
    throw "Looper: end event exceeds storage file";
}

//=========================================================
Loopers::Looper::~Looper(){
  for (unsigned int i=0; i<_singleAnalyzers.size(); i++)
      delete _singleAnalyzers.at(i);
  
  for (unsigned int i=0; i<_dualAnalyzers.size(); i++)
      delete _dualAnalyzers.at(i);
}

//=========================================================
void Loopers::Looper::progressBar(ULong64_t nevent){
  if (noBar) return;
  
  assert(nevent >= _startEvent && nevent <= _endEvent &&
	 "Looper: progress recieved event outside range");
  
  ULong64_t progress = nevent - _startEvent + 1;
  if (progress % 500 && nevent != _endEvent) return;
  
  progress = (progress * 100) / _numEvents; // Now as an integer %
  
  cout << "\rProgress: [";
  for (unsigned int i=1; i<=50; i++){
    if (progress >= i * 2) cout << "=";
      else cout << " ";
  }
    cout << "] " << std::setw(4) << progress << "%" << flush;
    if (nevent == _endEvent) cout << endl;
}

//=========================================================
void Loopers::Looper::addAnalyzer(Analyzers::SingleAnalyzer* analyzer){
  assert(analyzer && "Looper: tried to add a null analyzer");
  _singleAnalyzers.push_back(analyzer);
  _numSingleAnalyzers++;
}

//=========================================================
void Loopers::Looper::addAnalyzer(Analyzers::DualAnalyzer* analyzer){
  assert(analyzer && "Looper: tried to add a null analyzer");
  _dualAnalyzers.push_back(analyzer);
  _numDualAnalyzers++;
}

//=========================================================
void Loopers::Looper::print() const {
  std::cout << " - looper has "
	    << _numSingleAnalyzers << " single analyzers and "
	    << _numDualAnalyzers << " dual analyzers." << std::endl;
  if( _printLevel > 1 ){
    int cnt=1;
    for(std::vector<Analyzers::SingleAnalyzer*>::const_iterator cit=_singleAnalyzers.begin();
	cit!=_singleAnalyzers.end(); ++cit, cnt++)
      std::cout << "    * SingleAnalyzer #" << cnt << " => " << (*cit)->printStr() << std::endl;        
    cnt=1;
    for(std::vector<Analyzers::DualAnalyzer*>::const_iterator cit=_dualAnalyzers.begin();
	cit!=_dualAnalyzers.end(); ++cit, cnt++)
      std::cout << "    * DualAnalyzer   #" << cnt << " => " << (*cit)->printStr() << std::endl;
  }
  
}

