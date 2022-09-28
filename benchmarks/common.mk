BENCHMARKS := \
		quicksort \
		coremark \
		crc \
		sha \
		dijkstra \
		aes \
		picojpeg

BUILD_CONFIGURATIONS := \
		uninstrumented \
		cachehints


# Highlight begin/end
HLB = "\\e[33m\\e[1m"
HLE = "\\e[0m"
