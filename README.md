## What's Hammer?

Hammer is a programming language and runtime designed for building modern applications.

Work in progress.

# How to build and run

The Meson build system is required (see "Dependencies"). Initialize the project with:

    ./scripts/setup.sh

To build, run:

    ./scripts/make.sh

To run tests (requires Valgrind, see "Dependencies"):

    ./scripts/run-tests.sh

# Dependencies

To install Meson (tested version: 0.53.2+) on Ubuntu, run:

    sudo apt-get install meson

To install Valgrind (tested version: 3.15.0+) on Ubuntu, run:

    sudo apt-get install valgrind
