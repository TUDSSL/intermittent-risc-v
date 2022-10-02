.DEFAULT_GOAL := all

# Include the benchmark configurations
include common.mk

NACHO_NAIVE_CONFIGURATIONS := \
  		nacho-naive+256+2 \
  		nacho-naive+256+4 \
  		nacho-naive+512+2 \
  		nacho-naive+512+4 \
  		nacho-naive+1024+2 \
  		nacho-naive+1024+4

NACHO_PW_CONFIGURATIONS := \
  		nacho-pw+256+2 \
  		nacho-pw+256+4 \
  		nacho-pw+512+2 \
  		nacho-pw+512+4 \
  		nacho-pw+1024+2 \
  		nacho-pw+1024+4

NACHO_PW_ST_CONFIGURATIONS := \
  		nacho-pw-st+256+2 \
  		nacho-pw-st+256+4 \
  		nacho-pw-st+512+2 \
  		nacho-pw-st+512+4 \
  		nacho-pw-st+1024+2 \
  		nacho-pw-st+1024+4

NACHO_PW_STCONT_CONFIGURATIONS := \
  		nacho-pw-stcont+256+2 \
  		nacho-pw-stcont+256+4 \
  		nacho-pw-stcont+512+2 \
  		nacho-pw-stcont+512+4 \
  		nacho-pw-stcont+1024+2 \
  		nacho-pw-stcont+1024+4

NACHO_CLANK_CONFIGURATIONS += \
  		nacho-clank+256+2 \
  		nacho-clank+256+4 \
  		nacho-clank+512+2 \
  		nacho-clank+512+4 \
  		nacho-clank+1024+2 \
  		nacho-clank+1024+4

PROWL_CONFIGURATIONS += \
  		prowl+256+2 \
  		prowl+512+2 \
  		prowl+1024+2

CLANK_CONFIGURATIONS += \
  		clank

PLAINC_CONFIGURATIONS += \
  		plain-c

ALL_CONFIGURATIONS += \
		$(NACHO_NAIVE_OPTIONS) \
		$(NACHO_PW_CONFIGURATIONS) \
		$(NACHO_PW_ST_CONFIGURATIONS) \
		$(NACHO_CLANK_CONFIGURATIONS) \
		$(PROWL_CONFIGURATIONS) \
		$(CLANK_CONFIGURATIONS) \
		$(PLAINC_CONFIGURATIONS)


#generate_target_with_options(uninstrumented, NACHO_NAIVE_OPTIONS)
# $(3)+0+0 is for the on-duration and checkpoint period (0 means cont, i.e., no power failures and no periodic checkpoints)
define generate_run_target_configurations
$(1)-$(2)-run-$(3):
		@echo "$(HLB)Running benchmark '$(1)' build configuration '$(2)' run configuration '$(3)' $(HLE)"
		cd $(1)/build-$(2) && benchmark-run $(3)+0+0 $(1).elf $(2)
TARGETS += $(1)-$(2)-run-$(3)
TARGETS-$(1) += $(1)-$(2)-run-$(3)
TARGETS-$(2) += $(1)-$(2)-run-$(3)
TARGETS-$(3) += $(1)-$(2)-run-$(3)
endef

define generate_show_targets
show-targets-$(1):
	@echo "$(TARGETS-$(1))"

run-targets-$(1): $(TARGETS-$(1))
	@echo "$(HLB)Done running $(1) targets$(HLE)"
endef


define generate_run_targets_build_configuration

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(NACHO_NAIVE_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(NACHO_PW_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(NACHO_PW_ST_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(NACHO_PW_STCONT_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(NACHO_CLANK_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(PROWL_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(CLANK_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

$(foreach bench,$(BENCHMARKS), $(foreach run-config, $(PLAINC_CONFIGURATIONS), \
	$(eval $(call generate_run_target_configurations,$(bench),$(1),$(run-config)))))

endef

# Generate all the possible configurations
$(foreach build-config, $(BUILD_CONFIGURATIONS), \
	$(eval $(call generate_run_targets_build_configuration,$(build-config))))


# Generate power failure options
# 1 = benchmark
# 2 = compile config
# 3 = run target
# 4 = on-duration
define generate_pf_run_target_configurations
$(1)-$(2)-pf-run-$(3)+$(4):
		@echo "$(HLB)Running power failure benchmark '$(1)' build configuration '$(2)' run configuration '$(3)' on duration '$(4)' $(HLE)"
		cd $(1)/build-$(2) && benchmark-run $(3)+$(4)+$(CHECKPOINT_PERIOD) $(1).elf $(2)

PF_TARGETS += $(1)-$(2)-pf-run-$(3)+$(4)
endef

#
# Power-failure build configurations
#

# Nacho PW ST Cont
$(foreach bench,$(BENCHMARKS), $(foreach on-duration,$(ON_DURATIONS), \
	$(eval $(call generate_pf_run_target_configurations,$(bench),uninstrumented,nacho-pw-stcont+512+2,$(on-duration)))))

# Prowl
$(foreach bench,$(BENCHMARKS), $(foreach on-duration,$(ON_DURATIONS), \
	$(eval $(call generate_pf_run_target_configurations,$(bench),uninstrumented,prowl+512+2,$(on-duration)))))

# Clank
# TODO

show-pf-targets:
	@echo "$(PF_TARGETS)"

run-pf-targets: $(PF_TARGETS)
	@echo "$(HLB)Done running power failure targets$(HLE)"

# Show-targets
$(foreach target, $(BENCHMARKS), $(eval $(call generate_show_targets,$(target))))
$(foreach target, $(ALL_CONFIGURATIONS), $(eval $(call generate_show_targets,$(target))))

# Target groups
define generate_target_group
TARGETS-$(1) += $(TARGETS-$(2))
endef

$(foreach target, $(NACHO_NAIVE_CONFIGURATIONS), $(eval $(call generate_target_group,nacho-naive,$(target))))
show-targets-nacho-naive:
	@echo "$(TARGETS-nacho-naive)"

run-targets-nacho-naive: $(TARGETS-nacho-naive)
	@echo "$(HLB)Done running nacho-naive targets$(HLE)"

$(foreach target, $(NACHO_PW_CONFIGURATIONS), $(eval $(call generate_target_group,nacho-pw,$(target))))
show-targets-nacho-pw:
	@echo "$(TARGETS-nacho-pw)"

run-targets-nacho-pw: $(TARGETS-nacho-pw)
	@echo "$(HLB)Done running nacho-pw targets$(HLE)"

$(foreach target, $(NACHO_PW_ST_CONFIGURATIONS), $(eval $(call generate_target_group,nacho-pw-st,$(target))))
show-targets-nacho-pw-st:
	@echo "$(TARGETS-nacho-pw-st)"

run-targets-nacho-pw-st: $(TARGETS-nacho-pw-st)
	@echo "$(HLB)Done running nacho-pw-st targets$(HLE)"

$(foreach target, $(NACHO_PW_STCONT_CONFIGURATIONS), $(eval $(call generate_target_group,nacho-pw-stcont,$(target))))
show-targets-nacho-pw-stcont:
	@echo "$(TARGETS-nacho-pw-stcont)"

run-targets-nacho-pw-stcont: $(TARGETS-nacho-pw-stcont)
	@echo "$(HLB)Done running nacho-pw-stcont targets$(HLE)"

$(foreach target, $(NACHO_CLANK_CONFIGURATIONS), $(eval $(call generate_target_group,nacho-clank,$(target))))
show-targets-nacho-clank:
	@echo "$(TARGETS-nacho-clank)"

run-targets-nacho-clank: $(TARGETS-nacho-clank)
	@echo "$(HLB)Done running nacho-clank targets$(HLE)"

$(foreach target, $(PROWL_CONFIGURATIONS), $(eval $(call generate_target_group,prowl,$(target))))
show-targets-prowl:
	@echo "$(TARGETS-prowl)"

run-targets-prowl: $(TARGETS-prowl)
	@echo "$(HLB)Done running prowl targets$(HLE)"

# Clank has only one config called 'clank' which is already a target
#$(foreach target, $(CLANK_CONFIGURATIONS), $(eval $(call generate_target_group,clank,$(target))))
#show-targets-clank:
#	@echo "$(TARGETS-clank)"
#
# PlainC has only one config called 'plainc' which is already a target


all: $(TARGETS)

show-all-configurations:
	@echo "$(ALL_CONFIGURATIONS)"

show-targets:
	@echo "$(TARGETS)"

show-benchmarks:
	@echo "$(BENCHMARKS)"

clean:
	@echo "$(HLB)Removing logs directory$(HLE)"
	rm -rf ./logs

.PHONY: all clean \
	show-targets show-benchmarks \
	show-targets-nacho-naive show-targets-nacho-pw show-targets-nacho-clank show-targets-plain-c show-targets-prowl \
	run-targets-nacho-naive run-targets-nacho-pw run-targets-nacho-clank run-targets-plain-c run-targets-prowl \
	show-pf-targets run-pf-targets \
	$(TARGETS)
