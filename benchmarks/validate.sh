conditional_branch_instrs=("beq" "bne" "blt" "bge" "bltu" "bgeu")
store_instrs=("sw" "sh" "sb")
call_instrs=("jalr" "jal")
call_register="ra"
replaycache_instr_base="li	t6, "

# ReplayCache instructions
start_region_instr="$replaycache_instr_base""0"
start_region_return_instr="$replaycache_instr_base""1"
start_region_extension_instr="$replaycache_instr_base""2"
start_region_branch_instr="$replaycache_instr_base""3"
start_region_branch_dest_instr="$replaycache_instr_base""4"
fence_instr="$replaycache_instr_base""7"
clwb_instr="$replaycache_instr_base""8"

# Command line argument: file to read.
read_file="benchmarks/$1/build-replay-cache/$1.dis"

# Globals
expect_clwb_next_line=false
expect_fence_next_line=false
prev_line=""
num_checks_failed=0
branch_targets=()

while IFS= read -r line; do

    #### CLWB CHECK ####
    # If we expect a CLWB now, check if it is there.
    # If not, signal an error.
    if [ "$expect_clwb_next_line" = true ]; then
        if [[ ! $line =~ $clwb_instr ]]; then
            echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
            echo "Missing CLWB for store:"
            echo "$prev_line"
            echo ""
            num_checks_failed=$((num_checks_failed + 1))
        fi

        expect_clwb_next_line=false
    fi

    #### CLWB CHECK ####
    # Check for any store instructions. If one is found, expect CLWB at next line.
    for store_instr in "${store_instrs[@]}"
    do
        if [[ $line =~ $store_instr ]]; then
            expect_clwb_next_line=true
            break
        fi
    done

    #### BRANCH CHECK ####
    # Check for a START_REGION instruction in front of a branch.
    # Can be START_REGION_BRANCH or START_REGION_BRANCH_DEST (in case the branch instr is also a destination).
    for branch_instr in "${conditional_branch_instrs[@]}"
    do
        if [[ $line =~ $branch_instr ]]; then
            if [[ ! $prev_line =~ $start_region_branch_instr && ! $prev_line =~ $start_region_branch_dest_instr ]]; then
                echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
                echo "Missing region for branch instruction:"
                echo "$line"
                echo ""
                num_checks_failed=$((num_checks_failed + 1))
            fi

            # Add branch target address to array for later checking.
            branch_target=${line#*x}
            branch_target=${branch_target%" "*}
            branch_targets+=($branch_target)

            break
        fi
    done

    #### CALL RETURN CHECK ####
    # Check if there is a region boundary when it is expected.
    if [ "$expect_fence_next_line" = true ]; then
        if [[ ! $line =~ $fence_instr ]]; then
            echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
            echo "Missing region boundary after call:"
            echo "$prev_line"
            echo ""
            num_checks_failed=$((num_checks_failed + 1))
        fi

        expect_fence_next_line=false
    fi
    
    #### CALL RETURN CHECK ####
    # Expect region boundary after function call.
    for call_instr in "${call_instrs[@]}"
    do
        if [[ $line =~ $call_instr && $line =~ $call_register ]]; then
            expect_fence_next_line=true
            break
        fi
    done

    # Stop at .sbss section, this is not where we want to validate.
    # We can ignore the .debug_str section after it as well.
    if [[ $line = "Disassembly of section .sbss:" ]]; then
        break
    fi

    prev_line=$line
done < $read_file

#### BRANCH TARGETS CHECK ####
# Check if all branch targets target a region boundary.
# Must be done after processing all branches because targets may
# be located above the branch instructions.s
for target in "${branch_targets[@]}"
do
    target_line=$(grep -E -i "$target:" $read_file)

    if [[ ! $target_line =~ $fence_instr ]]; then
        echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
        echo "Branch destination is not a FENCE instruction:"
        echo "$target_line"
        echo ""
        num_checks_failed=$((num_checks_failed + 1))
    fi
done

# Feedback to the user how many checks failed, if any.
echo ""
if [[ $num_checks_failed -gt 0 ]]; then
    echo -e "\033[91m------- VALIDATION FAILED -------\033[0m"
    echo "$num_checks_failed check(s) failed"
else
    echo -e "\033[92m------- VALIDATION SUCCESSFUL -------\033[0m"
    echo -e "All checks passed"
fi
