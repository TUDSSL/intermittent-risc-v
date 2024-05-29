sed -i 's/li	t6, 0/li	t6, 0                   ; START_REGION/g' $1
sed -i 's/li	t6, 1/li	t6, 1                   ; START_REGION_RETURN/g' $1
sed -i 's/li	t6, 2/li	t6, 2                   ; START_REGION_EXTENSION/g' $1
sed -i 's/li	t6, 3/li	t6, 3                   ; START_REGION_BRANCH/g' $1
sed -i 's/li	t6, 4/li	t6, 4                   ; START_REGION_BRANCH_DEST/g' $1
sed -i 's/li	t6, 5/li	t6, 5                   ; START_REGION_STACK_SPILL/g' $1
sed -i 's/li	t6, 7/li	t6, 7                   ; FENCE/g' $1
sed -i 's/li	t6, 8/li	t6, 8                   ; CLWB/g' $1