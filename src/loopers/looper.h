#ifndef LOOPER_H
#define LOOPER_H

#include <vector>
#include <Rtypes.h>

namespace Storage { class StorageIO; }
namespace Analyzers { class SingleAnalyzer; }
namespace Analyzers { class DualAnalyzer; }

namespace Loopers {
  
  class Looper {
  protected:
    Looper(Storage::StorageIO* refStorage,
	   Storage::StorageIO* dutStorage = 0,
	   ULong64_t startEvent = 0,
	   ULong64_t numEvents = 0,
	   unsigned int eventSkip = 1,
	   int printLevel=0);
    
    virtual ~Looper();
    void progressBar(ULong64_t nevent);
    virtual void print() const;
    
    /** Pure virtual function (derived classes must
	implement this function) */
    virtual void loop() = 0;
    
  public:
    void addAnalyzer(Analyzers::SingleAnalyzer* analyzer);
    void addAnalyzer(Analyzers::DualAnalyzer* analyzer);
    
    ULong64_t getStartEvent() const { return _startEvent; }
    ULong64_t getEndEvent() const { return _endEvent; }

    void setPrintLevel(int printLevel) { _printLevel = printLevel; }
    
    static bool noBar;
      
  protected:
    Storage::StorageIO* _refStorage;
    Storage::StorageIO* _dutStorage;
    ULong64_t _startEvent;
    ULong64_t _numEvents;
    unsigned int _eventSkip;
    ULong64_t _totalEvents;
    ULong64_t _endEvent;

    unsigned int _numSingleAnalyzers;
    std::vector<Analyzers::SingleAnalyzer*> _singleAnalyzers;
    
    unsigned int _numDualAnalyzers;
    std::vector<Analyzers::DualAnalyzer*> _dualAnalyzers;

    int _printLevel;

  };  // end of class
} // end of namespace

#endif // LOOPER_H
