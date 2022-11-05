## What's Hammer?

Hammer is a programming language and runtime designed for building applications with modern architecture:
* strong/static typing
* built-in dependency injection/inversion of control
* support for Domain-Driven Design (entities, value objects, bounded contexts)
* support for units of work (transactions)
* request/response-oriented pipeline
* built-in concurrency
* minimum dependencies, lightweight
* batteries included for typical workloads

Work in progress.

# How to build

The Meson build system is required. Initialize the project with:

    ./scripts/setup.sh

To build, run:

    ./scripts/make.sh

To run tests:

    ./scripts/run-test.sh