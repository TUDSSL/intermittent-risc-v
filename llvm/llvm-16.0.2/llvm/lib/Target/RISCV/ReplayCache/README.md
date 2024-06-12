# REPLAY CACHE

## Analysis passes

### Region analysis
Stores the position of regions in a linked list. Allows iteration of regions through the linked list.  
Allows easy iteration through instructions inside every region.

## Transformation passes

### Order
1. Initial regions
2. Register pressure-aware region partitioning
3. *Register allocation*
4. First region repair
5. *other passes*
6. Second region repair
7. Stack spill handling
8. CLWB insertion
9. *Branch Relaxation*
10. Third region repair

### Initial regions
Flag the places in the code for the initial regions, around function calls and conditional branches.

### Register pressure-aware region partitioning
Keep track of the register pressure: if it exceeds a threshold, add a new region boundary.

### First region repair
Insert region instructions at instructions that were flagged in the initial regions pass.

### Second region repair
Recompute the initial regions and place region instructions at region boundaries.

### Stack spill handling
If a stack-spilled store register is used elsewhere in the region, insert a new region boundary.

### CLWB insertion
Insert CLWB instructions after every store instruction.

### Third region repair
Because all other passes need to come before Branch Relaxation (to avoid linker out of range error), we need to do one final region repair for after Branch Relaxation.