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
 * @file      FSR_LLD_SWEcc.h
 * @brief     SW Error Correction Code header file (Hamming Algorithm)
 * @author    JeongWook Moon
 * @date      08-MAY-2007
 * @remark
 * REVISION HISTORY
 * @n  27-DEC-2002 [Kwangyoon Lee]  : first writing
 * @n  15-JUL-2003 [SeWook Na]      : code modification
 * @n  11-AUG-2003 [Janghwan Kim]   : code modification
 * @n  02-OCT-2003 [Janghwan Kim]   : reorganization
 * @n  08-MAY-2007 [JeongWook Moon] : delete pre-API and add ECC parity code of 6bytes
 *
 */


#ifndef _FSR_LLD_SWECC_H_
#define _FSR_LLD_SWECC_H_

/*****************************************************************************/
/* Common Constant Definition                                                */
/*****************************************************************************/
#define M_AREA                       0
#define S_AREA                       1


/*****************************************************************************/
/* ECC Data Strcutuers                                                       */
/*****************************************************************************/
#define FSR_OND_SWECC_E_ERROR                1    /* ECC error                       */
#define FSR_OND_SWECC_N_ERROR                0    /* no error                        */
#define FSR_OND_SWECC_C_ERROR               -1    /* one bit data error              */
#define FSR_OND_SWECC_U_ERROR               -2    /* uncorrectable error             */
#define FSR_OND_SWECC_R_ERROR               -3    /* one bit data error by read      */
                                                  /* disturbance                     */


//#define BW_X08                       LLD_BW_X08 /* 0 */
//#define BW_X16                       LLD_BW_X16 /* 1 */

/*****************************************************************************/
/* ECC External Function Declarations                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

PUBLIC VOID  FSR_OND_ECC_GenS  (volatile UINT16 *pEcc,  
                                         UINT8  *pBuf);

PUBLIC INT32 FSR_OND_ECC_CompS (UINT8  *pEcc2, 
                                UINT8  *pBuf,
                                UINT32  nSectNum);

#ifdef __cplusplus
}
#endif /* __cplusplus */    


#endif /* _FSR_LLD_SWECC_H_ */
