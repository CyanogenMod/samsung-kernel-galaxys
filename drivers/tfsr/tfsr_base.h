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
 * @file	drivers/tfsr/tfsr_base.h
 * @brief	The most commom part and some inline functions to mainipulate
 *		the FSR instance (volume specification, partition table)
 *
 * 
 */

#ifndef _FSR_BASE_H_
#define _FSR_BASE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/fsr_if.h>
#include "debug.h"

#include <FSR.h>
//#include <FsrTypes.h>
#include <FSR_OAM.h>
#include <FSR_PAM.h>
#include <FSR_BML.h>
#include <FSR_LLD.h>
#ifdef CONFIG_PM
#include <../Core/BML/FSR_BML_Types.h>
#include <../Core/BML/FSR_BML_BIFCommon.h>
#endif

/*
 * kernel version macro
 */
#undef FSR_FOR_2_6
#undef FSR_FOR_2_6_14
#undef FSR_FOR_2_6_15
#undef FSR_FOR_2_6_19

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#define FSR_FOR_2_6             1
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#define FSR_FOR_2_6_19          1
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
#define FSR_FOR_2_6_15          1
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#define FSR_FOR_2_6_14          1
#endif

#define SECTOR_SIZE             512
#define SECTOR_BITS             9
#define OOB_SIZE		16
#define OOB_BITS		4
#define SECTOR_MASK             MASK(SECTOR_BITS)
#define MAX_LEN_PARTITIONS	(sizeof(FSRPartI))

#define TINYFSR_PROC_DIR	"tinyFSR"

extern struct semaphore fsr_mutex;

FSRVolSpec *fsr_get_vol_spec(u32 volume);
FSRPartI   *fsr_get_part_spec(u32 volume);

static inline unsigned int fsr_minor(unsigned int vol, unsigned int part)
{
	return ((vol << PARTITION_BITS) + (part + 1));
}

static inline unsigned int fsr_vol(unsigned int minor)
{
	return (minor >> PARTITION_BITS);
}

/*Get partition*/
static inline unsigned int fsr_part_spu(FSRVolSpec *volume, FSRPartI *pi, u32 partno)
{
	if(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_SLC)
	{
		return (volume->nSctsPerPg * volume->nPgsPerSLCUnit);
	}
	else
	{
		return (volume->nSctsPerPg * volume->nPgsPerMLCUnit);
	}
}

static inline unsigned int fsr_part_ppu(FSRVolSpec *volume, FSRPartI *pi, u32 partno)
{
	if(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_SLC)
	{
		return volume->nPgsPerSLCUnit;
	}
	else
	{
		return volume->nPgsPerMLCUnit;
	}
}

static inline unsigned int fsr_part(unsigned int minor)
{
	return (minor & PARTITION_MASK) - 1;
}

static inline unsigned int fsr_is_whole_dev(unsigned int part_no)
{
	return (part_no >> PARTITION_BITS);
}

static inline unsigned int fsr_parts_nr(FSRPartI *pt)
{
	return (pt->nNumOfPartEntry);
}

static inline FSRPartEntry *fsr_part_entry(FSRPartI *pt, unsigned int no)
{
	return &(pt->stPEntry[no]);
}

static inline unsigned int fsr_part_units_nr(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfUnits);
}

static inline unsigned int fsr_part_id(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nID);
}

static inline unsigned int fsr_part_attr(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nAttr);
}

static inline unsigned int fsr_part_start(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].n1stVun);
}

static inline unsigned int fsr_part_size(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfUnits << SECTOR_BITS);
}

/*Get volume*/
static inline unsigned int fsr_vol_spp(FSRVolSpec *volume)
{
	return (volume->nSctsPerPg);
}

static inline unsigned int fsr_vol_sectors_nr(u32 volume)
{
	FSRVolSpec *vs;
	FSRPartI *pi;
	FSRPartEntry pe;
	u32 n1stVpn, nPgsPerUnit;
	
	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);
	pe = pi->stPEntry[pi->nNumOfPartEntry - 1];

	if(FSR_BML_GetVirUnitInfo(volume, 
		fsr_part_start(pi, pi->nNumOfPartEntry - 1), 
		&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS)
	{
		printk("FSR_BML_GetVirUnitInfo FAIL\r\n");
	}

	return (n1stVpn + pe.nNumOfUnits * nPgsPerUnit) * vs->nSctsPerPg;
}

static inline unsigned int fsr_vol_pages_nr(u32 volume)
{
	FSRPartI *pi;
	FSRPartEntry pe;
	u32 n1stVpn, nPgsPerUnit;

	pi = fsr_get_part_spec(volume);
	pe = pi->stPEntry[pi->nNumOfPartEntry - 1];

	if(FSR_BML_GetVirUnitInfo(volume,
				fsr_part_start(pi, pi->nNumOfPartEntry - 1),
				&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS)
	{
		printk("FSR_BML_GetVirUnitInfo FAIL\r\n");
	}
	return n1stVpn + (pe.nNumOfUnits * nPgsPerUnit);
}

static inline unsigned int fsr_vol_unitsize(u32 volume, u32 partno)
{
	FSRVolSpec *vs;
	FSRPartI *pi;
	
	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);

	if(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_SLC)
	{
		return (vs->nSctsPerPg * vs->nPgsPerSLCUnit) 
			<< SECTOR_BITS;
	}
	else
	{
		return (vs->nSctsPerPg * vs->nPgsPerMLCUnit) 
			<< SECTOR_BITS;
	}
}

static inline unsigned int fsr_vol_unit_nr(FSRVolSpec *volume)
{
	 return (volume->nNumOfUsUnits);
}

int __init bml_block_init(void);
void __exit bml_block_exit(void);

int fsr_init_partition(u32 volume);
int fsr_update_vol_spec(u32 volume);
struct block_device_operations *bml_get_block_device_operations(void);
int bml_blkdev_init(void);
void bml_blkdev_exit(void);

int stl_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize);
stl_info_t *fsr_get_stl_info(u32 volume, u32 partno);
struct block_device_operations *stl_get_block_device_operations(void);
void stl_blkdev_clean(u32 first_minor, u32 nparts);
int stl_blkdev_init(void);
void stl_blkdev_exit(void);

static inline unsigned int fsr_stl_sectors_nr(stl_info_t *ssp)
{
	return (ssp->total_sectors);
}

static inline unsigned int fsr_stl_page_size(stl_info_t *ssp)
{
	return (ssp->page_size);
}

/* kernel 2.6 */
#include <linux/version.h>
#ifdef FSR_FOR_2_6
#include <linux/blkdev.h>

struct fsr_dev 
{
	struct request		*req;        
	struct list_head	list;
	int			size;
	spinlock_t		lock;
	struct request_queue	*queue;
	struct gendisk		*gd;
	int			dev_id;
	struct scatterlist	*sg;
};
#else
/* Kernel 2.4 */
#ifndef __user
#define __user
#endif
#endif

#endif	/* _FSR_BASE_H_ */
