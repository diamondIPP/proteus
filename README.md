Proteus --- Pixel telescope reconstruction
==========================================

Proteus is a software to reconstruct and analyze beam telescope
measurements. It started as a fork of the [Judith][judith] software but
has since then evolved into an independent package. It requires a
C++11 compatible compiler and [ROOT 5.34/36][root] or higher.

[![build status](https://gitlab.cern.ch/unige-fei4tel/proteus/badges/master/build.svg)](https://gitlab.cern.ch/unige-fei4tel/proteus/commits/master)

Building
--------

Use the following commands to build the software using [CMake][cmake] in a
separate build directory:

    mkdir build
    cd build
    cmake ..
    cmake --build .

The resulting binaries will be located in `build/bin`. An additional
activation script is provided that updates the paths variables in the shell
environment. By sourcing it via

    source build/activate.sh

the `pt-...` binaries can be called directly without specifying its location
explicitly.

The `test` directory contains example configurations.

A introduction guide and additional information including the coding and
contribution guide can be found in the `doc/` directory.

Authors
-------

The original Judith software was written by

*   Garrin McGoldrick
*   Matevž Červ
*   Andrej Gorišek

The Proteus fork has seen contributions from (in alphabetical order):

*   Javier Bilbao de Mendizabal
*   Reina Camacho
*   Francesco Di Bello
*   Moritz Kiehn
*   Lingxin Meng
*   Marco Rimoldi
*   Branislav Ristic
*   Sergio Gonzalez Sevilla

Citations
---------

Users of the software are expected to respect the rules of good
scientific practice. Publications that use this software should cite the
relevant publications:

*   G. McGoldrick et al, [NIM A765 140--145, Nov. 2014][paper2014]

License
-------

This software is distributed under the terms of the MIT license.

    Copyright (c) 2014 The Judith authors
    Copyright (c) 2014-2016 The Proteus authors

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
    BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

Contributions are expected to be submitted under the same license terms.

Proteus includes a copy of the [tinytoml][tinytoml]. [Tinytoml][tinytoml] is
distributed under the simplified BSD License.


[cmake]: http://www.cmake.org
[judith]: https://github.com/gmcgoldr/judith
[paper2014]: http://dx.doi.org/10.1016/j.nima.2014.05.033
[root]: https://root.cern.ch
[tinytoml]: https://github.com/mayah/tinytoml
