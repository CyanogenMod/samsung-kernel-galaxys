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
 * @file      FSR_OND_Shared.c
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
#include    "FSR_LLD_OneNAND.h"

/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/
#if defined(FSR_LLD_HANDSHAKE_ERR_INF)

volatile OneNANDSharedCxt gstONDSharedCxt[FSR_MAX_DEVS] =
{
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_PREOP_NONE, FSR_OND_PREOP_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_PREOP_NONE, FSR_OND_PREOP_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_PREOP_NONE, FSR_OND_PREOP_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_PREOP_NONE, FSR_OND_PREOP_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_OND_PREOP_ADDRESS_NONE, FSR_OND_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}}
};

#endif
