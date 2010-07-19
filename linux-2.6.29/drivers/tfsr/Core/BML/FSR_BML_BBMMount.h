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
 * @file      FSR_BML_BBMMount.h
 * @brief     This file contains the definition and prototypes of exported
 * @n         functions for Bad Block Manager 
 * @author    MinYoung Kim
 * @date      15-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  15-JAN-2007 [MinYoung Kim] : first writing
 *
 */ 

#ifndef     _FSR_BML_BBMMOUNT_H_
#define     _FSR_BML_BBMMOUNT_H_

/*****************************************************************************/
/* exported function prototype of Bad Block Manager                          */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32  FSR_BBM_Mount                (BmlVolCxt   *pstVol,
                                     BmlDevCxt   *pstDev, 
                                     FSRPartI    *pstPI,
                                     FSRPIExt    *pstPExt);
INT32  FSR_BBM_UnlockRsvr           (BmlVolCxt   *pstVol,
                                     BmlDevCxt   *pstDev,
                                     UINT32       nDieIdx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_BML_BBMMOUNT_H_ */
