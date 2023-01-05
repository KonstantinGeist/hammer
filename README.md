## What's Hammer?

Hammer is a programming language and runtime designed for building modern applications.

Work in progress.

# How to build and run

The Meson build system is required (see "Dependencies"). Initialize the project with:

    ./scripts/setup.sh

To build, run:

    ./scripts/make.sh

To run tests (requires Valgrind, see "Dependencies"):

    ./scripts/run-test.sh

# Dependencies

To install Meson on Ubuntu, run:

    sudo apt-get install meson

To install Valgrind on Ubuntu, run:

    sudo apt-get install valgrind