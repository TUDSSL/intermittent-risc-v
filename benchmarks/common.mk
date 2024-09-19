BENCHMARKS := \
	picojpeg
		# adpcm \
		# aes \
		# coremark \
		# crc \
		# dijkstra \
		# picojpeg \
		# quicksort \
		# sha \
		# towers

BUILD_CONFIGURATIONS := \
		uninstrumented \
		replay-cache

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

DEFAULT_OPT_LEVEL := -O3
OPT_LEVELS := \
		-Os \
		-O1 \
		-O2 \
		-O3

# Highlight begin/end
HLB = "\\e[33m\\e[1m"
HLE = "\\e[0m"
