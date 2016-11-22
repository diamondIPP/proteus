Test Configurations
===================

A set of example runs with configuration files and preconfigured analysis chain.
Comparison output files containing the expected results are provided. This
should allow a simple validation of changes to prevent functionality regressions
when changing the code.

**WARNING** This is *work-in-progress*

The following commands will build the software and run an example analysis
chain for one of the example datasets

    ./build.sh
    ./download_data.py
    ./run_telescope.sh <RUN>
    ./run_dut0.sh <RUN>


Run 875
-------

*   Part of the published 2015 data
*   Around 1M events
*   Setup and device-under-tests are not completely clear

Run 1066
--------

*   **Prefer this dataset for tests**
*   Unpublished data from the July 2016 campaign at SPS
*   Six layer FEI4 telescope
*   Two irradiated CCPDv4 device-under-tests
*   Around 100k events with mostly one track per event
