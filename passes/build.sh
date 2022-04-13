#!/bin/bash

function build_pass {
    pass_dir="$1"

    echo "Building pass: $pass_dir"
    pushd "$pass_dir"
    make -j $(nproc)
    popd
}

build_pass "test"
