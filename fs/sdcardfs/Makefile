SDCARDFS_VERSION="0.1"

KERNELBUILD := /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS += -DSDCARDFS_VERSION=\"$(SDCARDFS_VERSION)\"

obj-m += sdcardfs.o
sdcardfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o packagelist.o derived_perm.o

default:
	make -C $(KERNELBUILD) M=$(shell pwd) modules
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.markers *.order *.symvers .tmp_versions

