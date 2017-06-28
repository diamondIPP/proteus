Proteus --- Pixel telescope reconstruction
==========================================

Proteus is a software to reconstruct and analyze beam telescope
measurements. It started as a fork of the [Judith][judith] software but
has since then evolved into an independent package. It requires a C++11
compatible compiler and [ROOT 5.34/36][root] or higher. The software is
tested with gcc 4.8.4, ROOT 5.34/36 on Scientific Linux CERN 6 and gcc
4.8.5, ROOT 6.08/04 and clang 3.4.2, ROOT 6.08/04 on CERN CentOS 7.

An introduction guide and additional information including the contribution
guide can be found in the `doc` directory. Example configurations can be found
in the `test` directory.

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
[judith]: https://github.com/gmcgoldr/judith
[paper2014]: http://dx.doi.org/10.1016/j.nima.2014.05.033
[root]: https://root.cern.ch
[tinytoml]: https://github.com/mayah/tinytoml
