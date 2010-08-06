/*
 * Copyright (C) 2005-2009 Junjiro R. Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * super_block operations
 */

#ifndef __AUFS_SUPER_H__
#define __AUFS_SUPER_H__

#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/aufs_type.h>
#include "rwsem.h"
#include "spl.h"
#include "wkq.h"

typedef ssize_t (*au_readf_t)(struct file *, char __user *, size_t, loff_t *);
typedef ssize_t (*au_writef_t)(struct file *, const char __user *, size_t,
			       loff_t *);

/* policies to select one among multiple writable branches */
struct au_wbr_copyup_operations {
	int (*copyup)(struct dentry *dentry);
};

struct au_wbr_create_operations {
	int (*create)(struct dentry *dentry, int isdir);
	int (*init)(struct super_block *sb);
	int (*fin)(struct super_block *sb);
};

struct au_wbr_mfs {
	struct mutex	mfs_lock; /* protect this structure */
	unsigned long	mfs_jiffy;
	unsigned long	mfs_expire;
	aufs_bindex_t	mfs_bindex;

	unsigned long long	mfsrr_bytes;
	unsigned long long	mfsrr_watermark;
};

struct au_branch;
struct au_sbinfo {
	/* nowait tasks in the system-wide workqueue */
	struct au_nowait_tasks	si_nowait;

	struct au_rwsem		si_rwsem;

	/* branch management */
	unsigned int		si_generation;

	/* see above flags */
	unsigned char		au_si_status;

	aufs_bindex_t		si_bend;
	aufs_bindex_t		si_last_br_id;
	struct au_branch	**si_branch;

	/* policy to select a writable branch */
	unsigned char		si_wbr_copyup;
	unsigned char		si_wbr_create;
	struct au_wbr_copyup_operations *si_wbr_copyup_ops;
	struct au_wbr_create_operations *si_wbr_create_ops;

	/* round robin */
	atomic_t		si_wbr_rr_next;

	/* most free space */
	struct au_wbr_mfs	si_wbr_mfs;

	/* mount flags */
	/* include/asm-ia64/siginfo.h defines a macro named si_flags */
	unsigned int		si_mntflags;

	/* external inode number (bitmap and translation table) */
	au_readf_t		si_xread;
	au_writef_t		si_xwrite;
	struct file		*si_xib;
	struct mutex		si_xib_mtx; /* protect xib members */
	unsigned long		*si_xib_buf;
	unsigned long		si_xib_last_pindex;
	int			si_xib_next_bit;
	aufs_bindex_t		si_xino_brid;
	/* reserved for future use */
	/* unsigned long long	si_xib_limit; */	/* Max xib file size */

#ifdef CONFIG_AUFS_EXPORT
	/* i_generation */
	struct file		*si_xigen;
	atomic_t		si_xigen_next;
#endif

	/* vdir parameters */
	unsigned long		si_rdcache;	/* max cache time in HZ */
	unsigned int		si_rdblk;	/* deblk size */
	unsigned int		si_rdhash;	/* hash size */

	/*
	 * If the number of whiteouts are larger than si_dirwh, leave all of
	 * them after au_whtmp_ren to reduce the cost of rmdir(2).
	 * future fsck.aufs or kernel thread will remove them later.
	 * Otherwise, remove all whiteouts and the dir in rmdir(2).
	 */
	unsigned int		si_dirwh;

	/*
	 * rename(2) a directory with all children.
	 */
	/* reserved for future use */
	/* int			si_rendir; */

	/* pseudo_link list */
	struct au_splhead	si_plink;
	wait_queue_head_t	si_plink_wq;

	/*
	 * sysfs and lifetime management.
	 * this is not a small structure and it may be a waste of memory in case
	 * of sysfs is disabled, particulary when many aufs-es are mounted.
	 * but using sysfs is majority.
	 */
	struct kobject		si_kobj;
#ifdef CONFIG_DEBUG_FS
	struct dentry		 *si_dbgaufs, *si_dbgaufs_xib;
#ifdef CONFIG_AUFS_EXPORT
	struct dentry		 *si_dbgaufs_xigen;
#endif
#endif

	/* dirty, necessary for unmounting, sysfs and sysrq */
	struct super_block	*si_sb;
};

/* sbinfo status flags */
/*
 * set true when refresh_dirs() failed at remount time.
 * then try refreshing dirs at access time again.
 * if it is false, refreshing dirs at access time is unnecesary
 */
#define AuSi_FAILED_REFRESH_DIRS	1
#define AuSi_MAINTAIN_PLINK		(1 << 1)	/* ioctl */
static inline unsigned char au_do_ftest_si(struct au_sbinfo *sbi,
					   unsigned int flag)
{
	AuRwMustAnyLock(&sbi->si_rwsem);
	return sbi->au_si_status & flag;
}
#define au_ftest_si(sbinfo, name)	au_do_ftest_si(sbinfo, AuSi_##name)
#define au_fset_si(sbinfo, name) do { \
	AuRwMustWriteLock(&(sbinfo)->si_rwsem); \
	(sbinfo)->au_si_status |= AuSi_##name; \
} while (0)
#define au_fclr_si(sbinfo, name) do { \
	AuRwMustWriteLock(&(sbinfo)->si_rwsem); \
	(sbinfo)->au_si_status &= ~AuSi_##name; \
} while (0)

/* ---------------------------------------------------------------------- */

/* policy to select one among writable branches */
#define AuWbrCopyup(sbinfo, args...) \
	((sbinfo)->si_wbr_copyup_ops->copyup(args))
#define AuWbrCreate(sbinfo, args...) \
	((sbinfo)->si_wbr_create_ops->create(args))

/* flags for si_read_lock()/aufs_read_lock()/di_read_lock() */
#define AuLock_DW		1		/* write-lock dentry */
#define AuLock_IR		(1 << 1)	/* read-lock inode */
#define AuLock_IW		(1 << 2)	/* write-lock inode */
#define AuLock_FLUSH		(1 << 3)	/* wait for 'nowait' tasks */
#define AuLock_DIR		(1 << 4)	/* target is a dir */
#define au_ftest_lock(flags, name)	((flags) & AuLock_##name)
#define au_fset_lock(flags, name)	{ (flags) |= AuLock_##name; }
#define au_fclr_lock(flags, name)	{ (flags) &= ~AuLock_##name; }

/* ---------------------------------------------------------------------- */

/* super.c */
extern struct file_system_type aufs_fs_type;
struct inode *au_iget_locked(struct super_block *sb, ino_t ino);

/* sbinfo.c */
void au_si_free(struct kobject *kobj);
int au_si_alloc(struct super_block *sb);
int au_sbr_realloc(struct au_sbinfo *sbinfo, int nbr);

unsigned int au_sigen_inc(struct super_block *sb);
aufs_bindex_t au_new_br_id(struct super_block *sb);

void aufs_read_lock(struct dentry *dentry, int flags);
void aufs_read_unlock(struct dentry *dentry, int flags);
void aufs_write_lock(struct dentry *dentry);
void aufs_write_unlock(struct dentry *dentry);
void aufs_read_and_write_lock2(struct dentry *d1, struct dentry *d2, int isdir);
void aufs_read_and_write_unlock2(struct dentry *d1, struct dentry *d2);

/* wbr_policy.c */
extern struct au_wbr_copyup_operations au_wbr_copyup_ops[];
extern struct au_wbr_create_operations au_wbr_create_ops[];
int au_cpdown_dirs(struct dentry *dentry, aufs_bindex_t bdst);

/* ---------------------------------------------------------------------- */

static inline struct au_sbinfo *au_sbi(struct super_block *sb)
{
	return sb->s_fs_info;
}

/* ---------------------------------------------------------------------- */

#ifdef CONFIG_AUFS_EXPORT
void au_export_init(struct super_block *sb);

static inline int au_test_nfsd(struct task_struct *tsk)
{
	return !tsk->mm && !strcmp(tsk->comm, "nfsd");
}

int au_xigen_inc(struct inode *inode);
int au_xigen_new(struct inode *inode);
int au_xigen_set(struct super_block *sb, struct file *base);
void au_xigen_clr(struct super_block *sb);

static inline int au_busy_or_stale(void)
{
	if (!au_test_nfsd(current))
		return -EBUSY;
	return -ESTALE;
}
#else
static inline void au_export_init(struct super_block *sb)
{
	/* nothing */
}

static inline int au_test_nfsd(struct task_struct *tsk)
{
	return 0;
}

static inline int au_xigen_inc(struct inode *inode)
{
	return 0;
}

static inline int au_xigen_new(struct inode *inode)
{
	return 0;
}

static inline int au_xigen_set(struct super_block *sb, struct file *base)
{
	return 0;
}

static inline void au_xigen_clr(struct super_block *sb)
{
	/* empty */
}

static inline int au_busy_or_stale(void)
{
	return -EBUSY;
}
#endif /* CONFIG_AUFS_EXPORT */

/* ---------------------------------------------------------------------- */

static inline void dbgaufs_si_null(struct au_sbinfo *sbinfo)
{
	/*
	 * This function is a dynamic '__init' fucntion actually,
	 * so the tiny check for si_rwsem is unnecessary.
	 */
	/* AuRwMustWriteLock(&sbinfo->si_rwsem); */
#ifdef CONFIG_DEBUG_FS
	sbinfo->si_dbgaufs = NULL;
	sbinfo->si_dbgaufs_xib = NULL;
#ifdef CONFIG_AUFS_EXPORT
	sbinfo->si_dbgaufs_xigen = NULL;
#endif
#endif
}

/* ---------------------------------------------------------------------- */

/* lock superblock. mainly for entry point functions */
/*
 * si_noflush_read_lock, si_noflush_write_lock,
 * si_read_unlock, si_write_unlock, si_downgrade_lock
 */
AuSimpleLockRwsemFuncs(si_noflush, struct super_block *sb,
		       &au_sbi(sb)->si_rwsem);
AuSimpleUnlockRwsemFuncs(si, struct super_block *sb, &au_sbi(sb)->si_rwsem);

#define SiMustNoWaiters(sb)	AuRwMustNoWaiters(&au_sbi(sb)->si_rwsem)
#define SiMustAnyLock(sb)	AuRwMustAnyLock(&au_sbi(sb)->si_rwsem)
#define SiMustWriteLock(sb)	AuRwMustWriteLock(&au_sbi(sb)->si_rwsem)

static inline void si_read_lock(struct super_block *sb, int flags)
{
	if (au_ftest_lock(flags, FLUSH))
		au_nwt_flush(&au_sbi(sb)->si_nowait);
	si_noflush_read_lock(sb);
}

static inline void si_write_lock(struct super_block *sb)
{
	au_nwt_flush(&au_sbi(sb)->si_nowait);
	si_noflush_write_lock(sb);
}

static inline int si_read_trylock(struct super_block *sb, int flags)
{
	if (au_ftest_lock(flags, FLUSH))
		au_nwt_flush(&au_sbi(sb)->si_nowait);
	return si_noflush_read_trylock(sb);
}

static inline int si_write_trylock(struct super_block *sb, int flags)
{
	if (au_ftest_lock(flags, FLUSH))
		au_nwt_flush(&au_sbi(sb)->si_nowait);
	return si_noflush_write_trylock(sb);
}

/* ---------------------------------------------------------------------- */

static inline aufs_bindex_t au_sbend(struct super_block *sb)
{
	SiMustAnyLock(sb);
	return au_sbi(sb)->si_bend;
}

static inline unsigned int au_mntflags(struct super_block *sb)
{
	SiMustAnyLock(sb);
	return au_sbi(sb)->si_mntflags;
}

static inline unsigned int au_sigen(struct super_block *sb)
{
	SiMustAnyLock(sb);
	return au_sbi(sb)->si_generation;
}

static inline struct au_branch *au_sbr(struct super_block *sb,
				       aufs_bindex_t bindex)
{
	SiMustAnyLock(sb);
	return au_sbi(sb)->si_branch[0 + bindex];
}

static inline void au_xino_brid_set(struct super_block *sb, aufs_bindex_t brid)
{
	SiMustWriteLock(sb);
	au_sbi(sb)->si_xino_brid = brid;
}

static inline aufs_bindex_t au_xino_brid(struct super_block *sb)
{
	SiMustAnyLock(sb);
	return au_sbi(sb)->si_xino_brid;
}

#endif /* __KERNEL__ */
#endif /* __AUFS_SUPER_H__ */
