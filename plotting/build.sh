#!/bin/bash

set -e

function run_ipynb {
    notebook="$1"
    echo "Running notebook: $notebook"
    jupyter nbconvert --to notebook --execute "$notebook"
}

# Run notebook actions on each notebook in the current directory
for notebook in ./*.ipynb; do
    # Skip if this is a converted (executed) notebook
    if [[ "$notebook" == *.nbconvert.ipynb ]]; then
        continue
    fi
    run_ipynb "$notebook"
done
