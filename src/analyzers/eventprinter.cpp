#include "storage/event.h"
#include "storage/plane.h"
#include "storage/storageio.h"
#include "utils/definitions.h"
#include "utils/logger.h"

#include "eventprinter.h"

using Utils::logger;

std::unique_ptr<Analyzers::EventPrinter> Analyzers::EventPrinter::make()
{
  return std::unique_ptr<EventPrinter>(new EventPrinter());
}

Analyzers::EventPrinter::EventPrinter()
    : SingleAnalyzer(NULL)
    , _events(0)
{
}

void Analyzers::EventPrinter::processEvent(const Storage::Event* event)
{
  INFO("event ", _events, ":\n");
  INFO("  hits: ", event->getNumHits(), '\n');
  INFO("  clusters: ", event->getNumClusters(), '\n');
  INFO("  tracks: ", event->getNumTracks(), '\n');
  for (Index iplane = 0; iplane < event->getNumPlanes(); ++iplane) {
    Storage::Plane* plane = event->getPlane(iplane);
    DEBUG("  plane", iplane, ":\n");
    DEBUG("    hits: ", plane->getNumHits(), '\n');
    DEBUG("    clusters: ", plane->getNumHits(), '\n');
  }
  _events += 1;
}

void Analyzers::EventPrinter::postProcessing() {}
