# This file defines GNU Make targets.

all: clean setup build check-impl test-impl
test: build test-impl
check: build check-impl

.PHONY: clean
clean:
	./scripts/clean.sh

.PHONY: setup
setup:
	./scripts/setup.sh

.PHONY: build
build:
	./scripts/build.sh

.PHONY: test-impl
test-impl:
	./scripts/run-tests.sh $(suite)

.PHONY: check-impl
check-impl:
	./scripts/check.sh
