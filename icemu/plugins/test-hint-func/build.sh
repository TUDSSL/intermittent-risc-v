#!/bin/bash

c-to-llvmir test.c test.ll
pass-apply "--cache-no-writeback-hint-debug --cache-no-writeback-hint" test.ll test-cache.ll
llvmir-to-elf test-cache.ll

rm norm-test.ll
