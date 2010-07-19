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
 * @file      FSR_BML_BBMMount.c
 * @brief     This file contains the routine for mounting of reservoir
 * @author    MinYoung Kim
 * @date      30-MAY-2007
 * @remark
 * REVISION HISTORY
 * @n  30-MAY-2007 [MinYoung Kim] : first writing
 *
 */
 
/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/

#define  FSR_NO_INCLUDE_STL_HEADER
#include "FSR.h"

#include "FSR_BML_Config.h"
#include "FSR_BML_Types.h"
#include "FSR_BML_BBMCommon.h"
#include "FSR_BML_BBMMount.h"

#if !defined(TINY_FSR)
#include "FSR_BML_BadBlkMgr.h"
#endif /* TINY_FSR */

/*****************************************************************************/
/* Function prototype                                                        */
/*****************************************************************************/

PRIVATE INT32   _Mount              (BmlVolCxt     *pstVol, 
                                     BmlDevCxt     *pstDev, 
                                     FSRPartI      *pstPI, 
                                     FSRPIExt      *pstPExt, 
                                     UINT32         nDieIdx);
PRIVATE INT32   _ScanReservoir      (BmlDevCxt     *pstDev, 
                                     BmlVolCxt     *pstVol, 
                                     UINT32         nDieIdx);
PRIVATE BOOL32  _IsFreePg           (BmlVolCxt     *pstVol, 
                                     BmlReservoir  *pstRsv, 
                                     UINT32         nPDev, 
                                     UINT32         nPbn, 
                                     UINT32         nPgOffset);
PRIVATE BOOL32  _FindValidMetaData  (BmlDevCxt     *pstDev, 
                                     BmlVolCxt     *pstVol, 
                                     BmlDieCxt     *pstDie,
                                     UINT32         nPbn, 
                                     UINT32        *pValidOffset, 
                                     UINT32         nType);
PRIVATE BOOL32  _LoadBMS            (BmlVolCxt     *pstVol,
                                     BmlReservoir  *pstRsv, 
                                     BmlBMS        *pstBMS);
PRIVATE INT32   _LoadMetaData       (BmlVolCxt     *pstVol, 
                                     BmlDevCxt     *pstDev,
                                     UINT32         nDieIdx,
                                     UINT32         nPbn, 
                                     UINT32         nStartOffset, 
                                     VOID          *pstPI, 
                                     UINT32         nType);

/*
 * @brief           This function mounts BmlReservoir of the given (nPDev) device.
 * @n               Actually, it search for latest LPCB, UPCB. And load PI and PIExt.
 * @n               Then, it loads the latest BMI.
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 * @param[in]      *pstPI       : pointer for partition info
 * @param[in]      *pstPExt     : pointer for partition extension info
 *
 * @return          FSR_BML_SUCCESS  
 * @return          FSR_BML_CRITICAL_ERROR  
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC INT32
FSR_BBM_Mount(BmlVolCxt    *pstVol, 
              BmlDevCxt    *pstDev, 
              FSRPartI     *pstPI, 
              FSRPIExt     *pstPExt)
{   
    INT32       nRet = FSR_BML_SUCCESS;
    UINT32      nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;
    
    FSR_ASSERT((pstVol != NULL) && (pstDev != NULL));

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(PDev: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo));

    do
    {
        /* calculate BmlReservoir size before mounting */
        nRet = _CalcRsvrSize(pstVol, pstDev, NULL, FALSE32);
        if (nRet != FSR_BML_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _CalcRsvrSize(nFlag: 0x%x) is failed\r\n"), FALSE32));
            break;
        }

        /* initialize memory and set variables for BmlReservoir */
        _InitBBM(pstVol, pstDev);

        /* set volume context */
        _SetVolCxt(pstVol, pstDev);
        
        /* In case of DDP, do BBM mount for each die */
        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            /* mount for each die */
            nRet = _Mount(pstVol, pstDev, pstPI, pstPExt, nDieIdx);       
            if (nRet != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _Mount(nDie:%d) is failed\r\n"), nDieIdx));
                break;
            }
        }

        if (nRet != FSR_BML_SUCCESS)
        {
            break;
        }
        
    } while (0);
    
    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));

    return nRet;
}

/**
 * @brief          this function unlock blocks in the reservoir
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 * @param[in]       nDieIdx     : die index
 *
 * @return          FSR_BML_SUCCESS  
 * @return          FSR_BML_CANT_UNLOCK_BLOCK  
 * @return          FSR_BML_CRITICAL_ERROR  
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC INT32
FSR_BBM_UnlockRsvr(BmlVolCxt   *pstVol,
                   BmlDevCxt   *pstDev,
                   UINT32       nDieIdx)
{
    LLDProtectionArg    stLLDPrtArg;
    BmlReservoir       *pstRsv;
    BmlReservoirSh     *pstRsvSh;
    BmlBMI             *pstBMI;
    UINT32              nSbn;
    UINT32              nIdx;
    UINT32              nByteReturned;
    UINT32              nErrPbn;
    UINT32              nLockState;
    INT32               nLLDRe;
    INT32               nRet = FSR_BML_SUCCESS;
    UINT32              nNumOfRemainLockedRCB;

    FSR_STACK_VAR;

    FSR_STACK_END;
    
    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(PDev: %d, nDieIdx: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, nDieIdx));

    pstRsv      = pstDev->pstDie[nDieIdx]->pstRsv;
    pstRsvSh    = pstDev->pstDie[nDieIdx]->pstRsvSh;

    pstBMI = pstRsv->pstBMI;

    do
    {
        /* unlock reservoir meta blocks or unmapped reservoir block */
        for (nSbn = pstRsv->n1stSbnOfRsvr; nSbn <= pstRsv->nLastSbnOfRsvr; nSbn++)
        {
            /* unmapped area or BBM meta block */
            if ((_IsAllocRB(pstRsv, nSbn) != TRUE32) ||
                (nSbn == pstRsvSh->nUPCBSbn)           ||
                (nSbn == pstRsvSh->nLPCBSbn)           ||
                (nSbn == pstRsvSh->nTPCBSbn)           ||
                (nSbn == pstRsvSh->nREFSbn))
            {
                /* get lock state of the block */
                nLLDRe = pstVol->LLD_IOCtl(pstDev->nDevNo,
                                           FSR_LLD_IOCTL_GET_LOCK_STAT,
                                           (UINT8 *) &nSbn,
                                           sizeof(nSbn),
                                           (UINT8*) &nLockState,
                                           sizeof(nLockState),
                                           &nByteReturned);
                if (nLLDRe != FSR_LLD_SUCCESS)
                {  
                    nRet = FSR_BML_CRITICAL_ERROR;
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR] Can not get lock status of RCB block (Skip this block)\r\n")));
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR] nPDev: %d, nOrgSbn: %d, nLLDRe: 0x%x\r\n"), pstDev->nDevNo, nSbn, nLLDRe));
                    break;
                }

                if (nLockState == FSR_LLD_BLK_STAT_LOCKED)  
                {
                    /* Set the parameter of LLD_IOCtl() */
                    stLLDPrtArg.nStartBlk = nSbn;
                    stLLDPrtArg.nBlks     = 1;

                    nLLDRe = pstVol->LLD_IOCtl(pstDev->nDevNo,
                                               FSR_LLD_IOCTL_UNLOCK_BLOCK,
                                               (UINT8 *) &stLLDPrtArg,
                                               sizeof(stLLDPrtArg),
                                               (UINT8 *) &nErrPbn,
                                               sizeof(nErrPbn),
                                               &nByteReturned);
                    if (nLLDRe != FSR_LLD_SUCCESS)
                    {
                        nRet= FSR_BML_CANT_UNLOCK_BLOCK;
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Can't unlock BmlReservoir blocks (Skip this block, nPDev: %d, nSbn: %d)\r\n"),
                                                       pstDev->nDevNo, nSbn));                         
                        break;
                    }
                }

                /*
                 * if (nLockState == FSR_LLD_BLK_STAT_LOCKED_TIGHT), skip current block 
                 * if current block is already locked tightly, 
                 * BBM does not try to unlock the block.
                 */

            }
        }

        if (nRet != FSR_BML_SUCCESS)
        {
            break;
        }

        /* multi plane case */
        if (pstVol->nNumOfPlane > 1)
        {
            /* unlock unused reservoir candidate blocks */
            for ( nNumOfRemainLockedRCB   = pstBMI->nNumOfRCBs, nIdx = 0;
                  ( nNumOfRemainLockedRCB > 0 ) && ( nIdx < ( ( pstRsv->nNumOfRsvrInSLC + pstRsv->nNumOfRsvrInMLC ) / 2 ) );
                  nIdx ++ )
            {
                /* if the block is in the RCB queue, the block is unlocked */
                nSbn = pstRsv->pstRCB[nIdx];

                /* If the getting RCB is already in use, Don't touch */
                if ( nSbn == 0xFFFF )
                {
                    continue;
                }

                /* get lock state of the block */
                nLLDRe = pstVol->LLD_IOCtl(pstDev->nDevNo,
                                           FSR_LLD_IOCTL_GET_LOCK_STAT,
                                           (UINT8 *) &nSbn,
                                           sizeof(nSbn),
                                           (UINT8*) &nLockState,
                                           sizeof(nLockState),
                                           &nByteReturned);
                if (nLLDRe != FSR_LLD_SUCCESS)
                {
                    nRet = FSR_BML_CRITICAL_ERROR;
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR] Can not get lock status of RCB block\r\n")));
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR] nPDev: %d, nOrgSbn: %d, nLLDRe: 0x%x\r\n"), pstDev->nDevNo, nSbn, nLLDRe));
                    break;
                }

                /* if the block is locked */
                if (nLockState == FSR_LLD_BLK_STAT_LOCKED)
                {
                    /* Set the parameter of LLD_IOCtl() */
                    stLLDPrtArg.nStartBlk = nSbn;
                    stLLDPrtArg.nBlks     = 1;

                    nLLDRe = pstVol->LLD_IOCtl(pstDev->nDevNo,
                                               FSR_LLD_IOCTL_UNLOCK_BLOCK,
                                               (UINT8 *) &stLLDPrtArg,
                                               sizeof(stLLDPrtArg),
                                               (UINT8 *) &nErrPbn,
                                               sizeof(nErrPbn),
                                               &nByteReturned);
                    if (nLLDRe != FSR_LLD_SUCCESS)
                    {
                        nRet= FSR_BML_CANT_UNLOCK_BLOCK;
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Can't unlock free RCB blocks(nPDev: %d, nSbn: %d)\r\n"),
                                                       pstDev->nDevNo, nSbn)); 
                        break;
                    }
                }
                nNumOfRemainLockedRCB   --;

                /*
                 * if (nLockState == FSR_LLD_BLK_STAT_LOCKED_TIGHT), skip current block 
                 * if current block is already locked tightly, 
                 * BBM does not try to unlock the block.
                 */

            } /* end of for ( nNumOfRemainLockedRCB = pstBMI->nNumOfRCBs, ... */

        } /* end of if (multi-plane) */

    } while(0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));

    return nRet;
}

/*
 * @brief           This function mounts BmlReservoir of the given (nPDev) device.
 * @n               Actually, it search for latest LPCB, UPCB. And load PI and PIExt.
 * @n               Then, it loads the latest BMI.
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 * @param[in]      *pstPI       : pointer for partition info
 * @param[in]      *pstPExt     : pointer for partition extension info
 * @param[in]       nDieIdx     : index of die
 *
 * @return          FSR_BML_SUCCESS 
 * @return          FSR_BML_NO_LPCB
 * @return          FSR_BML_NO_UPCB
 * @return          FSR_BML_NO_TPCB
 * @return          FSR_BML_LOAD_LBMS_FAILURE
 * @return          FSR_BML_CRITICAL_ERROR  
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE INT32
_Mount(BmlVolCxt   *pstVol, 
       BmlDevCxt   *pstDev, 
       FSRPartI    *pstPI, 
       FSRPIExt    *pstPExt, 
       UINT32       nDieIdx)
{
    BmlReservoir   *pstRsv;
    BmlReservoirSh *pstRsvSh;
    INT32           nRet = FSR_BML_SUCCESS;
    BOOL32          bRet = FALSE32;
    UINT32          nValidOffset;
    UINT16          nPCBSbn;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPDev: %d, nDieIdx: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, nDieIdx));

    pstRsv      = pstDev->pstDie[nDieIdx]->pstRsv;
    pstRsvSh    = pstDev->pstDie[nDieIdx]->pstRsvSh;

    /* In order to search latest UPCB and latest LPCB, scan BmlReservoir */
    nRet = _ScanReservoir(pstDev, pstVol, nDieIdx);

    /* If there is no LPCB or no UPCB,
       it is unformated status or critical error */
    if (nRet != FSR_BML_SUCCESS)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _ScanReservoir(nPDev: %d, nDie:%d) is failed\r\n"), pstDev->nDevNo, nDieIdx));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));
        return nRet;
    }

    /* Find valid meta data for LPCB */
    bRet = _FindValidMetaData(pstDev, pstVol, pstDev->pstDie[nDieIdx], pstRsvSh->nLPCBSbn, &nValidOffset, BML_TYPE_LPCB);
    if (bRet == FALSE32)
    {
        /* If TPCB exists for backup */
        if (pstRsvSh->nTPCBSbn != 0)
        {
            /* Find valid meta data in TPCB */
            bRet = _FindValidMetaData(pstDev, pstVol, pstDev->pstDie[nDieIdx], pstRsvSh->nTPCBSbn, &nValidOffset, BML_TYPE_TPCB | BML_TYPE_LPCB);
            if (bRet == FALSE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _FindValidMetaData(nSbn:%d, LPCB) is failed\r\n"), pstRsvSh->nLPCBSbn));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_NO_LPCB));
                return FSR_BML_NO_LPCB;
            }

            /* swap LPCB with TPCB to restore previous data */
            nPCBSbn            = pstRsvSh->nLPCBSbn; 
            pstRsvSh->nLPCBSbn = pstRsvSh->nTPCBSbn;
            pstRsvSh->nTPCBSbn = nPCBSbn;
        }
    }

    /* Load meta data for LPCB */
    nRet = _LoadMetaData(pstVol, pstDev, nDieIdx, pstRsvSh->nLPCBSbn, nValidOffset, pstPI, BML_TYPE_LPCB);
    if (nRet != FSR_BML_SUCCESS)
    {        
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _LoadMetaData(nPDev: %d, nDieIdx: %d, nSbn:%d, nOffset: %d, LPCB) is failed\r\n"), 
                                       pstDev->nDevNo, nDieIdx, pstRsvSh->nLPCBSbn, nValidOffset));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_LOAD_LBMS_FAILURE));
        return FSR_BML_LOAD_LBMS_FAILURE;
    }

    /* Find valid meta data for UPCB */
    bRet = _FindValidMetaData(pstDev, pstVol, pstDev->pstDie[nDieIdx], pstRsvSh->nUPCBSbn, &nValidOffset, BML_TYPE_UPCB);
    if (bRet == FALSE32)
    {
        /* If TPCB exists for backup */
        if (pstRsvSh->nTPCBSbn != 0)
        {
            /* Find valid meta data in TPCB */
            bRet = _FindValidMetaData(pstDev, pstVol, pstDev->pstDie[nDieIdx], pstRsvSh->nTPCBSbn, &nValidOffset, BML_TYPE_TPCB | BML_TYPE_UPCB);
            if (bRet == FALSE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _FindValidMetaData(nSbn:%d, UPCB) is failed\r\n"), pstRsvSh->nUPCBSbn));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_NO_UPCB));
                return FSR_BML_NO_UPCB;
            }

            /* swap UPCB with TPCB to restore previous data */
            nPCBSbn            = pstRsvSh->nUPCBSbn; 
            pstRsvSh->nUPCBSbn = pstRsvSh->nTPCBSbn;
            pstRsvSh->nTPCBSbn = nPCBSbn;
        }
    }

    /* PIExt data is stored in die 0 of device 0 */
    if ((pstDev->nDevNo == 0) && (nDieIdx == 0))
    {
        /* Load meta data for UPCB */
        nRet = _LoadMetaData(pstVol, pstDev, nDieIdx, pstRsvSh->nUPCBSbn, nValidOffset, pstPExt, BML_TYPE_UPCB);
    }
    /* other cases */
    else
    {
        /* Load meta data for UPCB (Do not load PIExt) */
        nRet = _LoadMetaData(pstVol, pstDev, nDieIdx, pstRsvSh->nUPCBSbn, nValidOffset, NULL, BML_TYPE_UPCB);
    }

    if (nRet != FSR_BML_SUCCESS)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _LoadMetaData(nPDev: %d, nDieIdx: %d, nSbn:%d, nOffset: %d, UPCB) is failed\r\n"),
                                       pstDev->nDevNo, nDieIdx, pstRsvSh->nUPCBSbn, nValidOffset));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_LOAD_UBMS_FAILURE));
        return FSR_BML_LOAD_UBMS_FAILURE;
    }

    /* sorts by ascending power of pstBMI->pstBMF[].nSbn */
    _SortBMI(pstRsv);

    /* make BUMap to enhance performance of address translation */ 
    _ReconstructBUMap(pstVol, pstRsv, nDieIdx);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));

    return nRet; 
}

/*
 * @brief           This function scans BmlReservoir to search latest UPCB/LPCB
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer 
 * @param[in]       nDieIdx     : Index of die
 *
 * @return          FSR_BML_SUCCESS
 * @return          FSR_BML_NO_UPCB
 * @return          FSR_BML_NO_LPCB
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE INT32
_ScanReservoir(BmlDevCxt   *pstDev,
               BmlVolCxt   *pstVol,
               UINT32       nDieIdx)
{
    FSRSpareBuf     stSBuf;
    BmlReservoir   *pstRsv;
    BmlReservoirSh *pstRsvSh;
    BmlPoolCtlHdr  *pstPCH;
    INT32           nLLDRe;
    INT32           nUPCHCmpRet;
    INT32           nLPCHCmpRet;
    UINT32          nPbn;
    UINT32          n1stSbnOfRsvr;
    UINT32          nLastSbnOfRsvr;
    UINT32          nCnt;
    UINT32          nDZMask;
    BOOL32          bUPCB = FALSE32;
    BOOL32          bLPCB = FALSE32;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPDev: %d, nDieIdx: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, nDieIdx));

    pstRsv      = pstDev->pstDie[nDieIdx]->pstRsv;
    pstRsvSh    = pstDev->pstDie[nDieIdx]->pstRsvSh;

    /* initialize age of PCB blocks */
    pstRsvSh->nUPcbAge = 0;
    pstRsvSh->nLPcbAge = 0;

    /* Flex-OneNAND: SLC + MLC type */
    if (pstRsv->nRsvrType == BML_HYBRID_RESERVOIR)
    {
        /* Set range of BmlReservoir */
        n1stSbnOfRsvr  = pstRsv->n1stSbnOfRsvr;
        nLastSbnOfRsvr = pstRsv->n1stSbnOfMLC - 1;
    }
    /* MLC only or SLC only */
    else
    {
        /* Set range of BmlReservoir */
        n1stSbnOfRsvr = pstRsv->n1stSbnOfRsvr;
        nLastSbnOfRsvr = pstRsv->nLastSbnOfRsvr;
    }

    FSR_OAM_MEMCPY(&stSBuf, pstRsv->pSBuf, sizeof(FSRSpareBuf));

    if (stSBuf.nNumOfMetaExt != 0)
    {
        stSBuf.nNumOfMetaExt = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:INF]   Scan from %d to %d to find BML meta blocks (die: %d)\r\n"), 
                                   n1stSbnOfRsvr, nLastSbnOfRsvr, nDieIdx));

    nCnt = 0;

    /* scan all blocks in BmlReservoir */
    for (nPbn = n1stSbnOfRsvr; nPbn <= nLastSbnOfRsvr; nPbn++)
    {
        /* Get current debug zone mask */
        nDZMask = FSR_DBG_GetDbgZoneMask();

        /* 
         * Unset all debug zone mask 
         * to remove uncorrectable read error msg at open time 
         */
        FSR_DBG_UnsetAllDbgZoneMask();

        /* 
         * Check 0th page in a block to check BBM meta mark in spare area
         */
        nLLDRe = _LLDRead(pstVol, 
                          pstDev->nDevNo,
                          nPbn, 
                          0, 
                          pstRsv, 
                          pstRsv->pMBuf,        /* main buffer pointer  */
                          &stSBuf,              /* spare buffer pointer */
                          BML_META_DATA,
                          FALSE32, 
                          FSR_LLD_FLAG_ECC_ON);

        /* restore previous debug zone mask */
        FSR_DBG_SetDbgZoneMask(nDZMask);

        /* 
         * if the block is an initial bad block 
         * BML_VALID_BLK_MARK = FSR_FND_VALID_BLK_MARK = FSR_OND_VALID_BLK_MARK
         */
        if (stSBuf.pstSpareBufBase->nBadMark != BML_VALID_BLK_MARK)
        {
            /* if this block is initial bad block, skip */
            continue;
        }

        /* uncorrectable read error case */
        if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
            (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_2LV_READ_DISTURBANCE) &&
            (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_1LV_READ_DISTURBANCE))  
        {
            /* skip and move to next block */
            continue;
        }

        if (nCnt % 10 == 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:INF]   ")));
        }

        /* print BBM meta mark area */
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("%x "), stSBuf.pstSpareBufBase->nBMLMetaBase0));

        if (nCnt % 10 == 9)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
        }

        nCnt++;

        /* if this block is not a BBM meta block */
        if (stSBuf.pstSpareBufBase->nBMLMetaBase0 != FSR_LLD_BBM_META_MARK)
        {
            /* skip and move to next block */
            continue;
        }

        pstPCH = (BmlPoolCtlHdr *) pstRsv->pMBuf;

        /* Change Byte-Order for Tool */
        FSR_BBM_CHANGE_BYTE_ORDER_BMLPOOLCTLHDR( pstPCH );

        /* check signature of UPCB and LPCB */
        nUPCHCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_UPCH_SIG, BML_MAX_PCH_SIG);
        nLPCHCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_LPCH_SIG, BML_MAX_PCH_SIG);

        /* UPCB */
        if (nUPCHCmpRet == 0)
        {
            if (pstPCH->nAge == 0xFFFFFFFF)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Invalid UPCB age(nPbn: %d)\r\n"), nPbn));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PCB_AGE));
                return FSR_BML_INVALID_PCB_AGE;
            }

            /* check age of UPCB */
            if (pstPCH->nAge > pstRsvSh->nUPcbAge)
            {
                pstRsvSh->nUPCBSbn    = (UINT16)nPbn;
                pstRsvSh->nUPcbAge    = pstPCH->nAge;
            }

            /* Valid UPCB */
            bUPCB = TRUE32;
        }
        /* LPCB */
        else if (nLPCHCmpRet == 0)
        {
            if (pstPCH->nAge == 0xFFFFFFFF)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Invalid LPCB age(nPbn: %d)\r\n"), nPbn));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PCB_AGE));
                return FSR_BML_INVALID_PCB_AGE;
            }

            /* check age of LPCB */
            if (pstPCH->nAge > pstRsvSh->nLPcbAge)
            {
                pstRsvSh->nLPCBSbn    = (UINT16)nPbn;
                pstRsvSh->nLPcbAge    = pstPCH->nAge;
            }
            
            bLPCB = TRUE32;
        }

        /* Update global age for PCB blocks */
        if (((nUPCHCmpRet == 0) || (nLPCHCmpRet == 0)) &&
            (pstPCH->nGlobalAge > pstRsvSh->nGlobalPCBAge))
        {
            pstRsvSh->nGlobalPCBAge = pstPCH->nGlobalAge;

            /* Get latest TPCB info */
            pstRsvSh->nTPCBSbn      = pstPCH->nTPCBPbn;
        }

    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    /* If UPCB is not found */
    if (bUPCB == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] NO UPCB(1st Sbn: %d, Last Sbn: %d)\r\n"), 
                                       n1stSbnOfRsvr, nLastSbnOfRsvr));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_NO_UPCB));
        return FSR_BML_NO_UPCB;
    }

    /* If LPCB is not found */
    if (bLPCB == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] NO LPCB(1st Sbn: %d, Last Sbn: %d)\r\n"), 
                                       n1stSbnOfRsvr, nLastSbnOfRsvr));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_NO_LPCB));
        return FSR_BML_NO_LPCB;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_SUCCESS));

    return FSR_BML_SUCCESS;
}

/*
 * @brief           This function reads data from the given nPbn and this 
 * @n               function checks confirmation page
 * 
 * @param[in]      *pstDev      : device context pointer 
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstRsv      : resercoir context pointer 
 * @param[in]       nPbn        : physical block number
 * @param[out]     *pValidOffset: page offset for valid meta data
 * @param[in]       nType       : PCB block type
 * @param[in]      *pbLastest   : data which will be loaded is lastest info 
 * @n                             or previous info
 *
 * @return          TRUE32
 * @return          FALSE32
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE BOOL32
_FindValidMetaData(BmlDevCxt       *pstDev, 
                   BmlVolCxt       *pstVol, 
                   BmlDieCxt       *pstDie, 
                   UINT32           nPbn, 
                   UINT32          *pValidOffset,
                   UINT32           nType)
{
    BmlReservoir   *pstRsv;
    BmlReservoirSh *pstRsvSh;
    FSRSpareBuf     stSBuf;
    BmlPoolCtlHdr  *pstPCH;
    INT32           nLLDRe;
    INT32           nCmpRet;
    UINT32          nPDev;
    UINT32          nPgOffset;
    UINT32          nLatestPgOff;
    BOOL32          bFree  = TRUE32;
    BOOL32          bFound = FALSE32;
    BOOL32          bRet   = FALSE32;
    UINT32          nIdx; 

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPDev: %d, nPbn: %d, nType: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, nPbn, nType));

    do
    {
        nPDev       = pstDev->nDevNo;

        pstRsv      = pstDie->pstRsv;
        pstRsvSh    = pstDie->pstRsvSh;

        *pValidOffset = 0xFFFFFFFF;

        /* 
         * Find the latest confirm page 
         * Read the specific page offset to reduce the number of read operations
         * ex) 3, 7, 11, ..., 63
         */
        for (nPgOffset = pstVol->nNumOfPgsInSLCBlk; 
             nPgOffset > 0; 
             nPgOffset -= (BML_NUM_OF_META_PGS + 1))
        {
            /* checks this page is free or not */
            bFree = _IsFreePg(pstVol, pstRsv, nPDev, nPbn, nPgOffset - 1);

            /* If the page is not free, the page contains confirm in main area */
            if (bFree == FALSE32)
            {
                break;
            }
        }

        /* If all pages are free */
        if (bFree == TRUE32)
        {
            break;
        }

        /* UPCB case */
        if ((nType & BML_TYPE_UPCB) == BML_TYPE_UPCB)
        {
            pstRsvSh->nNextUPCBPgOff = (UINT16)nPgOffset;
        }
        /* LPCB case */
        else if ((nType & BML_TYPE_LPCB) == BML_TYPE_LPCB)
        {
            pstRsvSh->nNextLPCBPgOff = (UINT16)nPgOffset;
        }

        /* Confirm page offset */
        nPgOffset--;

        /* save latest written page offset */
        nLatestPgOff = nPgOffset;

        FSR_OAM_MEMCPY(&stSBuf, pstRsv->pSBuf, sizeof(FSRSpareBuf));
        if (stSBuf.nNumOfMetaExt != 0)
        {
            stSBuf.nNumOfMetaExt = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
        }

        do
        {
            /* read confirm page */
            nLLDRe = _LLDRead(pstVol,
                              nPDev,
                              nPbn, 
                              nPgOffset,
                              pstRsv,
                              pstRsv->pMBuf,        /* main buffer pointer  */
                              &stSBuf,              /* spare buffer pointer */
                              BML_META_DATA,
                              FALSE32,
                              FSR_LLD_FLAG_ECC_ON);
            
            if ((FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_SUCCESS) ||
                (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_2LV_READ_DISTURBANCE) ||
                (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_1LV_READ_DISTURBANCE)) 
            {
                /* Data of confirm page should be all 0x0 */
                bRet = _CmpData(pstVol, pstRsv->pMBuf, 0x0, FALSE32);
                if (bRet == TRUE32)
                {
                    bFound = TRUE32;
                    
                    for (nIdx = nPgOffset - BML_NUM_OF_META_PGS; 
                         nIdx < nPgOffset;
                         nIdx++)
                    {
                        /* if valid confirm page is found, read meta-data pages */
                        nLLDRe = _LLDRead(pstVol, 
                                          nPDev, 
                                          nPbn, 
                                          nIdx, 
                                          pstRsv, 
                                          pstRsv->pMBuf,        /* main buffer pointer  */
                                          &stSBuf,              /* spare buffer pointer */
                                          BML_META_DATA, 
                                          FALSE32, 
                                          FSR_LLD_FLAG_ECC_ON);
                        if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
                            (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_2LV_READ_DISTURBANCE) &&
                            (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_1LV_READ_DISTURBANCE)) 
                        {
                            /* invalid meta data */
                            bFound = FALSE32;
                            break;
                        }
                    }

                    /* If all meta data pages are valid */
                    if (bFound == TRUE32)
                    {
                        /* 
                         * If lastest written page has valid info, load data from the latest pages.
                         * If not, load data from previous pages and write new data to newly allocated block
                         */
                        if ((nLatestPgOff == nPgOffset) && 
                            (nPgOffset < pstVol->nNumOfPgsInSLCBlk - 1))
                        {
                            /* checks next page is free or not */
                            bFree = _IsFreePg(pstVol, pstRsv, nPDev, nPbn, nPgOffset + 1);
                        }

                        /* first page offset of valid meta data */
                        *pValidOffset = nPgOffset - BML_NUM_OF_META_PGS;

                        /* 
                         * Find TPCB block to restore previous PCB data 
                         * when there is no valid PCB data 
                         */
                        if ((nType & BML_TYPE_TPCB) == BML_TYPE_TPCB)
                        {
                            /* read valid page offset to check the PCH signature (ignore error) */
                            _LLDRead(pstVol, 
                                     nPDev, 
                                     nPbn, 
                                     *pValidOffset, 
                                     pstRsv, 
                                     pstRsv->pMBuf,        /* main buffer pointer  */
                                     &stSBuf,              /* spare buffer pointer */
                                     BML_META_DATA, 
                                     FALSE32, 
                                     FSR_LLD_FLAG_ECC_ON);

                            pstPCH = (BmlPoolCtlHdr *) pstRsv->pMBuf;

                            /* Change Byte-Order for Tool */
                            FSR_BBM_CHANGE_BYTE_ORDER_BMLPOOLCTLHDR( pstPCH );

                            nCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_UPCH_SIG, BML_MAX_PCH_SIG);

                            /* the block has UPCH signature */
                            if ((nCmpRet == 0) && ((nType & BML_TYPE_UPCB) == BML_TYPE_UPCB))
                            {
                                break;
                            }

                            nCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_LPCH_SIG, BML_MAX_PCH_SIG);

                            /* the block has LPCH signature */
                            if ((nCmpRet == 0) && ((nType & BML_TYPE_LPCB) == BML_TYPE_LPCB))
                            {
                                break;
                            }

                            /* If type of TPCB and PCH signature is not matched */
                            bFound = FALSE32;
                            break;
                        }
                        /* not TPCB, UPCB or LPCB */
                        else
                        {
                            break;
                        }
                    }
                }
            }

            /* If this block has no valid data, escape from the loop */
            if ((nPgOffset == 0) && (bFound == FALSE32))
            {
                break;
            }

            /* power-failure case, move previous confirm page */
            nPgOffset--;

        /* go backward until valid confirm is found */
        } while (bFound == FALSE32);

    } while(0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, bFound));

    return bFound;
}

/*
 * @brief           This function checks the page is free or not
 *
 * @param[in]       pstRsv      : BmlReservoir context pointer
 * @param[in]       nPDev       : physical device number
 * @param[in]       nPbn        : physical block number
 * @param[in]       nPgOffset   : page offset to be read
 *
 * @return          TRUE32
 * @return          FALSE32
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE BOOL32
_IsFreePg(BmlVolCxt    *pstVol,
          BmlReservoir *pstRsv,
          UINT32        nPDev,
          UINT32        nPbn,
          UINT32        nPgOffset)
{
    FSRSpareBuf     stSBuf;
    BOOL32          bFree;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_OAM_MEMCPY(&stSBuf, pstRsv->pSBuf, sizeof(FSRSpareBuf));
    if (stSBuf.nNumOfMetaExt != 0)
    {
        stSBuf.nNumOfMetaExt = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
    }

    /* read a page and ignore LLD error */
    _LLDRead(pstVol,
             nPDev,
             nPbn,
             nPgOffset,
             pstRsv,
             pstRsv->pMBuf,        /* main buffer pointer  */
             &stSBuf,              /* spare buffer pointer */
             BML_META_DATA,
             FALSE32,
             FSR_LLD_FLAG_ECC_OFF);

    /* check spare buffer */
    bFree = _CmpData(pstVol, (UINT8 *) &stSBuf, 0xFF, TRUE32);
    if (bFree != TRUE32)
    {
        return bFree;
    }

    /* check main buffer */
    bFree = _CmpData(pstVol, (UINT8 *) pstRsv->pMBuf, 0xFF, FALSE32);
    if (bFree != TRUE32)
    {
        return bFree;
    }

    return TRUE32;
}

/*
 * @brief           This function loads BBM meta data from PCB
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 * @param[in]       nDieIdx     : die index
 * @param[in]       nPbn        : physical block number
 * @param[in]       nStartOffset: start page offset
 * @param[in]      *pstPI       : partition info pointer
 * @param[in]       nType       : PCB block type
 *
 * @return          FSR_BML_SUCCESS
 * @return          FSR_BML_LOAD_UBMS_FAILURE
 * @return          FSR_BML_LOAD_LBMS_FAILURE
 * @return          Some LLD errors
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE INT32
_LoadMetaData(BmlVolCxt *pstVol,
              BmlDevCxt *pstDev,
              UINT32     nDieIdx,
              UINT32     nPbn,
              UINT32     nStartOffset,
              VOID      *pstPI,
              UINT32     nType)
{ 
    BmlPoolCtlHdr  *pstPCH;
    FSRPartI       *pstPartI;
    FSRPIExt       *pstPIExt;
    BmlBMS         *pstBMS;
    BmlReservoir   *pstRsv;
    BmlReservoirSh *pstRsvSh;
    FSRSpareBuf     stSBuf;
    INT32           nLLDRe;
    INT32           nMajorRe = 0;
    INT32           nRet = FSR_BML_SUCCESS;
    UINT32          nIdx = 0;
    UINT32          nSctIdx;
    UINT32          nPDev;   
    UINT32          nNumOfRCB;
    UINT32          nNumOfBMS;
    UINT32          nBMSSctOffset;
    UINT32          nSizeOfMaxRsvr;
    UINT32          nSizeOfBABitMap;
    BOOL32          bLoaded = TRUE32;
    BOOL32          bRet = TRUE32;
    UINT8          *pBufPtr = NULL;
    UINT16         *pRCBSbn;
#if !defined(TINY_FSR)
    BOOL32          bReadDisturb = FALSE32;
#endif
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPDev: %d, nDieIdx: %d, nPbn: %d, nStartOffset: %d, nType: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, nDieIdx, nPbn, nStartOffset, nType));

    nPDev       = pstDev->nDevNo;

    pstRsv      = pstDev->pstDie[nDieIdx]->pstRsv;
    pstRsvSh    = pstDev->pstDie[nDieIdx]->pstRsvSh;

    /* 2KB page: 4, 4KB page: 8 */
    nBMSSctOffset = pstVol->nSctsPerPg;

    /* default value for 1 plane device */
    nNumOfRCB = 0;

    /* 2KB page device */
    if (pstVol->nSctsPerPg == BML_2KB_PG)
    {
        /* number of BMS sectors */
        nNumOfBMS = BML_NUM_OF_BMS_2KB_PG;

        /* max number of reservoir blocks */
        nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_2KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
    }
    /* 4KB page device */
    else
    {   
        /* number of BMS sectors */
        nNumOfBMS = BML_NUM_OF_BMS_4KB_PG;

        /* max number of reservoir blocks */
        nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_4KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
    }

    /* calculate size of BABitMap using nSizeOfMaxRsvr */
    if (nSizeOfMaxRsvr % BML_NUM_OF_BITS_IN_1BYTE)
    {
        nSizeOfBABitMap = (nSizeOfMaxRsvr / BML_NUM_OF_BITS_IN_1BYTE) + 1;
    }
    else
    {
        nSizeOfBABitMap = (nSizeOfMaxRsvr / BML_NUM_OF_BITS_IN_1BYTE);
    }

    /* multi plane device */
    if (pstVol->nNumOfPlane > 1)
    {
        /* 
         * number of entries for reservoir candidate block = 
         * max number of reservoir blocks which is supported by FSR / 2
         * 
         * max number of BMF entries = 3/2 * max number of reservoir blocks 
         *                           = nNumOfBMS * BML_BMFS_PER_BMS
         * 
         * max number of BMF entries / 3 = max number of reservoir blocks / 2
         *
         * number of sectors for reservoir candidate block = 
         *  ceiling of (number of entries for reservoir candidate block / number of RCB entries in sector) 
         *
         */
        nNumOfRCB = ((nNumOfBMS * BML_BMFS_PER_BMS) / 3);

        if (nNumOfRCB % BML_RCBFS_PER_RCBS)
        {
            nNumOfRCB = (nNumOfRCB / BML_RCBFS_PER_RCBS) + 1;
        }
        else
        {
            nNumOfRCB = (nNumOfRCB / BML_RCBFS_PER_RCBS);
        }
    }

    FSR_OAM_MEMCPY(&stSBuf, pstRsv->pSBuf, sizeof(FSRSpareBuf));
    if (stSBuf.nNumOfMetaExt != 0)
    {
        stSBuf.nNumOfMetaExt = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
    }

    /* read all meta pages : 3 pages(4KB page) or 6 pages(2KB page) */
    for (nSctIdx = 0; nSctIdx < (BML_NUM_OF_META_PGS * pstVol->nSctsPerPg); nSctIdx++)
    {
        /* when a sector is first sector of a page, read one page */
        if ((nSctIdx % pstVol->nSctsPerPg) == 0)
        {
            /* read one page */
            nLLDRe = _LLDRead(pstVol,
                              nPDev, 
                              nPbn, 
                              nStartOffset + (nSctIdx / pstVol->nSctsPerPg),
                              pstRsv, 
                              pstRsv->pMBuf,        /* main buffer pointer  */
                              &stSBuf,              /* spare buffer pointer */
                              BML_META_DATA, 
                              FALSE32,
                              FSR_LLD_FLAG_ECC_ON);

            nMajorRe = FSR_RETURN_MAJOR(nLLDRe);

            /* uncorrectable read error case */
            if ((nMajorRe != FSR_LLD_SUCCESS) &&
                (nMajorRe != FSR_LLD_PREV_2LV_READ_DISTURBANCE) &&
                (nMajorRe != FSR_LLD_PREV_1LV_READ_DISTURBANCE))  
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _LLDRead(nPDev: %d, nPbn:%d, nOffset:%d, BBM META) is failed\r\n"), 
                                               nPDev, nPbn, nStartOffset + (nSctIdx / pstVol->nSctsPerPg)));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nLLDRe));
                return nLLDRe;
            }

#if !defined(TINY_FSR)
            if (nMajorRe == FSR_LLD_PREV_2LV_READ_DISTURBANCE)
            {
                bReadDisturb = TRUE32;
            }
#endif

            pBufPtr = pstRsv->pMBuf;
        }

        /* PCH sector offset : 0 */
        if (nSctIdx == BML_PCH_SCT_OFF)
        {
            pstPCH = (BmlPoolCtlHdr *) pBufPtr;

            /* Change Byte-Order for Tool */
            FSR_BBM_CHANGE_BYTE_ORDER_BMLPOOLCTLHDR( pstPCH );

            /* UPCB */
            if (nType == BML_TYPE_UPCB)
            {
                pstRsvSh->nUPcbAge    = pstPCH->nAge;
                pstRsvSh->nREFSbn     = pstPCH->nREFPbn;
            }
            /* LPCB */
            else
            {
                pstRsvSh->nLPcbAge    = pstPCH->nAge;
            }

            /* update global PCB age */
            if (pstPCH->nGlobalAge > pstRsvSh->nGlobalPCBAge)
            {
                pstRsvSh->nGlobalPCBAge = pstPCH->nGlobalAge;
            }

            pBufPtr += sizeof(BmlPoolCtlHdr);
        }
        /* PIA sector offset : 1 */
        else if (nSctIdx == BML_PIA_SCT_OFF)
        {
            /* UPCB */
            if (nType == BML_TYPE_UPCB)
            {
                if (pstPI != NULL)
                {

                    pstPIExt = (FSRPIExt *)pstPI;

                    /* load PI extension */
                    FSR_OAM_MEMCPY(pstPIExt, &pBufPtr[0], sizeof(FSRPIExt));

                    /* Change Byte-Order for Tool */
                    FSR_BBM_CHANGE_BYTE_ORDER_FSRPIEXT( pstPIExt );
                }
            }
            /* LPCB */
            else
            {
                if (pstPI != NULL)
                {
                    pstPartI = (FSRPartI *)pstPI;

                    /* load PI */
                    FSR_OAM_MEMCPY(pstPartI, pBufPtr, sizeof(FSRPartI));

                    /* Change Byte-Order for Tool */
                    FSR_BBM_CHANGE_BYTE_ORDER_FSRPARTI( pstPartI );
                }              
            }

            /* move buffer pointer to next */
            pBufPtr += FSR_SECTOR_SIZE;
        }
        /* ERL sector offset : 2 */
        else if (nSctIdx == BML_ERL_SCT_OFF)
        {
            if (nType == BML_TYPE_UPCB)
            {
                /* UPCB has valid ERL list (LPCB has empty ERL) */
                FSR_OAM_MEMCPY(pstRsv->pstERL, pBufPtr, sizeof(BmlERL));

                /* Change Byte-Order for Tool */
                FSR_BBM_CHANGE_BYTE_ORDER_BMLERL( pstRsv->pstERL );
            }

            pBufPtr += sizeof(BmlERL);
        }
        /* BAB sector offset : 3 */
        else if (nSctIdx == BML_BAB_SCT_OFF)
        {
            /* UPCB */
            if (nType == BML_TYPE_UPCB)
            {            
                /* UPCB has bad block allocation bitmap */
                FSR_OAM_MEMCPY(pstRsv->pBABitMap, pBufPtr, nSizeOfBABitMap);
            }

            /* move buffer pointer to next */
            pBufPtr += FSR_SECTOR_SIZE;
        }
        /* BMS sector */
        else if ((nBMSSctOffset <= nSctIdx) && 
                 (nSctIdx < nBMSSctOffset + nNumOfBMS))
        {
            pstBMS = (BmlBMS *) pBufPtr;

            /* Change Byte-Order for Tool */
            FSR_BBM_CHANGE_BYTE_ORDER_BMLBMS( pstBMS );

            /* Load first BMS sector */
            bRet = _LoadBMS(pstVol, pstRsv, pstBMS);
            if (bRet == FALSE32)
            {
                /* if it is not found... */
                bLoaded = FALSE32;
                break;
            }

            /* move buffer pointer to next */
            pBufPtr += FSR_SECTOR_SIZE;
        }
        /* RCBS sector */
        else if ((nSctIdx >= ((BML_NUM_OF_META_PGS * pstVol->nSctsPerPg) - nNumOfRCB)) && 
                 (nSctIdx < (BML_NUM_OF_META_PGS * pstVol->nSctsPerPg)))
        {
            if (nType == BML_TYPE_UPCB)
            {
                pRCBSbn = (UINT16 *) pBufPtr;

                /* Change Byte-Order for Tool */
                FSR_BBM_CHANGE_BYTE_ORDER_RCB( pRCBSbn );

                for (nIdx = 0; nIdx < BML_RCBFS_PER_RCBS; nIdx++)
                {
                    if (pRCBSbn[nIdx] != 0xFFFF)
                    {
                        pstRsv->pstRCB[pstRsv->pstBMI->nNumOfRCBs++] = pRCBSbn[nIdx];
                    }
                    else
                    {
                        break;
                    }
                }
            }

            /* move buffer pointer to next */
            pBufPtr += FSR_SECTOR_SIZE;
        }
        /* unused sectors */
        else
        {
            /* skip reserved area (1 sector) */
            pBufPtr += FSR_SECTOR_SIZE;
        }
    }

    /* if BMS is not loaded yet */
    if (bLoaded == FALSE32)
    {
        if (nType == BML_TYPE_UPCB)
        {
            /* UPCB load fails */
            nRet = FSR_BML_LOAD_UBMS_FAILURE;
        }
        else
        {
            /* LPCB load fails */
            nRet = FSR_BML_LOAD_LBMS_FAILURE;
        }
    }
    /* BMS is loaded successfully */
    else
    {
#if !defined(TINY_FSR)
        /* 
         * if UPCB has read disturbance error, BBM discards meta-data in UPCB and
         * writes current meta-data into TPCB to avoid using UPCB.
         * Meta-data will program into TPCB even if ERL is empty.
         * (read disturbance error of LPCB is ignored)
         */
        if ((bReadDisturb == TRUE32) &&
            (nType == BML_TYPE_UPCB))
        {
            /* 
             * RSVR should be unlocked, because of running FSR_BBM_UpdateERL 
             * nRet is only used by error message, 
            */
            nRet = FSR_BBM_UnlockRsvr(pstVol,
                                      pstDev,
                                      nDieIdx);
            if (nRet != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR]   %s / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BBM:ERR]   FSR_BBM_UnlockRsvr(nPDev: %d, nDieIdx: %d)\r\n"),
                                                nPDev, nDieIdx));
            }

            /* solve read disturbance error of UPCB */
            nRet = FSR_BBM_UpdateERL(pstVol,
                                     pstDev,
                                     nDieIdx,
                                     nPbn,
                                     BML_FLAG_ERL_FORCED_PROGRAM);
        }
#endif /* TINY_FSR */
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));

    return nRet;
}

/*
 * @brief           This function loads BMS
 *
 * @param[in]      *pstRsv      : BmlReservoir structure pointer
 * @param[in]      *pstBMS      : BMS structure pointer
 *
 * @return          TRUE32
 * @return          FALSE32
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PRIVATE BOOL32
_LoadBMS(BmlVolCxt *pstVol,
         BmlReservoir  *pstRsv, 
         BmlBMS        *pstBMS)
{
    BmlBMI    *pstBMI;
    BmlBMF    *pSrcBMF;
    BmlBMF    *pDstBMF;
    UINT32     nBMFIdx;
    UINT32     nNumOfRsvrBlks;
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s()\r\n"), __FSR_FUNC__));

    pstBMI           = pstRsv->pstBMI;
    pSrcBMF          = (BmlBMF *) (pstBMS->stBMF);
    pDstBMF          = pstRsv->pstBMF;

    /* set number of Reservoir blocks in a die */
    nNumOfRsvrBlks = pstVol->nNumOfRsvrBlks >> pstVol->nSftDDP;

    do
    {
        /* construct Block Map Information using BMS */
        for (nBMFIdx = 0; nBMFIdx < BML_BMFS_PER_BMS; nBMFIdx++)
        {
            /* if Rbn is 0xFFFF, slot (nIdx2) is empty */
            if (pSrcBMF[nBMFIdx].nRbn == (UINT16) 0xFFFF && pSrcBMF[nBMFIdx].nSbn == (UINT16) 0xFFFF)
            {
                break;
            }

            /* multi plane device */
            if (pstVol->nNumOfPlane > 1)
            {
                if (pstBMI->nNumOfBMFs > (UINT16) (nNumOfRsvrBlks + (nNumOfRsvrBlks / 2)))
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] pstBMI->nNumOfBMFs(%d) >= number of blocks in reservoir + 50%\r\n"), pstBMI->nNumOfBMFs));
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] n1stSbnOfRsvr(%d) >= nLastSbnOfRsvr(%d)\r\n"), 
                                                   pstRsv->n1stSbnOfRsvr, pstRsv->nLastSbnOfRsvr));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FALSE32));
                    return FALSE32;
                }
            }
            else
            {
                if (pstBMI->nNumOfBMFs > (UINT16) (nNumOfRsvrBlks))
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] pstBMI->nNumOfBMFs(%d) >= number of blocks in reservoir\r\n"), pstBMI->nNumOfBMFs));
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] n1stSbnOfRsvr(%d) >= nLastSbnOfRsvr(%d)\r\n"), 
                                                   pstRsv->n1stSbnOfRsvr, pstRsv->nLastSbnOfRsvr));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FALSE32));
                    return FALSE32;
                }
            }

            /* save Sbn and Rbn */
            pDstBMF[pstBMI->nNumOfBMFs].nSbn = pSrcBMF[nBMFIdx].nSbn;
            pDstBMF[pstBMI->nNumOfBMFs].nRbn = pSrcBMF[nBMFIdx].nRbn;

            /* increase the number of BMFs */
            pstBMI->nNumOfBMFs++;
        }
    }
    while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, TRUE32));

    return TRUE32;
}
