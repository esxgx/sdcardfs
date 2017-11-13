/* vim:set ts=8 sw=8 tw=0 noet ft=c:
 *
 * fs/sdcardfs/create.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */

/* this file is as part of inode.c, do NOT use it directly */

/* context for sdcardfs_do_create_xxxx */
typedef struct sdcardfs_do_create_struct {
	struct dentry *parent, *real_dentry;
	struct dentry *real_dir_dentry;
	const struct cred *saved_cred;
	struct fs_struct *saved_fs;
} _sdcardfs_do_create_struct;

static int __sdcardfs_do_create_begin(
	_sdcardfs_do_create_struct *this,
	struct inode *dir,
	struct dentry *dentry)
{
	int err;
	struct sdcardfs_sb_info *sbi;

	if (SDCARDFS_D(dentry) != NULL) {
		warnln("%s, negative dentry(%s) should not have tree entry",
			__func__, dentry->d_name.name);
		return -ESTALE;
	}

	/* some forbidden filenames should be checked before creating */
	if (permission_denied_to_create(dir, dentry->d_name.name)) {
		errln("permission denied to create %s", dentry->d_name.name);
		return -EACCES;
	}

	this->parent = dget_parent(dentry);
	BUG_ON(d_inode(this->parent) != dir);

	this->real_dir_dentry = sdcardfs_get_lower_dentry(this->parent);
	BUG_ON(this->real_dir_dentry == NULL);

	inode_lock_nested(d_inode(this->real_dir_dentry), I_MUTEX_PARENT);

	/* save current_cred and override it */
	sbi = SDCARDFS_SB(dir->i_sb);
	OVERRIDE_CRED(sbi, this->saved_cred);
	if (IS_ERR(this->saved_cred)) {
		err = PTR_ERR(this->saved_cred);
		goto unlock_err;
	}

#ifdef SDCARDFS_CASE_INSENSITIVE
	if (sbi->ci->may_create != NULL) {
		struct path path = {.dentry = this->real_dir_dentry,
			.mnt = sbi->lower_mnt};

		err = sbi->ci->may_create(&path, &dentry->d_name);
		if (err)
			goto revert_cred_err;
	}
#endif

	this->real_dentry = lookup_one_len(dentry->d_name.name,
		this->real_dir_dentry, dentry->d_name.len);

	if (IS_ERR(this->real_dentry)) {
		err = PTR_ERR(this->real_dentry);
		goto revert_cred_err;
	}

	if (d_is_positive(this->real_dentry)) {
		err = -ESTALE;
		goto dput_err;
	}

	BUG_ON(sbi->override_fs == NULL);
	this->saved_fs = override_current_fs(sbi->override_fs);
	return 0;

dput_err:
	dput(this->real_dentry);
revert_cred_err:
	REVERT_CRED(this->saved_cred);
unlock_err:
	inode_unlock(d_inode(this->real_dir_dentry));
	dput(this->real_dir_dentry);
	dput(this->parent);
	return err;
}

static int __sdcardfs_do_create_end(
	_sdcardfs_do_create_struct *this,
	const char *__caller_FUNCTION__,
	struct inode *dir,
	struct dentry *dentry,
	int err)
{
	revert_current_fs(this->saved_fs);
	REVERT_CRED(this->saved_cred);

	if (err) {
		dput(this->real_dentry);
		goto out;
	}

	err = PTR_ERR(sdcardfs_interpose(this->parent,
		dentry, this->real_dentry));
	if (err)
		errln("%s, unexpected error when interposing: %d",
			__caller_FUNCTION__, err);

out:
	fsstack_copy_inode_size(dir, d_inode(this->real_dir_dentry));

	inode_unlock(d_inode(this->real_dir_dentry));
	dput(this->real_dir_dentry);
	dput(this->parent);
	return err;
}
