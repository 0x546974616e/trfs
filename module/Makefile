# https://www.kernel.org/doc/html/latest/kbuild/
# https://docs.kernel.org/bpf/btf.html

TRFS_MODULE = trfs
TRFS_SOURCES_DIR = $(CURDIR)/sources/
TRFS_BUILD_DIR = $(CURDIR)/build/

KBUILD_DIR ?= /lib/modules/$(shell uname -r)/build/

# For Kbuild files
export TRFS_MODULE
export TRFS_SOURCES_DIR
export KBUILD_VERBOSE := 0

# M/KBUILD_EXTMOD (https://docs.kernel.org/kbuild/kbuild.html)
# Set the directory to look for the kernel source when building external modules.

# src (https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)
# $(src) is the directory where the Makefile is located. Always use $(src) when
# referring to files located in the src tree.

.PHONY: all clean dmesg modinfo install uninstall

all:
	make -C $(KBUILD_DIR) M=$(TRFS_BUILD_DIR) src=$(TRFS_SOURCES_DIR) modules

clean:
	make -C $(KBUILD_DIR) M=$(TRFS_BUILD_DIR) src=$(TRFS_SOURCES_DIR) clean

dmesg:
	@dmesg --color=always | grep --color=no $(TRFS_MODULE)

modinfo:
	modinfo $(TRFS_BUILD_DIR)/$(TRFS_MODULE).ko

install:
	insmod $(TRFS_BUILD_DIR)/$(TRFS_MODULE).ko

uninstall:
	rmmod $(TRFS_MODULE)
