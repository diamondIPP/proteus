#include "storage/event.h"
#include "storage/plane.h"
#include "storage/storageio.h"
#include "utils/definitions.h"
#include "utils/logger.h"

#include "eventprinter.h"

Analyzers::EventPrinter::EventPrinter()
    : SingleAnalyzer(NULL)
    , _events(0)
{
}

void Analyzers::EventPrinter::processEvent(const Storage::Event* event)
{
  Utils::Logger& log = Utils::globalLogger();

  log.info("event ", _events, ":\n");
  log.info("  hits: ", event->getNumHits(), '\n');
  log.info("  clusters: ", event->getNumClusters(), '\n');
  log.info("  tracks: ", event->getNumTracks(), '\n');
  for (Index iplane = 0; iplane < event->getNumPlanes(); ++iplane) {
    Storage::Plane* plane = event->getPlane(iplane);
    log.debug("  plane", iplane, ":\n");
    log.debug("    hits: ", plane->getNumHits(), '\n');
    log.debug("    clusters: ", plane->getNumHits(), '\n');
  }
  _events += 1;
}

void Analyzers::EventPrinter::postProcessing() {}
