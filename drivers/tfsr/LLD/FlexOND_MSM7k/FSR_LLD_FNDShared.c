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
 * @file      FSR_LLD_FNDShared.c
 * @brief     This file declares the shared variable for supporting the linux
 * @author    NamOh Hwang
 * @date      1-Aug-2007
 * @remark
 * REVISION HISTORY
 * @n  1-Aug-2007 [NamOh Hwang] : first writing
 *
 */

/******************************************************************************/
/* Header file inclusions                                                     */
/******************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"
#include    "FSR_LLD_FlexOND.h"

#define     FSR_LLD_LOGGING_HISTORY

#if defined(FSR_ONENAND_EMULATOR)
#define     FSR_LLD_LOGGING_HISTORY
#endif

#if defined(FSR_NBL2)
#undef      FSR_LLD_LOGGING_HISTORY
#endif

/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/
#if defined(TINY_FSR)
volatile FlexONDSharedCxt gstFNDSharedCxt[FSR_MAX_DEVS] =
{
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_FND_PREOP_NONE, FSR_FND_PREOP_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_FND_PREOP_NONE, FSR_FND_PREOP_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_FND_PREOP_NONE, FSR_FND_PREOP_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_FND_PREOP_NONE, FSR_FND_PREOP_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_FND_PREOP_ADDRESS_NONE, FSR_FND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}}
};

#if defined(FSR_LINUX_OAM)
EXPORT_SYMBOL(gstFNDSharedCxt);
#endif

#endif /* defined(TINY_FSR)*/

#if defined(FSR_LLD_LOGGING_HISTORY)

#if defined(TINY_FSR) || !defined(FSR_LLD_HANDSHAKE_ERR_INF)

volatile FlexONDOpLog gstFNDOpLog[FSR_MAX_DEVS] =
{
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}}
};

#if defined(FSR_LINUX_OAM)
EXPORT_SYMBOL(gstFNDOpLog);
#endif

#endif

#endif  /* #if defined(FSR_LLD_LOGGING_HISTORY) */
