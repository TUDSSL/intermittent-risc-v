all: pass-utils test-pass

clean: pass-utils-clean test-pass-clean

# Common libraries
pass-utils:
	$(MAKE) -C common/PassUtils

pass-utils-clean:
	$(MAKE) clean -C common/PassUtils

# Transformations
test-pass: pass-utils
	$(MAKE) -C TestPass

test-pass-clean:
	$(MAKE) clean -C TestPass

