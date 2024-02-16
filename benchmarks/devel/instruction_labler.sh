sed -i 's/li	t6, 0/li	t6, 0                   ; START_REGION/g' $1
sed -i 's/li	t6, 1/li	t6, 1                   ; CLWB/g' $1
sed -i 's/li	t6, 2/li	t6, 2                   ; FENCE/g' $1
sed -i 's/li	t6, 3/li	t6, 3                   ; POWER_FAILURE/g' $1