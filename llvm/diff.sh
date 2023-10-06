#!/bin/bash

VER="16.0.2"
DOWNLOAD="wget"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "$DIR"

echo "Calculating diff summary..."
diff_args="--recursive -x tools -x projects --strip-trailing-cr llvm-$VER/llvm llvm-$VER-ref/llvm"
diff=$(diff --brief $diff_args)

README_FILE=README.md
echo "Writing $README_FILE..."
echo "# Changes made to LLVM $VER for ReplayCache support" > "$README_FILE"
echo "" >> "$README_FILE"
echo "\`\`\`" >> "$README_FILE"
echo "$diff" >> "$README_FILE"
echo "\`\`\`" >> "$README_FILE"

echo "Done"
echo "If you want to view the full diff, run:"
echo "  diff $diff_args"
