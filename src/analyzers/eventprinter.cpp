#include "storage/event.h"
#include "storage/plane.h"
#include "storage/storageio.h"
#include "utils/definitions.h"
#include "utils/logger.h"

#include "eventprinter.h"

using Utils::logger;

std::shared_ptr<Analyzers::EventPrinter> Analyzers::EventPrinter::make()
{
  return std::make_shared<EventPrinter>();
}

std::string Analyzers::EventPrinter::name() const { return "EventPrinter"; }

void Analyzers::EventPrinter::analyze(uint64_t eventId,
                                      const Storage::Event& event)
{
  INFO("event ", eventId, ":\n");
  INFO("  hits: ", event.getNumHits(), '\n');
  INFO("  clusters: ", event.getNumClusters(), '\n');
  INFO("  tracks: ", event.getNumTracks(), '\n');
  for (Index iplane = 0; iplane < event.getNumPlanes(); ++iplane) {
    const Storage::Plane* plane = event.getPlane(iplane);
    INFO("  sensor", iplane, ":\n");
    if (0 < plane->numHits())
      INFO("    hits: ", plane->numHits(), '\n');
    if (0 < plane->numClusters())
      INFO("    clusters: ", plane->numClusters(), '\n');
  }
  if (0 < event.numTracks())
    INFO("  tracks: ", event.numTracks(), '\n');
}

void Analyzers::EventPrinter::finalize() {}
