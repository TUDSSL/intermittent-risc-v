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

# Power failure tests. 0 means continious, but is interesting as it includes the CHECKPOINT_PERIOD
ON_DURATIONS := \
		0			\
		100000 		\
		1000000 	\
		5000000 	\
		10000000 	\

# Only one possible.
# The period for the periodic checkpoint, only used when running power-failure tests
# using the ON_DURATIONS
CHECKPOINT_PERIOD := 90000 

# Highlight begin/end
HLB = "\\e[33m\\e[1m"
HLE = "\\e[0m"
