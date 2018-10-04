Test datasets and scripts
=========================

This directory contains example configuration files and scripts for
different simulated datasets. To be used to verify new functionality and
check for regressions on internal changes.

The following commands will download an example dataset and run all
parts of a typical analysis chain:

    ./download_data.py # only needed once
    ./run_noisescan.sh ${dataset}
    ./run_align_tel.sh ${dataset}
    ./run_align_dut.sh ${dataset}
    ./run_recon.sh ${dataset}

To simplify tests, each script is independent from previous steps and
can be run on its own. E.g. the dut alignment will start from an initial
geometry where the telescope planes are already in the correct
correction; the reconstruction code will use the true simulated
geometry.

Another set of commands runs the full analysis chain, i.e. starting from an
unaligned geometry with each step depending on the output of the previous
step. Either run each step separately

    ./download_data.py # only needed once
    ./run_chain_noisescan.sh ${dataset}
    ./run_chain_align_tel.sh ${dataset}
    ./run_chain_align_dut.sh ${dataset}
    ./run_chain_recon.sh ${dataset}

or in one combined command

    ./download_data.py # only needed once
    ./run_chain_all.sh ${dataset}

All scripts assume that the environment is setup such that the `pt-...`
binaries can be called directly, e.g. by sourcing the `activate.sh`
script in the build directory. Please see the `README.md` file in the
main directory for build instructions.

Setup
-----

The simulation setup is modelled after the UNIGE telescope: six IBL
planar modules as telescope planes and a seventh IBL planar module to
act as the device-under-test. The telescope planes are simulated with
FE-I4-like digitization and a threshold of 3000e; the device-under-test
uses a threshold of 2000e and stores the simulated charge without
digitization. The timing information is currently unavailable in the
simulated datasets.

The simulated geometries are misaligned by random offsets and rotations
with respect to the nominal geometry.

The available geometry files are located in the `geometry` folder. The
following files are available:

*   `unigetel_nominal.toml` is the nominal geometry file, i.e. the one that
    one would write when setting up the telescope. It needs alignment.
*   `<dataset>.toml` is the real geometry of the full device, i.e. telescop
    and device-under-test. The geometry file obtained after alignment should
    be very similar to this, if the alignment is working fine.
*   `<dataset>-telescope.tel` contains the true telescope geometry and the
    nominal device-under-test geometry. Just the device-under-test needs to
    be aligned. A file similar to this should be obtained after the telescope
    alignment.
*   `<dataset>-telescope_first_last.tel` contains the true geometry for the
    first and last telescope plane and the nominal geometry for the remaining
    sensors.

Datasets
--------

| Name                                          | Beam        | Particles/event | Events | Comment |
| --------------------------------------------- | ----------- | --------------- | ------ | ------- |
| unigetel_ebeam012_nparticles01                | 12 GeV pi+  | 1               | 100k   | |
| unigetel_ebeam120_nparticles01_misalignxyrotz | 120 GeV pi+ | 1               | 100k   | Only misalignment in x, y, z-rotation |
| unigetel_ebeam120_nparticles01                | 120 GeV pi+ | 1               | 100k   | |
| unigetel_ebeam120_nparticles02                | 120 GeV pi+ | 2               | 100k   | |
