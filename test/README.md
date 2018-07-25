Test datasets and scripts
=========================

This directory contains example configuration files and scripts for
different simulated datasets. To be used to verify new functionality and
check for regressions on internal changes.

The following commands will download an example dataset and run all
parts of a typical analysis chain:

    ./download_data.py ${dataset}
    ./run_noisescan.sh ${dataset}
    ./run_align_tel.sh ${dataset}
    ./run_align_dut.sh ${dataset}
    ./run_reco.sh ${dataset}

To simplify tests, each script is independent from previous steps and
can be run on its own. E.g. the dut alignment will start from an initial
geometry where the telescope planes are already in the correct
correction; the reconstruction code will use the true simulated
geometry.

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
digitization. The timing information is currently unavailable in the simulated datasets.

The simulated geometry is misaligned by random offsets and rotations
with respect to the nominal geometry.

Datasets
--------

| Name | Beam | Particles/event | Comment |
| ---- | ---- | --------------- | ------- |
| unigetel_ebeam012_nparticles01 | 12 GeV pi+ | 1 | |
| unigetel_ebeam120_nparticles01_misalignxyrotz | 120 GeV pi+ | 1 | Only misalignment in x, y, z-rotation |
| unigetel_ebeam120_nparticles01 | 120 GeV pi+ | 1 | |
| unigetel_ebeam120_nparticles02 | 120 GeV pi+ | 2 | |
