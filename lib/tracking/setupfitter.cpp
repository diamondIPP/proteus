/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2019-03
 */

#include "setupfitter.h"

#include <memory>

#include "loop/eventloop.h"
#include "tracking/gblfitter.h"
#include "tracking/straightfitter.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

void Tracking::setupTrackFitter(const Mechanics::Device& device,
                                const std::string& type,
                                Loop::EventLoop& loop)
{
  if (type.empty()) {
    INFO("no track fitter is configured");
  } else if (type == "gbl3d") {
    loop.addProcessor(std::make_shared<GblFitter>(device));
  } else if (type == "straight3d") {
    loop.addProcessor(std::make_shared<Straight3dFitter>(device));
  } else if (type == "straight4d") {
    loop.addProcessor(std::make_shared<Straight4dFitter>(device));
  } else {
    FAIL("unknown configured track fitter '", type, "'");
  }
}
