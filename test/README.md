Test Configurations
===================

A set of example runs with configuration files and preconfigured analysis chain.
This should allow a simple validation of changes to prevent functionality
regressions when changing the code.

**WARNING** This is *work-in-progress*

The test scripts assume that the environment is setup such that the `pt-...`
binaries can be called directly, e.g. by sourcing the `activate.sh` script in
the build directory. Please see the `README.md` file in the main directory for
further build instructions.

The following commands will download the example data files and run the analysis
chain for one of the example dataset with run number 1066:

    ./download_data.py
    ./run_tel_noisescan.sh 1066
    ./run_tel_align.sh 1066
    ./run_track.sh 1066
    ./run_dut0_noisescan.sh 1066


Run 875
-------

*   Part of the published 2015 data
*   Around 1M events
*   Setup and device-under-tests are not completely clear

Run 1066
--------

*   **Prefer this dataset for tests**
*   Unpublished data from the July 2016 campaign at SPS
*   Six layer FE-I4 telescope
*   Two irradiated CCPDv4 device-under-tests
*   Around 100k events with mostly one track per event

Run 9817
--------

*   Six layer FE-I4 telescope
*   One H35demo device-under-test
