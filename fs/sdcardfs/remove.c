/* vim:set ts=8 sw=8 tw=0 noet ft=c:
 *
 * fs/sdcardfs/remove.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */

/* this file is as part of inode.c, do NOT use it directly */

/* context for sdcardfs_do_remove_xxxx */
typedef struct sdcardfs_do_remove_struct {
	struct dentry *real_dentry;
	struct dentry *real_dir_dentry;
	const struct cred *saved_cred;
} _sdcardfs_do_remove_struct;

static int __sdcardfs_do_remove_begin(
	_sdcardfs_do_remove_struct *this,
	struct inode *dir,
	struct dentry *dentry)
{
	int err;
	struct dentry *real;

	/* some forbidden filenames should be checked before removing */
	if (permission_denied_to_remove(dir, dentry->d_name.name)) {
		errln("permission denied to remove %s", dentry->d_name.name);
		return -EACCES;
	}

	this->real_dentry = sdcardfs_get_real_dentry(dentry);
	BUG_ON(this->real_dentry == NULL);

retry:
	this->real_dir_dentry = dget_parent(this->real_dentry);

	/* TODO: disconnected dentry is not supported yet.*/
	BUG_ON(this->real_dir_dentry == NULL);

	/*
	 * note that real_dir_dentry ?(!=) lower_dentry(dget_parent(dentry)).
	 * it's unsafe to check by use IS_ROOT since inode_lock has not taken
	 */
	if (unlikely(this->real_dentry ==
		this->real_dir_dentry)) {
		err = -EBUSY;
		goto dput_err;
	}
	inode_lock_nested(d_inode(this->real_dir_dentry), I_MUTEX_PARENT);

	if (unlikely(this->real_dir_dentry !=
		this->real_dentry->d_parent)) {
		inode_unlock(d_inode(this->real_dir_dentry));
		dput(this->real_dir_dentry);
		goto retry;
	}

	/* save current_cred and override it */
	OVERRIDE_CRED(SDCARDFS_SB(dir->i_sb), this->saved_cred);
	if (IS_ERR(this->saved_cred)) {
		err = PTR_ERR(this->saved_cred);
		goto unlock_err;
	}

	/* real_dentry must be hashed and in the real_dir */
	real = lookup_one_len(this->real_dentry->d_name.name,
		this->real_dir_dentry, this->real_dentry->d_name.len);
	if (IS_ERR(real)) {
		/* maybe some err or real_dir_dentry DEADDIR */
		err = PTR_ERR(real);
		goto revert_cred_err;
	}

	dput(real);

	/*
	 * although we find a dentry with the same name in lower fs,
	 * it's not the old real dentry stored in tree_entry
	 */
	if (this->real_dentry != real) {
		err = -ESTALE;

		/* since we dont support hashed but negative dentry */
		d_invalidate(dentry);
		goto revert_cred_err;
	}
	return 0;

revert_cred_err:
	REVERT_CRED(this->saved_cred);
unlock_err:
	inode_unlock(d_inode(this->real_dir_dentry));
dput_err:
	dput(this->real_dir_dentry);
	dput(this->real_dentry);
	return err;
}

static void __sdcardfs_do_remove_end(
	_sdcardfs_do_remove_struct *this,
	struct inode *dir)
{
	REVERT_CRED(this->saved_cred);
	fsstack_copy_inode_size(dir, d_inode(this->real_dir_dentry));

	inode_unlock(d_inode(this->real_dir_dentry));
	dput(this->real_dir_dentry);
	dput(this->real_dentry);
}

