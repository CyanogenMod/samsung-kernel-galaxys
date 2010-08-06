/* include/linux/android_power.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_ANDROID_POWER_H
#define _LINUX_ANDROID_POWER_H

#include <linux/list.h>
#include <linux/ktime.h>

typedef struct
{
	struct list_head    link;
	int                 flags;
	const char         *name;
	int                 expires;
#ifdef CONFIG_ANDROID_POWER_STAT
	struct {
		int             count;
		int             expire_count;
		ktime_t         total_time;
		ktime_t         max_time;
		ktime_t         last_time;
	} stat;
#endif
} android_suspend_lock_t;

#if 0 /* none of these flags are implemented */
#define ANDROID_SUSPEND_LOCK_FLAG_COUNTED (1U << 0)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_READABLE (1U << 1)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_SET (1U << 2)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_CLEAR (1U << 3)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_INC (1U << 4)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_DEC (1U << 5)
#define ANDROID_SUSPEND_LOCK_FLAG_USER_VISIBLE_MASK (0x1fU << 1)
#endif
#define ANDROID_SUSPEND_LOCK_AUTO_EXPIRE (1U << 6)
#define ANDROID_SUSPEND_LOCK_ACTIVE      (1U << 7)

enum {
	ANDROID_STOPPED_DRAWING,
	ANDROID_REQUEST_STOP_DRAWING,
	ANDROID_DRAWING_OK,
};

enum {
	ANDROID_EARLY_SUSPEND_LEVEL_BLANK_SCREEN = 50,
	ANDROID_EARLY_SUSPEND_LEVEL_CONSOLE_SWITCH = 100,
	ANDROID_EARLY_SUSPEND_LEVEL_DISABLE_FB = 150,
};
typedef struct android_early_suspend android_early_suspend_t;
struct android_early_suspend
{
	struct list_head link;
	int level;
	void (*suspend)(android_early_suspend_t *h);
	void (*resume)(android_early_suspend_t *h);
};

typedef enum {
	ANDROID_CHARGING_STATE_UNKNOWN,
	ANDROID_CHARGING_STATE_DISCHARGE,
	ANDROID_CHARGING_STATE_MAINTAIN, /* or trickle */
	ANDROID_CHARGING_STATE_SLOW,
	ANDROID_CHARGING_STATE_NORMAL,
	ANDROID_CHARGING_STATE_FAST,
	ANDROID_CHARGING_STATE_OVERHEAT
} android_charging_state_t;

/* android_suspend_lock_t *android_allocate_suspend_lock(const char *debug_name); */
/* void android_free_suspend_lock(android_suspend_lock_t *lock); */
int android_init_suspend_lock(android_suspend_lock_t *lock);
void android_uninit_suspend_lock(android_suspend_lock_t *lock);
void android_lock_idle(android_suspend_lock_t *lock);
void android_lock_idle_auto_expire(android_suspend_lock_t *lock, int timeout);
void android_lock_suspend(android_suspend_lock_t *lock);
void android_lock_suspend_auto_expire(android_suspend_lock_t *lock, int timeout);
void android_unlock_suspend(android_suspend_lock_t *lock);

int android_power_is_driver_suspended(void);
int android_power_is_low_power_idle_ok(void);

void android_register_early_suspend(android_early_suspend_t *handler);
void android_unregister_early_suspend(android_early_suspend_t *handler);

#endif

