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
 *  @file       FSR_BML_OpQMgr.h
 *  @brief      This header defines Data types which are shared by all BML submodules.
 *  @author     SuRuyn Lee
 *  @author     MinYoung Kim
 *  @date       20-FEB-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  20-FEB-2007 [SuRyun Lee] : first writing
 *
 */

#ifndef _FSR_BML_NONBLKMGR_H_
#define _FSR_BML_NONBLKMGR_H_

/****************************************************************************/
/*Common Constant definitions for nonblocking mode                          */
/****************************************************************************/
/* nFlag & mask for interleaving */
#define     BML_NBM_FLAG_START_SAME_OPTYPE      (0x00004000)
#define     BML_NBM_FLAG_CONTINUE_SAME_OPTYPE   (0x00008000)
#define     BML_NBM_FLAG_END_OF_SAME_OPTYPE     (0x00000000)

#define     BML_NBM_MASK_SAME_OPTYPE            (BML_NBM_FLAG_START_SAME_OPTYPE     | \
                                                 BML_NBM_FLAG_CONTINUE_SAME_OPTYPE  | \
                                                 BML_NBM_FLAG_END_OF_SAME_OPTYPE)

/* Mask of CopyBack_Load for Nonblocking mode */
#define     BML_FLAG_CPBK_LOAD                  (0x00000001)

/* Flag to set interrupt bit for LLD func. */
#define     BML_CLEAR_INTERRUPT_BIT             (0x00000000)
#define     BML_SET_INTERRUPT_BIT               (0x00000020)

/****************************************************************************/
/* External Functions                                                       */
/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************/
/* APIs to map the LLD functions to Non-Blocking functions                  */
/****************************************************************************/
#if !defined(FSR_NBL2)
INT32       FSR_NBM_Read        (UINT32         nDev, 
                                 UINT32         nPbn,
                                 UINT32         nPgOffset,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nFlag);
INT32       FSR_NBM_Write       (UINT32         nDev, 
                                 UINT32         nPbn,
                                 UINT32         nPgOffset,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nFlag);
INT32       FSR_NBM_CopyBack    (UINT32         nDev,
                                 LLDCpBkArg    *pstCpArg,
                                 UINT32         nFlag);
INT32       FSR_NBM_Erase       (UINT32         nDev,
                                 UINT32        *pnPbn,
                                 UINT32         nNumOfBlks,
                                 UINT32         nFlag);
#endif /* FSR_NBL2 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_BML_OPQMGR_H_ */

