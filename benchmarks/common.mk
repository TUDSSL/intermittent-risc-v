BENCHMARKS := \
		quicksort \
		coremark \
		crc \
		sha \
		dijkstra \
		aes \
		picojpeg

BUILD_CONFIGURATIONS := \
		uninstrumented

# Note that the checkpoint period in the benchmarks is on_duration/2
# 250000   =>   5ms @ 50MHz
# 500000   =>  10ms @ 50MHz
# 2500000  =>  50ms @ 50MHz
# 5000000  => 100ms @ 50MHz
# 25000000 => 500ms @ 50MHz
ON_DURATIONS := \
    	250000 \
    	500000 \
    	2500000 \
    	5000000 \
    	25000000

# Highlight begin/end
HLB = "\\e[33m\\e[1m"
HLE = "\\e[0m"
