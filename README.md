Proteus --- Pixel telescope reconstruction
==========================================

Proteus is a software to reconstruct and analyze data from beam telescopes. It
started as a fork of the [Judith][judith] software but has evolved into an
independent package.

Dependencies
------------

Proteus requires a C++14 compatible compiler, [CMake][cmake], [Eigen][eigen],
and [ROOT][root] for its core functionality. Optional components might require
additional software. A full list of dependencies and their minimum version
required is listed below.

*   A C++14-compatible compiler, e.g. gcc 5 or clang 3.4
*   [CMake][cmake] 2.8.12
*   [Eigen][eigen] 3.2.9
*   [ROOT][root] 6.08
*   [EUDAQ][eudaq] 1.7 (optional, for the EUDAQ reader)
*   Doxygen 1.6 (optional, for the documentation generation)
*   Sphinx (optional, for the documentation generation)
*   Breathe (optional, for the documentation generation)

On CERN lxplus machines you can setup a LCG release to provide a recent
compiler and compatible depedencies via the following command

    source /cvmfs/sft.cern.ch/lcg/views/LCG_88/x86_64-slc6-gcc7-opt/setup.sh

Building
--------

[![build status](https://gitlab.cern.ch/unige-fei4tel/proteus/badges/master/build.svg)](https://gitlab.cern.ch/unige-fei4tel/proteus/commits/master)

Use the following commands to build the software using [CMake][cmake] in a
separate build directory:

    mkdir build
    cd build
    cmake ..
    make

The resulting binaries will be located in `build/bin`. An additional
activation script is provided that updates the paths variables in the shell
environment. By sourcing it via

    source build/activate.sh

the `pt-...` binaries can be called directly without specifying its location
explicitly.

Build options
-------------

The following options can be set during the cmake configuration to activate
optional components, e.g.

    cmake -DPROTEUS_ENABLE_DOC=on ..

to enable building the documentation via the `doc` target. By default all
options are deactivated.

| Option             | Comment |
| :----------------- | :------ |
| PROTEUS_ENABLE_DOC | Enable documentation build target `doc`
| PROTEUS_USE_EUDAQ  | Build EUDAQ reader; set `EUDAQ_DIR` env variable to EUDAQ installation

Documentation
-------------

The documentation for Proteus is provided in the `rst` file format and can
either be read directly on the repository website or translated to HTMl
documents using the following command inside the build directory:

    make doc

Authors
-------

Proteus has seen contributions from (in alphabetical order):

*   Javier Bilbao de Mendizabal
*   Reina Camacho
*   Francesco Di Bello
*   Moritz Kiehn
*   Lingxin Meng
*   Marco Rimoldi
*   Branislav Ristic
*   Sergio Gonzalez Sevilla
*   Simon Spannagel
*   Morag William

The original Judith software was written by

*   Garrin McGoldrick
*   Matevž Červ
*   Andrej Gorišek

Citations
---------

Users of the software are expected to respect the rules of good
scientific practice. Publications that use this software should cite the
relevant publications:

*   G. McGoldrick et al, [NIM A765 140--145, Nov. 2014][paper2014]

License
-------

The software is distributed under the terms of the MIT license. The
documentation is distributed under the terms of the [CC-BY-4.0][ccby4] license.
The licenses can be found in the `LICENSE` file. Contributions are expected to
be submitted under the same license terms.

Proteus includes a copy of the [tinytoml][tinytoml]. [Tinytoml][tinytoml] is
distributed under the simplified BSD License.


[ccby4]: https://creativecommons.org/licenses/by/4.0/
[cmake]: http://www.cmake.org
[eigen]: http://eigen.tuxfamily.org
[eudaq]: http://eudaq.github.io
[judith]: https://github.com/gmcgoldr/judith
[paper2014]: http://dx.doi.org/10.1016/j.nima.2014.05.033
[root]: https://root.cern.ch
[tinytoml]: https://github.com/mayah/tinytoml
