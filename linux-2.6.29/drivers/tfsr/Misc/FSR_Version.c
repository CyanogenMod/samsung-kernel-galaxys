/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b032-FSR_1.2.1p1_b129_RTM
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *---------------------------------------------------------------------------*
 *                                                                           *
 * Copyright (C) 2003-2010 Samsung Electronics                               *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License version 2 as         *
 * published by the Free Software Foundation.                                *
 *                                                                           *
 *---------------------------------------------------------------------------*
 *
 *     @section Description
 *
 */

/**
 * @file      FSR_Version.c
 * @brief     This file contains the version of FSR.
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : first writing
 *
 */

#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"

/** FSR verion code and string macro */
FSR_VERSION(FSR_VER_MAJOR, FSR_VER_MINOR1, FSR_VER_MINOR2, FSR_VER_PATCHLEVEL, FSR_BUILD_NUMBER)

/**
 * @brief           this function gets FSR version and returns FSR version string
 *
 * @param[out]     *pnVerCode : version code
 *
 * @return          FSR version string ("FSR_v1.1.0p0_release")
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function gets FSR version and
 *                  returns FSR version string
 *
 */
PUBLIC UINT8 *
FSR_Version(UINT32 *pnVerCode)
{
    *pnVerCode = gnFSRVerCode;
    return (UINT8 *) gpaFSRVerStr;
}

/**
 * @brief           this function returns FSR version code
 *
 * @return          FSR version code (v1.2.3p4 : 0x01020304)
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC UINT32
FSR_VersionCode(VOID)
{
    return gnFSRVerCode;
}

/**
 * @brief           this function returns FSR build number
 *
 * @return          FSR build number
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC UINT32
FSR_BuildNumber(VOID)
{
    return FSR_BUILD_NUMBER;
}
