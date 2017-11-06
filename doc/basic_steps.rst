Basic Steps
===========

This documents describes the the basic steps necessary to setup the software and
run basic telescope reconstruction for a typical beam telescope setup. Starting
from the raw hit data, the reconstruction finds and fits tracks using the
telescope planes and estimates the expected track position on one of more
devices-under-test for further analysis.

Setup on lxplus
---------------

Proteus requires `ROOT <https://root.cern.ch>`_ and a C++11 compiler. These
are not available by default on all lxplus machines. Appropriate version are
made available through the LCG releases. LCG releases *LCG_88* or higher
are recent enough.

On lxplus6 (Scientific Linux CERN 6) use

.. code-block:: console

    $ source /cvmfs/sft.cern.ch/lcg/views/LCG_88/x86_64-slc6-gcc62-opt/setup.sh

and on lxplus7 (CentOS Linux 7) use

.. code-block:: console

    $ source /cvmfs/sft.cern.ch/lcg/views/LCG_88/x86_64-centos7-gcc62-opt/setup.sh

to activate the required software. The following commands are then used to build
the software using `CMake <https://cmake.org/>`_ in a separate build directory:

.. code-block:: console

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

The resulting binaries will be located in ``build/bin``. An activation script is
provided that updates the necessary path variables in the shell environment.
After sourcing it via

.. code-block:: console

    $ source build/activate.sh

all binaries can be called directly without the need to specify their location
explicitly.

For a permanent installation on lxplus you can use the following installation
helper script:

.. code-block:: console

    $ ./scripts/lxplus_versioned_install.sh <INSTALL_BASE_DIR>

This builds proteus in a temporary directory and installs it into the
``<INSTALL_BASE_DIR>/<version>/<platform>`` directory together with a combined
``setup.sh`` script that activates both the appropriated LCG release and the
software itself.

Setup on a local machine
------------------------

On your local machine your are responsible for providing a C++11 compiler and a
ROOT installation. On most recent Linux distributions both are available via the
system package manager. The build commands should be identical.

Configuration
-------------

The software is configures via a set of configuration files which are described
in detail in :doc:`configuration_files`.

The device configuration file describes the telescope setup and possible
devices-under-test. It contains a list of sensors and their properties and
defines e.g. the sensor ids that are used to identify specific sensors. This
file should change only infrequently.

The setup geometry is defined in a separate file and can in principle change
from run to run. An initial alignment file with reasonable values close to the
physical setup must be provided. The alignment procedures will produce updated
files based also on the initial alignment.

Pixel masks remove specific pixels from the analysis, e.g. because they are
noisy or broken. The initial pixel mask can be empty.

Reconstruction Steps
--------------------

There are a few typical steps that need to be performed to reconstruct tracks
with a telescope:

* Hit preprocessing, e.g. FE-I4 to CCPD address mapping.
* Clustering, i.e. combining adjacent active pixel hits.
* Track finding, i.e. finding a set of clusters on multiple sensors that
  originate from the same particle track.
* Track fitting and extrapolation, i.e. estimating the track parameters
  from the cluster positions either globally or locally on selected sensors.
* Alignment

In contrast to other reconstruction packages, Proteus tries to perform as many
of these steps automatically or on-the-fly without an explicit processing step
or storage of intermediate results. This simplifies the interface at the expense
of some computing time.

Noise Scan
^^^^^^^^^^

The noise scan requires only the initial data and checks for noisy pixels in
selected sensors. It generates a new mask file that lists masked pixels. It can
be called as follows.

.. code-block:: console

    $ pt-noisescan -d device.toml -c noisescan.toml <INPUT> <OUTPUT_PREFIX>

This results in a ``<OUTPUT_PREFIX-mask.toml`` and a
``<OUTPUT_PREFIX-hists.root`` file that contains the new pixel mask and any
histograms created during the noise scan.

Alignment
^^^^^^^^^

.. todo:: Add documentation on alignment of telescope and dut


Tracking
^^^^^^^^

The tracking step takes the initial data and finds tracks using the data from
selected sensors. Which sensors are used (and which sensors are ignored) can
be configured in the analysis configuration file. For a typical analysis only
the telescope sensors are used to construct the tracks. Hit preprocessing and
hit clustering are performed on-the-fly.

.. code-block:: console

    $ pt-track -d device.toml -c analysis.toml <INPUT> <OUTPUT_PREFIX>

This creates a ``<OUTPUT_PREFIX>-data.root`` file with the full output data and
a ``<OUTPUT_PREFIX>-hists.root`` file with additional histograms.

Matching and Export
^^^^^^^^^^^^^^^^^^^

.. code-block:: console

    $ pt-match -d device.toml -c analysis.toml <TRACK_OUTPUT> <OUTPUT_PREFIX>
