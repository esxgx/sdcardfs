/* vim:set ts=8 sw=8 tw=0 noet ft=c:
 *
 * fs/sdcardfs/misc.h
 * Reusable code-snippets / Utilities for the sdcardfs implementation
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#if defined(__SDCARDFS_MISC__SHOW_OPTIONS)
#undef __SDCARDFS_MISC__SHOW_OPTIONS
	if (opts->fs_low_uid)
		xx(",fsuid=%u", opts->fs_low_uid);
	if (opts->fs_low_gid)
		xx(",fsgid=%u", opts->fs_low_gid);
	if (opts->gid)
		xx(",gid=%u", opts->gid);
	if (opts->multiuser)
		xx(",multiuser");
	if (opts->mask)
		xx(",mask=%u", opts->mask);
	if (opts->fs_user_id)
		xx(",userid=%u", opts->fs_user_id);
	if (opts->reserved_mb)
		xx(",reserved_mb=%u", opts->reserved_mb);
	if (opts->quiet)
		xx(",quiet");
#elif defined(__SDCARDFS_MISC__COMPAT_DEFS)
#undef __SDCARDFS_MISC__COMPAT_DEFS

#ifndef d_inode
#define d_inode(x)  ((x)->d_inode)
#endif

#ifndef inode_lock_nested
#define inode_lock_nested(x, y) mutex_lock_nested(&(x)->i_mutex, (y))
#endif

#ifndef inode_lock
#define inode_lock(x) mutex_lock(&(x)->i_mutex)
#endif

#ifndef inode_unlock
#define inode_unlock(x) mutex_unlock(&(x)->i_mutex)
#endif

#ifndef lockless_dereference
#define lockless_dereference(p) ({ \
        typeof(p) _________p1 = ACCESS_ONCE(p); \
        typeof(*(p)) *___typecheck_p __maybe_unused; \
        smp_read_barrier_depends(); /* Dependency order vs. p above. */ \
        (_________p1); \
})
#endif

#ifndef d_really_is_negative
#define d_really_is_negative(dentry) ((dentry)->d_inode == NULL)
#endif

#else
#error precompiled macro is not defined
#endif

