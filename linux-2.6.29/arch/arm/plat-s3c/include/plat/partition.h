/* linux/arch/arm/plat-s3c/include/plat/partition.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Partition information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/mach/flash.h>

struct mtd_partition s3c_partition_info[] = {
#if defined(CONFIG_SPLIT_ROOT_FOR_ANDROID)
        {
                .name          = "misc",
                .offset                = (768*SZ_1K),          /* for bootloader */
                .size          = (256*SZ_1K),
                .mask_flags    = MTD_CAP_NANDFLASH,
        },
        {
                .name          = "recovery",
                .offset                = MTDPART_OFS_APPEND,
                .size          = (5*SZ_1M),
//                .mask_flags  = MTD_CAP_NANDFLASH,
        },
        {
                .name          = "kernel",
                .offset                = MTDPART_OFS_APPEND,
                .size          = (3*SZ_1M),
        },
        {
                .name          = "ramdisk",
                .offset                = MTDPART_OFS_APPEND,
                .size          = (1*SZ_1M),
        },
        {
                .name          = "system",
                .offset                = MTDPART_OFS_APPEND,
                .size          = (67*SZ_1M),
        },
        {
                .name          = "cache",
                .offset                = MTDPART_OFS_APPEND,
                .size          = (67*SZ_1M),
        },
        {
                .name          = "userdata",
                .offset                = MTDPART_OFS_APPEND,
                .size          = MTDPART_SIZ_FULL,
        }
#else


        {
                .name		= "Bootloader",
                .offset		= 0,
                .size		= (512*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "Kernel",
                .offset		= (512*SZ_1K),
                .size		= (4*SZ_1M) - (512*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
#if defined(CONFIG_SPLIT_ROOT_FILESYSTEM)
        {
                .name		= "Rootfs",
                .offset		= (4*SZ_1M),
                .size		= (48*SZ_1M),
        },
#endif
        {
                .name		= "File System",
                .offset		= MTDPART_OFS_APPEND,
                .size		= MTDPART_SIZ_FULL,
        }
#endif //ADNROID
};

struct s3c_nand_mtd_info s3c_nand_mtd_part_info = {
	.chip_nr = 1,
	.mtd_part_nr = ARRAY_SIZE(s3c_partition_info),
	.partition = s3c_partition_info,
};

struct flash_platform_data s3c_onenand_data = {
	.parts		= s3c_partition_info,
	.nr_parts	= ARRAY_SIZE(s3c_partition_info),
};

