## What's Hammer?

Hammer is a programming language and runtime designed for building modern applications.

Work in progress.

# Requirements

#### Ubuntu

To install Meson (tested version: 0.53.2+), run:

    sudo apt-get install meson

To install Valgrind (tested version: 3.15.0+), run:

    sudo apt-get install valgrind

To install CppCheck (tested version: 1.90+), run:

    sudo apt-get install cppcheck

# How to build and run

To build, run:

    ./scripts/make.sh

This will build the project, run tests, run a linter, and create a `hammer` executable in the `bin` folder.

Also, see other scripts in the `scripts` folder.
