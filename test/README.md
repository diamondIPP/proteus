Test Configurations
===================

A set of example runs with configuration files and preconfigured analysis chain.
Comparison output files containing the expected results are provided. This
should allow a simple validation of changes to prevent functionality regressions
when changing the code.

**WARNING** This is *work-in-progress*

The following commands will build the software and run a full analysis chain.

    ./build.sh
    ./run_reference.sh
    ./run_dut.sh


Run 875
-------

*   Part of the previously published 2015 data
*   Composition?
*   Device-under-tests?

Run 1066
--------

*   Unpublished data from the July 2016 campaign at SPS
*   Six layer FEI4 telescope
*   Two irradiated CCPDv4 device-under-tests
*   Around 100k events with mostly one track per event
