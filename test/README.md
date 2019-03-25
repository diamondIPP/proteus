# Test datasets and scripts

This directory contains example data, configurations and scripts for
different setups. This can be used to verify new functionality and check
for regressions on internal changes. The configuration files are
organized into folders by setup and dataset as

    <setup0>/<dataset0>/
            /<dataset1>/
            ...
    <setup1>/<dataset0>/
    ...

and the input data files are stored as

    data/<setup0>/<dataset0>.root # or other extension
                  <dataset1>.root
    ...

Since the data files are binary files, potentially large, and are
expected to only change rarely, they are stored in a separated git
repository and linked to this repository as a
[git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules). To
make the data available run the following commands:

    git submodule init   # once per clone or if submodule config changes
    git submodule update

Within a setup the device configuration, i.e. number and type of
sensors, is kept fix. Datasets can differ in geometry and beam
conditions. Each setup folder needs to have a `device.toml` file to
configure the device, and a `analysis.toml` file to configure the tools.
Each dataset folder should contain a `data.*` file and the following
geometry files:

*   `geometry.toml` is the best geometry description for the full
    device, i.e. the true geometry from the simulation. If the alignment
    works correctly, the geometry file obtained after alignment should
    be very similar to it.
*   `geometry-initial.toml` is the initial unaligned geometry file,
    i.e. the one that an operator would write when setting up the
    telescope. It needs alignment.
*   `geometry-telescope_first_last.tel` contains the best geometry for
    the first and last telescope plane and the nominal geometry for the
    remaining sensors.
*   `geometry-telescope.tel` contains the best telescope geometry and
    the nominal device-under-test geometry. Just the device-under-test
    needs to be aligned. A file similar to this should be obtained after
    the telescope alignment.

Depending on which scripts will be used, only a subset of the geometry
files need to be available. The following commands will download the
example data and run all parts of a typical analysis chain:

    ./run_noisescan.sh <setup>/<dataset> # uses `geometry.toml`
    ./run_align_tel.sh <setup>/<dataset> # uses `geometry-telescope_first_last.toml`
    ./run_align_dut.sh <setup>/<dataset> # uses `geometry-telescope.toml`
    ./run_recon.sh <setup>/<dataset> # uses `geometry.toml`

To simplify tests, each script is independent from previous steps and
can be run on its own. E.g. the dut alignment will start from an initial
geometry where the telescope planes are already in the correct
correction; the reconstruction code will use the best available
geometry, e.g. the true simulated geometry.

Another set of commands runs the full analysis chain, i.e. starting from
an unaligned geometry with each step depending on the output of the
previous step. Either run each step separately

    ./run_chain_noisescan.sh <setup>/<dataset> # uses `geometry-initial.toml`
    ./run_chain_align_tel.sh <setup>/<dataset> # uses `geometry-initial.toml`
    ./run_chain_align_dut.sh <setup>/<dataset>
    ./run_chain_recon.sh <setup>/<dataset>

or in one combined command

    ./run_chain_all.sh <setup>/<dataset> # uses `geometry-initial.toml`

All scripts assume that the environment is setup such that the `pt-...`
binaries can be called directly, e.g. by sourcing the `activate.sh`
script in the build directory. Please see the `README.md` file in the
main directory for build instructions.

## UNIGE telescope with a FE-I4 dummy dut (unigetel_dummy)

A simulated setup modelled after the UNIGE telescope: six IBL FE-I4
planar modules as telescope planes and a seventh IBL planar module to
act as the device-under-test. The telescope planes are simulated with
FE-I4-like digitization and a threshold of 3000e; the device-under-test
uses a threshold of 2000e and stores the simulated charge without
digitization. Timing information is unavailable in the simulated
datasets.

The simulated geometries are misaligned by random offsets and rotations
with respect to the nominal geometry. The following datasets are
available:

| Name | Beam | Particles/event | Comment |
| ---- | ---- | --------------- | ------- |
| ebeam005_positron_nparticles01_inc | 5GeV e+ | 1 | Inclined telescope planes |
| ebeam005_positron_nparticles01_upr | 5GeV e+ | 1 | Upright telescope planes |
| ebeam120_pionp_nparticles01_inc | 120GeV π+ | 1 | Inclined telescope planes |
| ebeam120_pionp_nparticles01_upr | 120GeV π+ | 1 | Upright telescope planes |
| ebeam180_pionp_nparticles01_inc | 180GeV π+ | 1 | Inclined telescope planes |
| ebeam180_pionp_nparticles01_inc_xygamma | 180GeV π+ | 1 | Inclined telescope planes, misalignment only in x,y,gamma |
| ebeam180_pionp_nparticles01_upr | 180GeV π+ | 1 | Upright telescope planes |

## MALTAscope

A simulated setup with 5 MALTA sensors; each with 512x512 pixels and
36.4µmx36.4µm pitch. The following datasets are available:

| Name | Beam | Particles/event | Comment |
| ---- | ---- | --------------- | ------- |
| 3GeV_electrons | 3GeV e- | 1 | |
