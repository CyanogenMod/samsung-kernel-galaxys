/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 * Copyright (C) 2003-2010 Samsung Electronics                               *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License version 2 as         *
 * published by the Free Software Foundation.                                *
 *                                                                           *
 *---------------------------------------------------------------------------*
*/
/**
 * @version	LinuStoreIII_1.2.0_b032-FSR_1.2.1p1_b129_RTM
 * @file        drivers/tfsr/tfsr_block.c
 * @brief       This file is BML common part which supports the raw interface
 *              It provides block device operations and utilities
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/slab.h>

#include <asm/errno.h>
#include <asm/uaccess.h>

#include "tfsr_base.h"

/**
 * BML block open operation
 * @param inode         block device inode
 * @param file          block device file
 * @return              0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int bml_block_open(struct block_device *bdev, fmode_t mode)
{
	u32 volume, minor;
	int ret;
	minor = MINOR(bdev->bd_dev);
#else
static int bml_block_open(struct inode *inode, struct file *file)
{
	u32 volume, minor;
	int ret;
	
	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	
	DEBUG(DL3,"TINY[I]: volume(%d), minor(%d)\n", volume, minor);

	if (volume >= FSR_MAX_VOLUMES)
	{
		ERRPRINTK("TINY: Out of volume range\n");
		return -ENODEV;
	}

	ret = FSR_BML_Open(volume, FSR_BML_FLAG_NONE);
	if (ret != FSR_BML_SUCCESS) 
	{
		ERRPRINTK("TINY: open error = 0x%x\n", ret);
		return -ENODEV;
	}
	
	DEBUG(DL3,"TINY[O]: volume(%d), minor(%d)\n", volume, minor);

	return 0;
}

/**
 * bml_block_release - BML block release operation
 * @param inode         block device inode
 * @param file          block device file
 * @return              0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int bml_block_release(struct gendisk *disk, fmode_t mode)
{
	u32 volume, minor;

	minor = disk->first_minor;
#else
static int bml_block_release(struct inode *inode, struct file *file)
{
	u32 volume, minor;
	
	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	
	DEBUG(DL3,"TINY[I]: volume(%d), minor(%d)\n", volume, minor);

	FSR_BML_Close(volume, FSR_BML_FLAG_NONE);
	
	DEBUG(DL3,"TINY[O]: volume(%d), minor(%d)\n", volume, minor);

	return 0;
}

/**
 * FSR common block device operations
 */
static struct block_device_operations bml_block_fops = 
{
	.owner          = THIS_MODULE,
	.open           = bml_block_open,
	.release        = bml_block_release,
};

/**
 * bml_get_block_device_ops
 * @return              FSR common block device operations
 */
struct block_device_operations *bml_get_block_device_operations(void)
{
	return &bml_block_fops;
}

/**
 * BML block module init
 * @return      0 on success
 */
int __init bml_block_init(void)
{

	DEBUG(DL3,"TINY[I]\n");

	if (bml_blkdev_init() == 0)
	{
		printk("FSR: Registered TinyFSR Driver.\n");
	}
	else
	{
		ERRPRINTK("FSR: Couldn't register TinyFSR Driver.\n");
	}
	
	DEBUG(DL3,"TINY[O]\n");

	return 0;
}

/**
 * BML block module exit
 */
void __exit bml_block_exit(void)
{
	DEBUG(DL3,"TINY[I]\n");

	bml_blkdev_exit();

	DEBUG(DL3,"TINY[O]\n");
}

