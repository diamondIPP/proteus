#ifndef ANALYSISDUT_H
#define ANALYSISDUT_H

#include "looper.h"

namespace Storage { class StorageIO; }
namespace Mechanics { class Device; }
namespace Processors { class TrackMatcher; }

namespace Loopers {
  
  class AnalysisDut : public Looper {
  public:
    AnalysisDut(/* These arguments are needed to be passed to the base looper class */
		Storage::StorageIO* refInput,
		Storage::StorageIO* dutInput,
		/* Use if the looper needs to make clusters and/or tracks... */
		Processors::TrackMatcher* trackMatcher,
		ULong64_t startEvent = 0,
		ULong64_t numEvents = 0,
		unsigned int eventSkip = 1);
    
    void loop();
    
    void print();

  private:
    Processors::TrackMatcher* _trackMatcher;

  }; // end of class
  
} // end of namespace

#endif // ANALYSISDUT_H
