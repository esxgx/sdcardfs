/* vim:set ts=8 sw=8 tw=0 noet ft=c:
 *
 * fs/sdcardfs/dir_ci.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include <linux/version.h>

struct __generic_ci_readdir_private {
	struct dir_context ctx;
	const struct qstr *to_find;
	union {
		char *name;
		bool found;
	} u;
};

static int __generic_lookup_ci_match(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	void *_ctx, const char *name, int namelen,
#else
	struct dir_context *_ctx, const char *name, int namelen,
#endif
	loff_t offset, u64 ino, unsigned int d_type
) {
	struct __generic_ci_readdir_private *buf = container_of(_ctx,
		struct __generic_ci_readdir_private, ctx);

	if (unlikely(namelen == buf->to_find->len &&
		!strncasecmp(name, buf->to_find->name, namelen))) {
		buf->u.name = __getname();
		if (buf->u.name == NULL)
			buf->u.name = ERR_PTR(-ENOMEM);
		else {
			memcpy(buf->u.name, name, namelen);
			buf->u.name[namelen] = '\0';
		}
		return -1;
	}
	return 0;
}

static int __generic_may_create_ci_match(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	void *_ctx, const char *name, int namelen,
#else
	struct dir_context *_ctx, const char *name, int namelen,
#endif
	loff_t offset, u64 ino, unsigned int d_type)
{
	struct __generic_ci_readdir_private *buf = container_of(_ctx,
		struct __generic_ci_readdir_private, ctx);

	if (unlikely(namelen == buf->to_find->len &&
		!strncasecmp(name, buf->to_find->name, namelen))) {
		buf->u.found = true;
		return -1;		/* found */
	}
	return 0;
}

static inline int __iterate_dir_locked(struct file *file,
	struct dir_context *ctx)
{
	struct inode *inode;
	int res;
	if (file->f_op->iterate == NULL)
		return -ENOTDIR;

	inode = file_inode(file);
	if (IS_DEADDIR(inode))
		return -ENOENT;

	ctx->pos = file->f_pos;
	res = file->f_op->iterate(file, ctx);
	file->f_pos = ctx->pos;
	return res;
}

/* remember that the currect cred has been overrided */
static char *
__generic_lookup_ci_begin(struct path *dir,
	struct qstr *name, bool locked)
{
	int err;
	struct file *file;
	struct __generic_ci_readdir_private buffer = {
		.ctx.actor = __generic_lookup_ci_match,
		.to_find = name,
		.u.name = NULL,
	};

	/* any risk dentry_open within inode_lock(dir)? */
	file = dentry_open(dir, O_RDONLY | O_DIRECTORY
		| O_NOATIME, current_cred());

	if (IS_ERR(file)) {
		errln("%s: unexpected error when dentry_open, err=%ld",
			__FUNCTION__, PTR_ERR(file));
		return ERR_CAST(file);
	}

	err = locked ? __iterate_dir_locked(file, &buffer.ctx) :
		iterate_dir(file, &buffer.ctx);
	fput(file);
	if (err)
		return ERR_PTR(err);
	return buffer.u.name;
}

static struct dentry *sdcardfs_generic_lookup_ci(
	struct path *dir,
	struct qstr *orig, bool locked)
{
	struct dentry *dentry;
	char *name;

	name = __generic_lookup_ci_begin(dir, orig, locked);
	if (IS_ERR(name))
		return (struct dentry *)name;
	else if (name == NULL)
		return ERR_PTR(-ENOENT);

	if (!locked)
		inode_lock_nested(d_inode(dir->dentry), I_MUTEX_PARENT);
	dentry = lookup_one_len(name, dir->dentry, orig->len);
	if (!locked)
		inode_unlock(d_inode(dir->dentry));
	__putname(name);
	/* remember that lookup_one_len never returns NULL */
	return dentry;
}

static int
sdcardfs_generic_may_create_ci(struct path *dir, struct qstr *name)
{
	int err;
	struct file *file;
	struct __generic_ci_readdir_private buffer = {
		.ctx.actor = __generic_may_create_ci_match,
		.to_find = name,
		.u.found = false
	};

	/* any risk dentry_open within inode_lock(dir)? */
	file = dentry_open(dir, O_RDONLY | O_DIRECTORY
		| O_NOATIME, current_cred());
	if (IS_ERR(file)) {
		err = PTR_ERR(file);
		errln("%s: unexpected error when dentry_open, err=%d",
			__FUNCTION__, err);
		return err;
	}
	err = __iterate_dir_locked(file, &buffer.ctx);
	fput(file);
	if (err)
		return err;
	return buffer.u.found ? -EEXIST : 0;
}

struct sdcardfs_dir_ci_ops sdcardfs_generic_dir_ci_op = {
	.lookup = sdcardfs_generic_lookup_ci,
	.may_create = sdcardfs_generic_may_create_ci
}, sdcardfs_stub_dir_ci_op = {
	.lookup = NULL,
	.may_create = NULL
};

#define EXFAT_SUPER_MAGIC       (0x2011BAB0L)
#define NTFS_SUPER_MAGIC        (0x5346544EL)

struct sdcardfs_dir_ci_ops *sdcardfs_lowerfs_ci_ops(struct super_block *sb)
{
	if (sb->s_magic == MSDOS_SUPER_MAGIC
		|| sb->s_magic == EXFAT_SUPER_MAGIC
		|| sb->s_magic == NTFS_SUPER_MAGIC)
		return &sdcardfs_stub_dir_ci_op;
	return &sdcardfs_generic_dir_ci_op;
}

