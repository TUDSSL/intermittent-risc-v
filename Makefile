# Use:
# $ make nodownload=1 target
# To skip downloading (in case everything is pre-downloaded)

# Phony targets
PNONIES=""

all: toolchains llvm icemu
	echo "Done building all"

toolchains:
	cd toolchains && ./setup.sh

# All downloads
download: download-llvm download-icemu
	echo "Done downloading files"

# LLVM
download-llvm: llvm/.download_llvm
	@echo "LLVM up-to-date"
PHONIES+=download-llvm

llvm/.download_llvm:
ifdef nodownload
	@echo "Skipping download"
else
	echo "Downloading LLVM components"
	cd llvm && ./download.sh
endif

llvm: download-llvm
	cd llvm && ./build.sh
	@echo "LLVM build completed"
PHONIES+=llvm

clean-llvm-downloads:
	@echo "Removing LLVM downloads marker (does not remove the files), enable re-download"
	rm -f llvm/.download_llvm
PHONIES+=clean-llvm-downloads

clean-llvm:
	@echo "Cleaning LLVM directory"
	rm -rf llvm/llvm-*/build
PHONIES+=clean-llvm


# ICEmu
download-icemu:
ifdef nodownload
	@echo "Skipping download"
else
	git submodule update --init --recursive icemu/icemu
	@echo "ICEmu submodule up-to-date"
endif
PHONIES+=donwnload-icemu

icemu: download-icemu
	cd icemu && ./build.sh
	@echo "ICEmu build completed"
PHONIES+=icemu


.PHONY: ${PHONIES} all
