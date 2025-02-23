WORKDIR := /workspaces/mikanos-devcontainer
SRCDIR := src
EDK2DIR := ~/edk2
DEVENV := ~/osbook/devenv


LoaderPkgDir := MikanLoaderPkg
LoaderPkgDsc := MikanLoaderPkg.dsc
LoaderPkgExists := $(shell ls $(EDK2DIR) |grep ${LoaderPkgDir})

OUTPUT := $(EDK2DIR)/Build/MikanLoaderX64/DEBUG_CLANG38/X64/MikanLoaderPkg/Loader/DEBUG

KernelSrcDir := kernel

.PHONY:all
all: ${OUTPUT}/Loader.efi ${SRCDIR}/${KernelSrcDir}/kernel.elf

${OUTPUT}/Loader.efi: ${SRCDIR}/${LoaderPkgDir}/Main.c
	rm -rf $(EDK2DIR)/$(LoaderPkgDir)
	cp -pr $(SRCDIR)/$(LoaderPkgDir) $(EDK2DIR)/$(LoaderPkgDir)
	WORKDIR=$(WORKDIR) script/build_loader.sh

${SRCDIR}/${KernelSrcDir}/kernel.elf: 
	make -C $(SRCDIR)/kernel

.PHONY:run
run: all
	${DEVENV}/run_qemu.sh $(OUTPUT)/Loader.efi $(SRCDIR)/kernel/kernel.elf

clean:
	rm -rf $(EDK2DIR)/Build/MikanLoaderX64
	rm -rf src/MikanLoaderPkg/Loader.efi
	make -C src/${KernelSrcDir} clean
	rm -rf disk.img

.PHONY:edk2tools
edk2tools:
	make -C $(EDK2DIR)/BaseTools/Source/C
