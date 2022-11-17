## What's Hammer?

Hammer is a programming language and runtime designed for building applications with modern architecture.
The following features will be implemented:
* built-in dependency injection/inversion of control
* support for Domain-Driven Design (entities, value objects, bounded contexts)
* support for layered architecture: layers, layer dependency tracking
* support for units of work (transactions)
* request/response-oriented pipeline
* built-in concurrency
* minimum dependencies, lightweight
* high fault tolerance
* batteries included for typical workloads
* strong/static typing
* built-in tooling: unit testing, fuzzing
* automatic memory management
* sandboxing
* multitenancy support
* event handling & queues
* saga support
* ease of debugging, inspectability
  and more.

Work in progress.

# How to build

The Meson build system is required. Initialize the project with:

    ./scripts/setup.sh

To build, run:

    ./scripts/make.sh

To run tests:

    ./scripts/run-test.sh