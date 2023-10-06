build:
	@echo "Building all benchmarks"
	OPT_LEVEL="-O3" $(MAKE) -f make-build.mk all

run: build
	@echo "Running all benchmarks using all configurations"
	$(MAKE) -f make-run.mk all

analyze:
	@echo "Analyzing results"
	cd ../plotting && ./build.sh

clean-build:
	$(MAKE) -f make-build.mk clean

clean-run:
	$(MAKE) -f make-run.mk clean

clean: clean-build clean-run

.PHONY: build run analyze clean-build clean-run clean
