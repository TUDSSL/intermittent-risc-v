all: icemu llvm noelle
	echo "Done building all"

icemu:
	echo "Building icemu"
	cd icemu && ./build.sh

llvm:
	echo "Building llvm"
	cd llvm && ./download.sh && ./build.sh

noelle: llvm
	echo "Building noelle"
	cd noelle && ./build.sh

.PHONY: icemu llvm noelle

