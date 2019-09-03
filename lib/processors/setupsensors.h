// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/** Automated setup of per-sensor processors.
 *
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#pragma once

namespace proteus {

class Device;
class EventLoop;

/** Add per-sensor processors to the event loop.
 *
 * Depending on the device configuration this can include hit mappers,
 * hit region application, and/or clusterizers.
 */
void setupPerSensorProcessing(const Device& device, EventLoop& loop);

} // namespace proteus
