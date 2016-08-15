<TO BE RENAMED> --- beam telescope reconstruction and analysis
==============================================================

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
explicitly.

Additional information including the coding and contribution guide can be found
in the `doc/` directory.


[cmake]: http://www.cmake.org
