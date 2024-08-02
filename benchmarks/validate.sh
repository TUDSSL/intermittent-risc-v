#!/bin/bash

########################################################################################
# Validates the compiler-outputted assembly code according to the ReplayCache rules.
#
# The following rules are checked:
# - No registers must be reused after store
# - All store instructions must be followed by a CLWB
# - Conditional branch instructions must have a region boundary before
# - Branch instructions must be followed by a region boundary or an unconditional jump
# - There must be a region boundary after every function call
# - Every START_REGION_BRANCH must have a following branch instruction
# - Every START_REGION_BRANCH_DEST must have a branch pointing to that region boundary
# - Every branch target (including fallthrough) must target a region boundary
########################################################################################

conditional_branch_instrs=("beq" "bne" "blt" "bge" "bltu" "bgeu" "bgtz" "blez")
store_instrs=("sw" "sh" "sb")
call_instrs=("jalr")
call_register="ra"
replaycache_instr_base="li	t6, "

# ReplayCache instructions
start_region_instr="$replaycache_instr_base""1"
start_region_return_instr="$replaycache_instr_base""2"
start_region_extension_instr="$replaycache_instr_base""3"
start_region_branch_instr="$replaycache_instr_base""4"
start_region_branch_dest_instr="$replaycache_instr_base""5"
start_region_stack_spill_instr="$replaycache_instr_base""6"
fence_instr="$replaycache_instr_base""7"
clwb_instr="$replaycache_instr_base""8"

# Command line argument: file to read.
read_file="benchmarks/$1/build-replay-cache-$2/$1.dis"

# Globals
expect_clwb_next_line=false
expect_fence_next_line=false
prevprev_line=""
prev_line=""
num_checks_failed=0
branch_targets=()
live_store_registers=()
lineno=0

while IFS= read -r line; do
    lineno=$((lineno + 1))
    
    if [ -z "$line" ]; then
        continue
    fi

    if [[ $line =~ ">:" ]]; then
        continue
    fi

    # Stop at .sbss section, this is not where we want to validate.
    # We can ignore the .debug_str section after it as well.
    if [[ $line = "Disassembly of section .sbss:" || $line = "Disassembly of section .sdata:" ]]; then
        break
    fi

    # Empty live registers at end of region.
    if [[ $line =~ $start_region_instr || $line =~ $start_region_return_instr || $line =~ $start_region_extension_instr || $line =~ $start_region_branch_instr || $line =~ $start_region_branch_dest_instr || $line =~ $start_region_stack_spill_instr ]]; then
        live_store_registers=()
    fi

    #### REGISTER REDEFINITION CHECK ####
    # Check if a register is redefined after a store instruction.
    register_used=${line##*"	"}
    register_used=${register_used%%,*}
    for live_store_register in "${live_store_registers[@]}"
    do
        if [ "$live_store_register" = "$register_used" ]; then
            found=false
            # Ignore branches.
            for instr in "${conditional_branch_instrs[@]}"
            do
                if [[ $line =~ $instr ]]; then
                    found=true
                    break
                fi
            done
            # Ignore store instructions.
            for instr in "${store_instrs[@]}"
            do
                if [[ $line =~ "$instr	" ]]; then
                    found=true
                    break
                fi
            done

            if [[ $found = false ]]; then
                echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
                echo "Register $register_used used after store: $read_file:$lineno"
                echo "$line"
                echo ""
                num_checks_failed=$((num_checks_failed + 1))
                failed=true
                break
            fi
        fi
    done

    #### CLWB CHECK ####
    # If we expect a CLWB now, check if it is there.
    # If not, signal an error.
    if [ "$expect_clwb_next_line" = true ]; then
        if [[ ! $line =~ $clwb_instr ]]; then
            echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
            echo "Missing CLWB for store: $read_file:$((lineno - 1))"
            echo "$prev_line"
            echo ""
            num_checks_failed=$((num_checks_failed + 1))
        fi

        expect_clwb_next_line=false
    fi

    #### CLWB CHECK ####
    # Check if there is a lone CLWB without a store.
    if [[ $line =~ $clwb_instr ]]; then
        found=false
        for store_instr in "${store_instrs[@]}"
        do
            if [[ $prev_line =~ "$store_instr	" ]]; then
                found=true
                break
            fi
        done

        if [[ $found = false ]]; then
            echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
            echo "CLWB without store: $read_file:$((lineno))"
            echo "$prev_line"
            echo ""
            num_checks_failed=$((num_checks_failed + 1))
        fi
    fi

    #### STORE CHECK ####
    # Check for any store instructions. If one is found, expect CLWB at next line.
    # Also add the store register to the live registers array for checking.
    for store_instr in "${store_instrs[@]}"
    do
        if [[ $line =~ "$store_instr	" ]]; then
            # Whenever a store is found, we expect a CLWB instruction next.
            expect_clwb_next_line=true
            stored=false

            # Extract store register from line.
            store_register=${line##*"	"}
            store_register=${store_register%%,*}

            # Check if register is already used in a store.
            for live_store_register in "${live_store_registers[@]}"
            do
                if [ "$live_store_register" = "$store_register" ]; then
                    stored=true
                    break
                fi
            done
            
            # If it was not used yet, add register to list.
            if [[ $stored = false ]]; then
                live_store_registers+=($store_register)
            fi

            # Extract address register from line.
            store_register=${line##*"("}
            store_register=${store_register%%")"*}

            # Check if register is already used in a store.
            for live_store_register in "${live_store_registers[@]}"
            do
                if [ "$live_store_register" = "$store_register" ]; then
                    stored=true
                    break
                fi
            done
            
            # If it was not used yet, add register to list.
            if [[ $stored = false ]]; then
                live_store_registers+=($store_register)
            fi

            break
        fi
    done

    # #### BRANCH CHECK ####
    # # Check for a START_REGION instruction in front of a branch.
    # # Can be START_REGION_BRANCH or START_REGION_BRANCH_DEST (in case the branch instr is also a destination).
    # for branch_instr in "${conditional_branch_instrs[@]}"
    # do
    #     if [[ $line =~ $branch_instr ]]; then
    #         if [[ ! $prev_line =~ $start_region_branch_instr && ! $prev_line =~ $start_region_branch_dest_instr ]]; then
    #             echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
    #             echo "Missing region for branch instruction: $read_file:$lineno"
    #             echo "$line"
    #             echo ""
    #             num_checks_failed=$((num_checks_failed + 1))
    #         fi

    #         # Add branch target address to array for later checking.
    #         branch_target=${line#*x}
    #         branch_target=${branch_target%" "*}
    #         branch_targets+=($branch_target)

    #         break
    #     fi
    # done

    #### ELSE BRANCH CHECK ####
    # If the previous instruction was a conditional branch, check if this instruction is either a fence or an unconditional jump.
    for branch_instr in "${conditional_branch_instrs[@]}"
    do
        if [[ $prev_line =~ $branch_instr ]]; then
            if [[ ! ($line =~ $start_region_instr || $line =~ $start_region_return_instr || $line =~ $start_region_extension_instr || $line =~ $start_region_branch_instr || $line =~ $start_region_branch_dest_instr || $line =~ $start_region_stack_spill_instr) && ! $line =~ "j	" ]]; then
                echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
                echo "Missing fall-through region: $read_file:$lineno"
                echo "$line"
                echo ""
                num_checks_failed=$((num_checks_failed + 1))
                break
            fi

            if [[ $line =~ "j	" ]]; then
                # Add branch target address to array for later checking.
                branch_target=${line#*x}
                branch_target=${branch_target%" "*}
                branch_targets+=($branch_target)
            fi
        fi
    done

    #### CALL RETURN CHECK ####
    # Check if there is a region boundary when it is expected.
    if [ "$expect_fence_next_line" = true ]; then
        if [[ ! ($line =~ $start_region_instr || $line =~ $start_region_return_instr || $line =~ $start_region_extension_instr || $line =~ $start_region_branch_instr || $line =~ $start_region_branch_dest_instr || $line =~ $start_region_stack_spill_instr) ]]; then
            echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
            echo "Missing region boundary after call: $read_file:$((lineno - 1))"
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

    # Check if every BRANCH_DEST region has a corresponding jump to that address, or a corresponding
    # branch instruction in case of fallthrough.
    if [[ $line =~ $start_region_branch_dest_instr ]]; then
        found=false

        for branch_instr in "${conditional_branch_instrs[@]}"
        do
            if [[ $prev_line =~ $branch_instr ]]; then
                found=true
                break
            fi
        done

        if [[ $found == false ]]; then
            instr_address=${line%%:*}
            target_line=$(grep -E -i "0x$instr_address" $read_file)

            if [[ ! $target_line =~ $instr_address ]]; then
                echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
                echo "Branch destination region without jump: $read_file:$((lineno))"
                echo "$line"
                echo ""
                num_checks_failed=$((num_checks_failed + 1))
            fi
        fi
    fi

    # # Check if every BRANCH region has a corresponding branch instruction.
    # if [[ $prev_line =~ $start_region_branch_instr ]]; then
    #     found=false

    #     for branch_instr in "${conditional_branch_instrs[@]}"
    #     do
    #         if [[ $line =~ $branch_instr ]]; then
    #             found=true
    #             break
    #         fi
    #     done

    #     if [[ $found == false ]]; then
    #         echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
    #         echo "Branch region without branch instruction: $read_file:$((lineno - 1))"
    #         echo "$prev_line"
    #         echo ""
    #         num_checks_failed=$((num_checks_failed + 1))
    #     fi
    # fi

    prevprev_line=$prev_line
    prev_line=$line
done < $read_file

#### BRANCH TARGETS CHECK ####
# Check if all branch targets target a region boundary.
# Must be done after processing all branches because targets may
# be located above the branch instructions.s
for target in "${branch_targets[@]}"
do
    target_line=$(grep -E -i "$target:" $read_file)

    if [[ ! ($target_line =~ $start_region_instr || $target_line =~ $start_region_return_instr || $target_line =~ $start_region_extension_instr || $target_line =~ $start_region_branch_instr || $target_line =~ $start_region_branch_dest_instr || $target_line =~ $start_region_stack_spill_instr) ]]; then
        echo -e "\033[91m--- CHECK FAILED ($num_checks_failed) ---\033[0m"
        echo "Branch destination is not a region boundary instruction: $read_file"
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
