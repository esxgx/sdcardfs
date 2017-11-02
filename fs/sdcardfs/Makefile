SDCARDFS_VERSION="1.0"

ccflags-y += -D__SDCARDFS__=sdcardfs -DSDCARDFS_VERSION=\"$(SDCARDFS_VERSION)\"
ccflags-y += -DSDCARDFS_UNDERLAY_MULTI_ALIASES -g -Wall
ccflags-y += -I$(src)   # needed for trace events

ccflags-$(CONFIG_SDCARD_FS_CASE_INSENSITIVE) += -DSDCARDFS_CASE_INSENSITIVE
ccflags-$(CONFIG_SDCARD_FS_RESERVED_SPACE) += -DSDCARDFS_SUPPORT_RESERVED_SPACE
ccflags-$(CONFIG_SDCARD_FS_SYSFS) += -DSDCARDFS_SYSFS_FEATURE
ccflags-$(CONFIG_SDCARD_FS_PLUGIN_PRIVACY_SPACE) += -DSDCARDFS_PLUGIN_PRIVACY_SPACE

obj-$(CONFIG_SDCARD_FS) += sdcardfs.o
sdcardfs-objs := dentry.o file.o inode.o configfs.o dir_ci.o main.o super.o namei.o \
	mmap.o packagelist.o derived_perm.o tree.o xattr.o
sdcardfs-$(CONFIG_SDCARD_FS_SYSFS) += sysfs.o

