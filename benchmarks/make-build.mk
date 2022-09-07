.DEFAULT_GOAL := all

# Include the benchmark configurations
include common.mk

define generate_benchmark_build_targets
$(1)-build-$(2):
		@echo "$(HLB)Building benchmark '$(1)' target '$(2)'$(HLE)"
		cd $(1) && benchmark-build $(2)

TARGETS_BUILD_$(1) += $(1)-build-$(2)
TARGETS_BUILD_$(2) += $(1)-build-$(2)
TARGETS_BUILD_ALL += $(1)-build-$(2)
endef


define generate_benchmark_group_targets
build-targets-$(1): $(TARGETS_BUILD_$(1))
	@echo "$(HLB)Done building configurations for $(1)$(HLE)"

show-targets-$(1):
	@echo "$(TARGETS_BUILD_$(1))"

TARGETS += show-targets-$(1) build-target-$(1)
endef

# Generate the benchmark configurations
$(foreach bench,$(BENCHMARKS), $(foreach config, $(BUILD_CONFIGURATIONS), \
	$(eval $(call generate_benchmark_build_targets,$(bench),$(config)))))

# Generate the benchmark group targets
$(foreach bench,$(BENCHMARKS),$(eval $(call generate_benchmark_group_targets,$(bench))))

# Generate the build configurations group targets
$(foreach target,$(BUILD_CONFIGURATIONS),$(eval $(call generate_benchmark_group_targets,$(target))))

# Make all the default target
all: build

build: $(TARGETS_BUILD_ALL)
	@echo "$(HLS)Done building all benchmarks$(HLE)"

show-benchmarks:
		@echo "Benchmarks: $(BENCHMARKS)"

show-build-targets:
		@echo "Build targets: $(TARGETS_BUILD_ALL)"

show-targets:
		@echo "Targets: $(TARGETS)"

.PHONY: all build show-benchmarks show-build-targets show-targets \
		$(TARGETS_BUILD_ALL) $(TARGETS)
