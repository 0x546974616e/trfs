KDIR ?= /lib/modules/$(shell uname -r)/build

SOURCEDIR = $(CURDIR)/source
BUILDDIR = $(CURDIR)/build

.PHONY: all clean

# M/KBUILD_EXTMOD (https://docs.kernel.org/kbuild/kbuild.html)
# Set the directory to look for the kernel source when building external modules.

# src (https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)
# $(src) is the directory where the Makefile is located. Always use $(src) when
# referring to files located in the src tree.

all:
	make -C $(KDIR) M=$(BUILDDIR) src=$(SOURCEDIR) modules

clean:
	make -C $(KDIR) M=$(BUILDDIR) src=$(SOURCEDIR) clean
