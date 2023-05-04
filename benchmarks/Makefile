build = $(MAKE) -f make-build.mk
run = $(MAKE) -f make-run.mk

build:
	@echo "Building all benchmarks"
	OPT_LEVEL="-O3" $(build) all

run:
	@echo "Running all benchmarks using all configurations"
	$(run) all

analyze:
	@echo "Analyzing results"
	cd ../plotting && ./build.sh

clean-build:
	$(build) clean

clean-run:
	$(run) clean

clean: clean-build clean-run

.PHONY: build run analyze clean-build clean-run clean
