#define RC_RR x31
//#define RC_START_REGION auipc RC_RR, 0
//#define RC_CLWB auipc RC_RR, 1
//#define RC_FENCE auipc RC_RR, 2
//#define RC_POWER_FAILURE_NEXT auipc RC_RR, 3
#define RC_START_REGION addi RC_RR, zero, 0
#define RC_CLWB addi RC_RR, zero, 1
#define RC_FENCE addi RC_RR, zero, 2
#define RC_POWER_FAILURE_NEXT addi RC_RR, zero, 3

.data
x: .skip 4
y: .skip 4

.text

.global rc_test
rc_test:
    RC_START_REGION
    li t1, 1

    // End the current region before a (conditional) branch,
    // the branch target will start a new region
    RC_FENCE

    j a

a:
    // This is a branch target, so it will start a new region
    RC_START_REGION

    // ...
    li a0, 123
    la t0, x
    sw a0, 0(t0)
    RC_CLWB

    // No need to end the region when jumping out of a branch target,
    // because the destination we jump to will already have a region boundary
    j end

b:
    // This is a branch target, so it will start a new region
    RC_START_REGION

    // ...
    li a0, 2
    la t0, y
    sw a0, 0(t0)
    RC_CLWB

    // We directly flow to the end of the conditional branch,
    // so we do not need to end the region here either

end:
    // Request a power failure for demonstration purposes
    RC_POWER_FAILURE_NEXT

    // Regular boundary
    RC_FENCE
    RC_START_REGION

    // ...
    addi a0, a0, 10

    // End the region before returning, as that is just a form of branching
    RC_FENCE

    ret
