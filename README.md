## What's Hammer?

Hammer is a programming language and runtime designed for building modern applications.

Work in progress.

# Requirements

#### Ubuntu

To install GCC (tested version: 9.4.0) and GNU make (tested version: 4.2.1), run:

    sudo apt install build-essential

To install Meson (tested version: 0.53.2+), run:

    sudo apt-get install meson

To install Valgrind (tested version: 3.15.0+), run:

    sudo apt-get install valgrind

To install CppCheck (tested version: 1.90+), run:

    sudo apt-get install cppcheck

# How to build and run

#### Ubuntu

To build, run:

    git clone git@github.com:KonstantinGeist/hammer
    cd hammer
    make

This will build the project, run tests, execute linting and create a `hammer` executable in the `bin` folder.

To run unit tests only:

    make test

To run a specific test suite:

    make test suite=strings

To execute linting only:

    make check

To clean all build artifacts:

    make clean