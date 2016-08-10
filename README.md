Judith --- beam telescope reconstruction and analysis
=====================================================

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

the `Judith` binary can be called directly without specifying its location
explicitely.

The `test` directory contains example configurations and additional information
on how to use them.


[cmake]: http://www.cmake.org
