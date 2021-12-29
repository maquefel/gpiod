# Requirements:
#  /bin/bash as SHELL
export SHELL = /bin/bash

all: tests

TESTS_BATS = \
	gpiod_hook_tests.bats \
	gpiod_sysfs_tests.bats \
	gpiod_uapi_tests.bats

.PHONY: tests
tests: $(TESTS_BATS)
	for t in $(TESTS_BATS) ; do \
		bats $$(realpath $$t) ; \
	done
