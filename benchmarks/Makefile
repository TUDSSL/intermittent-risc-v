build:
	@echo "Building all benchmarks"
	OPT_LEVEL="-O3" $(MAKE) -f make-build.mk all

run:
	@echo "Running all benchmarks using all configurations"
	$(MAKE) -f make-run.mk all

analyze:
	@echo "Analyzing results"
	# TODO

.PHONY: build run analyze
