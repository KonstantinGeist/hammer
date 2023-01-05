## What's Hammer?

Hammer is a programming language and runtime designed for building modern applications.

Work in progress.

# Requirements

#### Ubuntu

To install GCC and GNU make, run:

    sudo apt install build-essential

To install Meson, run:

    sudo apt-get install meson

To install Valgrind, run:

    sudo apt-get install valgrind

To install CppCheck, run:

    sudo apt-get install cppcheck

Tested versions:
* Meson: 0.53.2
* Valgrind: 3.15.0
* CppCheck: 1.90
* GCC: 9.4.0
* GNU make: 4.2.1

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