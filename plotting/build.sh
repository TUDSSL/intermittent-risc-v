#!/bin/bash

function run_ipynb {
    notebook="$1"
    echo "Running notebook: $notebook"
    #jupyter nbconvert --execute --to html "$notebook"
    jupyter nbconvert --execute --to notebook "$notebook"
}

# Run all the notebooks (that generate the plots)
run_ipynb PrimaryStoryLine.ipynb

# Copy all the notebooks to a 'notebooks' directory
#mkdir -p notebooks
#mv ./*.html notebooks/

