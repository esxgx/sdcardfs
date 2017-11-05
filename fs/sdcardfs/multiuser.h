/*
 * fs/sdcardfs/multiuser.h
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#ifndef __SDCARDFS_MULTIUSER_H
#define __SDCARDFS_MULTIUSER_H

#define MULTIUSER_APP_PER_USER_RANGE 100000

typedef uid_t userid_t;
typedef uid_t appid_t;

static inline userid_t multiuser_get_user_id(uid_t uid)
{
	return uid / MULTIUSER_APP_PER_USER_RANGE;
}

static inline appid_t multiuser_get_app_id(uid_t uid)
{
	return uid % MULTIUSER_APP_PER_USER_RANGE;
}

static inline uid_t multiuser_get_uid(userid_t user_id,
	appid_t app_id)
{
	return user_id * MULTIUSER_APP_PER_USER_RANGE
		+ (app_id % MULTIUSER_APP_PER_USER_RANGE);
}

#endif

