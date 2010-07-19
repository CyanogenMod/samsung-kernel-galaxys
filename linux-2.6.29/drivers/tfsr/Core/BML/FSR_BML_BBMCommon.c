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
 * @file      FSR_BML_BBMCommon.c
 * @brief     This file contains the common routines for BBM module
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
#include "FSR_BML_BadBlkMgr.h"
#include "FSR_BML_BBMCommon.h"


/*****************************************************************************/
/* Function prototype                                                        */
/*****************************************************************************/

/**
 * @brief          this function calculate a size of reservoir
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 * @param[in]      *pstPart     : partition context pointer
 * @param[in]       bInitFormat : flag for initial format
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
_CalcRsvrSize(BmlVolCxt    *pstVol,
              BmlDevCxt    *pstDev,
              FSRPartI     *pstPart,
              BOOL32        bInitFormat)
{
    BmlReservoir   *pstRsv;
    FSRSpareBuf     stSBuf;
    LLDPIArg        stLLDPIArg;    
    INT32           nRet = FSR_BML_SUCCESS;
    INT32           nLLDRe;    
    INT32           nUPCBCmpRet;
    INT32           nLPCBCmpRet;
    UINT32          nDieIdx;    
    UINT32          nNumOfSLCBlksInDie;
    UINT32          nNumOfRsvrBlksInDie;
    UINT32          nNewNumOfSLCRsvrBlksInDie;
    UINT32          nNewNumOfMLCRsvrBlksInDie;
    UINT32          nByteRet;
    UINT32          nFirstBlk;
    UINT32          nBlkIdx;
    UINT32          nLastSLCPbnInDie;
    UINT32          nDZMask;
    BOOL32          bFound;
    BOOL32          bRet;
    BmlPoolCtlHdr  *pstPCH;
    
#if !defined(FSR_NBL2)
    UINT32          nLastSLCPbn;
    UINT32          nOldNumOfSLCRsvrBlksInDie;
#endif


    FSR_STACK_VAR;

    FSR_STACK_END; 

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(PDev: %d, bInitFormat: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo, bInitFormat));

    nNewNumOfSLCRsvrBlksInDie = 0;
    nNewNumOfMLCRsvrBlksInDie = 0;

    nNumOfSLCBlksInDie = 0;

    /* total number of Reservoir blocks in a die */
    nNumOfRsvrBlksInDie = pstVol->nNumOfRsvrBlks >> pstVol->nSftDDP;

    /* initial format time */
    if (bInitFormat == TRUE32)
    {
#if !defined(FSR_NBL2)
        /* sort partition info and get a pbn of last SLC block */
        nRet = _CheckPartInfo(pstVol, pstPart, &nLastSLCPbn);
        if (nRet != FSR_BML_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] _CheckPartInfo() fails!\r\n")));    
            FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));
            return nRet;
        }

        /* calculate Reservoir size of Flex-OneNAND */
        if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
        {
            /* 
             * Before calling FSR_BBM_Format() for initial format,
             * ratio of SLC and MLC should be calculated by partition info.
             * And number of blocks in Reservoir and ratio of SLC and MLC in reservoir
             * shoule be also calculated
             */

            /* number of SLC blocks in a die */
            nNumOfSLCBlksInDie = nLastSLCPbn + 1;

            /* SLC blocks:MLC blocks = 1:1 = SLC Reservoir blocks : MLC Reservoir blocks */
            nOldNumOfSLCRsvrBlksInDie = nNumOfRsvrBlksInDie / 2;            

            /*
             * recalculate number of blocks in Reservoir based on partition info
             * SLC(1/2 dev): SLC partition size (partition info) +  new SLC Reservoir + PCB
             * = old SLC reservoir: new SLC reservoir
             */
            nNewNumOfSLCRsvrBlksInDie = (nOldNumOfSLCRsvrBlksInDie  * (nNumOfSLCBlksInDie + BML_RSV_PCB_BLKS)) / 
                                        (pstVol->nNumOfBlksInDie / 2 - nOldNumOfSLCRsvrBlksInDie);


            /* 
             * if number of Reservoir for SLC is larger than total number of reservoir,
             * it should be adjusted
             */
            if (nNewNumOfSLCRsvrBlksInDie > nNumOfRsvrBlksInDie)
            {
                nNewNumOfSLCRsvrBlksInDie = nNumOfRsvrBlksInDie;
            }

            /* 
             * if newly calculated number of Reservoir blocks in SLC is larger than allowable value, 
             * the number should be adjusted to max allowable value
             */
            if (nNewNumOfSLCRsvrBlksInDie >= pstVol->nNumOfBlksInDie - nNumOfSLCBlksInDie - BML_RSV_META_BLKS)
            {
                nNewNumOfSLCRsvrBlksInDie = pstVol->nNumOfBlksInDie  - nNumOfSLCBlksInDie - BML_RSV_META_BLKS;
            }

            /* 
             * if attribute of blocks in all partitions is SLC, 
             * Reservoir should consists of SLC blocks (no MLC block)
             */
            if (nNumOfSLCBlksInDie >= pstVol->nNumOfBlksInDie - nNumOfRsvrBlksInDie - BML_RSV_META_BLKS)
            {
                nNewNumOfSLCRsvrBlksInDie = nNumOfRsvrBlksInDie;
            }

            /* except block 0 */
            if ((nLastSLCPbn >= pstVol->nNumOfPlane) && (nNewNumOfSLCRsvrBlksInDie < 1))
            {
                /* minimum number of SLC blocks in Reservoir for hybrid type */
                nNewNumOfSLCRsvrBlksInDie = 1;
            }

            /* remained area is MLC Reservoir blocks */
            nNewNumOfMLCRsvrBlksInDie = nNumOfRsvrBlksInDie - nNewNumOfSLCRsvrBlksInDie;

            /* 
             * if this device is not SLC only type, 
             * MLC Reservoir should exist at least one block 
             */
            if ((nNumOfSLCBlksInDie < pstVol->nNumOfBlksInDie - nNumOfRsvrBlksInDie - BML_RSV_META_BLKS) &&
                (nNewNumOfMLCRsvrBlksInDie < 1))                            
            {
                /* minimum number of MLC blocks in Reservoir for hybrid type */
                nNewNumOfMLCRsvrBlksInDie = 1;
                nNewNumOfSLCRsvrBlksInDie = nNumOfRsvrBlksInDie - nNewNumOfMLCRsvrBlksInDie;
            }
        }
#endif /* FSR_NBL2 */
    }
    /* mount */
    else
    {
        /* Flex-OneNAND */
        if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
        {
            for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
            {
                pstRsv = pstDev->pstDie[nDieIdx]->pstRsv;

                /* get # of SLC Blocks in a device using LLD_IOCtl*/
                nRet = pstVol->LLD_IOCtl(pstDev->nDevNo,
                                         FSR_LLD_IOCTL_PI_READ,
                                         (UINT8 *)&nDieIdx,
                                         sizeof(nDieIdx),
                                         (UINT8 *)&(stLLDPIArg),
                                         sizeof(stLLDPIArg),
                                         &nByteRet);

                if (nRet != FSR_LLD_SUCCESS)
                {
                   FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] LLD_IOCtl(nCode:FSR_LLD_IOCTL_PI_READ) fails!(Dev:%d, Die:%d)\r\n"), 
                                                  pstDev->nDevNo, nDieIdx));
                   FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));
                   return nRet;
                }

                /* 
                 * FBA in a die + die index * number of blocks in a die 
                 * ex. 1024 blocks in a die, 1 SLC block in a die
                 * die0: nLastSLCPbnInDie = 0, die1: nLastSLCPbnInDie = 1024
                 */
                nLastSLCPbnInDie = stLLDPIArg.nEndOfSLC;

                if ((nLastSLCPbnInDie < pstVol->nNumOfBlksInDie * nDieIdx) ||
                    (nLastSLCPbnInDie >= pstVol->nNumOfBlksInDie * (nDieIdx + 1)))
                {
                   nRet = FSR_BML_CRITICAL_ERROR;
                   FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Invalid SLC Pbn!(nPbn: %d, Die: %d, Number of blocks in a die: %d)\r\n"), 
                                                  nLastSLCPbnInDie, nDieIdx, pstVol->nNumOfBlksInDie));
                   FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));
                   return nRet;

                }

                /* set number of SLC blocks in a die */
                nNumOfSLCBlksInDie  = nLastSLCPbnInDie + 1 - pstVol->nNumOfBlksInDie * nDieIdx;
                pstDev->nNumOfSLCBlksInDie[nDieIdx] = nNumOfSLCBlksInDie;

                /* 
                 * When there is only one SLC block for BL, 
                 * this device is regarded as MLC only type 
                 */
                if (nLastSLCPbnInDie - (pstVol->nNumOfBlksInDie * nDieIdx) < pstVol->nNumOfPlane)
                {
                    /* number of Reservoir for MLC area */
                    pstRsv->nNumOfRsvrInSLC = 0;
                    pstRsv->nNumOfRsvrInMLC = nNumOfRsvrBlksInDie;  
                    continue;
                }

                /* SLC only Flex-OneNAND */
                if (nLastSLCPbnInDie >= (pstVol->nNumOfBlksInDie * (nDieIdx + 1)) - 1)
                {                    
                    /* number of Reservoir for SLC area */
                    pstRsv->nNumOfRsvrInSLC = nNumOfRsvrBlksInDie;
                    pstRsv->nNumOfRsvrInMLC = 0;  
                    continue;                
                }

                /*
                 *           Flex-OneNAND (SLC+MLC type)
                 *
                 *                              first block of MLC 
                 *                              V
                 *   --------------------------------------------
                 *   |    SLC              X    | MLC           |
                 *   --------------------------------------------   
                 *                   |<- Scan ->|
                 *
                 * scan blocks to find BBM meta block(X) from SLC reservoir
                 * to SLC last block 
                 * Max size of Reservoir = SLC only case
                 */

                /*
                 *   If number of SLC blocks in a die is larger than or 
                 *   equal to max size of SLC reservoir
                 *   --------------------------------------------
                 *   |SLC                |        MLC           |
                 *   --------------------------------------------  
                 */
                if (nNumOfSLCBlksInDie >= (nNumOfRsvrBlksInDie + BML_RSV_PCB_BLKS + pstVol->nNumOfPlane))
                {
                    /* nFirstBlk >= pstVol->nNumOfPlane */
                    nFirstBlk = (nNumOfSLCBlksInDie - (nNumOfRsvrBlksInDie + BML_RSV_PCB_BLKS)) +
                                pstVol->nNumOfBlksInDie * nDieIdx;
                }
                /*
                 *   If number of SLC blocks in a die is smaller than max size of SLC reservoir
                 *   --------------------------------------------
                 *   |SLC |                       MLC           |
                 *   --------------------------------------------  
                 */
                else
                {
                    /* scan from the next block of first block of the plane */
                    nFirstBlk = pstVol->nNumOfPlane + pstVol->nNumOfBlksInDie * nDieIdx;
                } 
                
                bFound = FALSE32;

                FSR_OAM_MEMCPY(&stSBuf, pstRsv->pSBuf, sizeof(FSRSpareBuf));
                if (stSBuf.nNumOfMetaExt !=0)
                {
                    stSBuf.nNumOfMetaExt = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
                }

                /* find BBM meta block in MLC area to calculate size of Reservoir */
                for (nBlkIdx = (nNumOfSLCBlksInDie + pstVol->nNumOfBlksInDie * nDieIdx) - 1; 
                     nBlkIdx >= nFirstBlk; 
                     nBlkIdx--)
                {
                    /* Get current debug zone mask */
                    nDZMask = FSR_DBG_GetDbgZoneMask();

                    /* 
                     * Unset all debug zone mask 
                     * to remove uncorrectable read error msg at open time 
                     */
                    FSR_DBG_UnsetAllDbgZoneMask();


                    /* read the first page of a block */
                    nLLDRe = _LLDRead(pstVol,
                                      pstDev->nDevNo,
                                      nBlkIdx,
                                      0,                    /* page offset */
                                      pstRsv,
                                      pstRsv->pMBuf,        /* main buffer pointer  */
                                      &stSBuf,              /* spare buffer pointer */ 
                                      BML_USER_DATA,
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

                    /* if this block is not a BBM meta block */
                    if (stSBuf.pstSpareBufBase->nBMLMetaBase0 != FSR_LLD_BBM_META_MARK)
                    {
                        /* skip and move to next block */
                        continue;
                    }
                    else
                    {
                        /* get number of MLC Reservoir from main area */
                        pstPCH = (BmlPoolCtlHdr *) pstRsv->pMBuf;

                        /* Change Byte-Order for Tool */
                        FSR_BBM_CHANGE_BYTE_ORDER_BMLPOOLCTLHDR( pstPCH );

                        nLPCBCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_LPCH_SIG, BML_MAX_PCH_SIG);
                        nUPCBCmpRet = FSR_OAM_MEMCMP(pstPCH->aSig, BML_UPCH_SIG, BML_MAX_PCH_SIG);

                        if ((nLPCBCmpRet == 0) || (nUPCBCmpRet == 0))
                        {
                            /* get data from main buffer */
                            pstRsv->nNumOfRsvrInMLC = pstPCH->nNumOfMLCRsvr;

                            /* set SLC reservoir blocks */
                            pstRsv->nNumOfRsvrInSLC = nNumOfRsvrBlksInDie - pstRsv->nNumOfRsvrInMLC;

                            /* 
                             * read next page to check whether page 0 has valid data or not 
                             * (return value is ignored)
                             */
                            _LLDRead(pstVol,
                                     pstDev->nDevNo,
                                     nBlkIdx,
                                     1,                    /* page offset */
                                     pstRsv,
                                     pstRsv->pMBuf,        /* main buffer pointer  */
                                     &stSBuf,              /* spare buffer pointer */ 
                                     BML_USER_DATA,
                                     FALSE32,
                                     FSR_LLD_FLAG_ECC_ON);

                            /* If data is written to page1, spare data should not be all 0xFF */
                            bRet = _CmpData(pstVol, (UINT8 *) &stSBuf, 0xFF, TRUE32);

                            /* if data is not all 0xFF, integrity of data in page0 can not be guaranteed */
                            if (bRet != TRUE32)
                            {
                                bFound = TRUE32;

                                /* first BBM meta block is found */
                                break;
                            }
                            /* power-failure may occur during program operation for page 0 */
                            else
                            {
                                continue;
                            }
                        }

                        /* skip and move to next block (TPCB has a PCH sector) */
                        continue;
                    }

                } /* end of for : nBlkIdx */

                if (bFound == FALSE32)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] No BBM meta block!(Dev:%d, Die:%d)\r\n"), 
                                                   pstDev->nDevNo, nDieIdx));
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Searched from Pbn #%d to Pbn #%d\r\n"), 
                                                   (nNumOfSLCBlksInDie + pstVol->nNumOfBlksInDie * nDieIdx) - 1, nFirstBlk));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_CRITICAL_ERROR));
                    return FSR_BML_CRITICAL_ERROR;
                }                

            } /* end of for : nDieIdx */

        } /* end of if : Flex-OneNAND */

    }

    /* When Flex-OneNAND is mounted, below codes are skipped */
    if ((bInitFormat != TRUE32) && (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND))
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_SUCCESS));
        return FSR_BML_SUCCESS;
    }    

    if (pstVol->nNANDType == FSR_LLD_MLC_NAND)
    {
        nNewNumOfSLCRsvrBlksInDie = 0;
        nNewNumOfMLCRsvrBlksInDie = nNumOfRsvrBlksInDie;
    }
    else if ((pstVol->nNANDType == FSR_LLD_SLC_ONENAND) ||
             (pstVol->nNANDType == FSR_LLD_SLC_NAND))
    {
        nNewNumOfSLCRsvrBlksInDie = nNumOfRsvrBlksInDie;
        nNewNumOfMLCRsvrBlksInDie = 0;
    }

    for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
    {
        pstRsv = pstDev->pstDie[nDieIdx]->pstRsv;

        /*
         * Set size of Reservoir for each die 
         * In this step, each Reservoir has identical configurations
         */
        pstRsv->nNumOfRsvrInSLC = nNewNumOfSLCRsvrBlksInDie;
        pstRsv->nNumOfRsvrInMLC = nNewNumOfMLCRsvrBlksInDie;

         /* SLC only : all usable blocks are SLC block */
        if ((pstRsv->nNumOfRsvrInMLC == 0) && (pstRsv->nNumOfRsvrInSLC == nNumOfRsvrBlksInDie))
        {
            pstDev->nNumOfSLCBlksInDie[nDieIdx] = pstVol->nNumOfBlksInDie;
        }
        /* MLC only : all usable blocks are MLC block except first block of die */
        else if ((pstRsv->nNumOfRsvrInSLC == 0) && (pstRsv->nNumOfRsvrInMLC == nNumOfRsvrBlksInDie))
        {
            /* Flex-OneNAND */
            if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
            {
                /* add number of SLC blocks in Reservoir to total numer of SLC blocks in a die */
                pstDev->nNumOfSLCBlksInDie[nDieIdx] = pstVol->nNumOfPlane;
            }
            /* MLC device */
            else
            {
                pstDev->nNumOfSLCBlksInDie[nDieIdx] = 0;
            } 
        } 
        /* hybrid type */
        else if ((pstRsv->nNumOfRsvrInSLC > 0) && (pstRsv->nNumOfRsvrInMLC > 0))
        {
            /* add number of SLC blocks in Reservoir to total numer of SLC blocks in a die */
            pstDev->nNumOfSLCBlksInDie[nDieIdx] = nNumOfSLCBlksInDie + pstRsv->nNumOfRsvrInSLC + BML_RSV_PCB_BLKS;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_SUCCESS));

    return FSR_BML_SUCCESS;
}

/**
 * @brief          this function sets the fields of volume context
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 *
 * @return          none
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC VOID
_SetVolCxt(BmlVolCxt   *pstVol,
           BmlDevCxt   *pstDev)
{
    BmlReservoir   *pstRsv;    
    UINT32          nNumOfUsableSLCBlksInDie;
    UINT32          nBlksInRsv;    
    UINT32          nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END; 

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(PDev: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo));

    /* total size of Reservoir for each die */
    nBlksInRsv = pstVol->nNumOfRsvrBlks >> pstVol->nSftDDP;

    /* SLC or OneNAND */
    if ((pstVol->nNANDType == FSR_LLD_SLC_NAND) ||
        (pstVol->nNANDType == FSR_LLD_SLC_ONENAND))
    {
        nNumOfUsableSLCBlksInDie = pstVol->nNumOfBlksInDie - nBlksInRsv - BML_RSV_META_BLKS;

        /* number of SLC units */
        pstVol->nNumOfSLCUnit = nNumOfUsableSLCBlksInDie >> pstVol->nSftNumOfPln;

        /* the first nVpn of MLC blocks */
        pstVol->n1stVpnOfMLC  =  (pstVol->nNumOfSLCUnit << (pstVol->nSftSLC + pstVol->nSftNumOfWays));

        pstVol->nLastVpn = pstVol->n1stVpnOfMLC - 1;        

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDev->nNumOfSLCBlksInDie[nDieIdx] = pstVol->nNumOfBlksInDie;
        }
    }
    /* MLC NAND */
    else if (pstVol->nNANDType == FSR_LLD_MLC_NAND)
    {
        /* number of SLC units */
        pstVol->nNumOfSLCUnit = 0;

        /* the first nVpn of MLC blocks  */
        pstVol->n1stVpnOfMLC  = 0;

        pstVol->nLastVpn = (((pstVol->nNumOfBlksInDie - nBlksInRsv - BML_RSV_META_BLKS) >> pstVol->nSftNumOfPln)
                           << (pstVol->nSftMLC + pstVol->nSftNumOfWays));

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDev->nNumOfSLCBlksInDie[nDieIdx] = 0;
        }
    }
    /* Flex-OneNAND */
    else
    {
        pstRsv = pstDev->pstDie[0]->pstRsv;

        /* MLC only */
        if (pstRsv->nNumOfRsvrInSLC == 0)
        {
            nNumOfUsableSLCBlksInDie = pstVol->nNumOfPlane;
        }
        /* SLC only */
        else if (pstRsv->nNumOfRsvrInMLC == 0)
        {
            /* number of SLC blocks in die except Reservoir blocks */
            nNumOfUsableSLCBlksInDie = pstVol->nNumOfBlksInDie - (nBlksInRsv + BML_RSV_META_BLKS);
        }
        /* hybrid type */
        else
        {
            nNumOfUsableSLCBlksInDie = pstRsv->n1stSbnOfRsvr;
        }

         /* number of SLC units */
        pstVol->nNumOfSLCUnit   =  nNumOfUsableSLCBlksInDie >> pstVol->nSftNumOfPln;

        /* the first nVpn of MLC blocks */
        pstVol->n1stVpnOfMLC    =  (pstVol->nNumOfSLCUnit << (pstVol->nSftSLC + pstVol->nSftNumOfWays));
        
        /* last Vpn of volume */
        pstVol->nLastVpn        =  pstVol->n1stVpnOfMLC + 
                                   (((pstVol->nNumOfBlksInDie - nNumOfUsableSLCBlksInDie - nBlksInRsv - BML_RSV_META_BLKS)
                                   >> pstVol->nSftNumOfPln) << (pstVol->nSftMLC + pstVol->nSftNumOfWays)) - 1;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s()\r\n"), __FSR_FUNC__));
}

/*
 * @brief           This function checks whether the given Pbn is allocated or not
 * 
 * @param[in]       pstRsv      : pointer of Reservoir structure
 * @param[in]       nPbn        : physical block number
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
PUBLIC BOOL32
_IsAllocRB(BmlReservoir *pstRsv, 
           UINT32        nPbn)
{
    INT32  nRBIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPbn: %d)\r\n"),
                                     __FSR_FUNC__, nPbn));

    /* get reserved block index */
    nRBIdx = nPbn - pstRsv->n1stSbnOfRsvr;
    FSR_ASSERT(nRBIdx >= 0);

    /* set the bit of Block Allocation BitMap as 1 */
    if ((pstRsv->pBABitMap[nRBIdx / 8] & (UINT8) (0x80 >> (nRBIdx % 8))) != 0)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, TRUE32));
        return TRUE32;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FALSE32));

    return FALSE32;
}

/*
 * @brief           This function reads a page using LLD function
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]       nPDev       : physical device number 
 * @param[in]       nPbn        : physical block number
 * @param[in]       nPgOffset   : page offset to be read
 * @param[in]       pstRsv      : Reservoir context pointer
 * @param[in]       nDataType   : type of data to be written
 * @param[in]       bRecovery   : recovery load or normal load
 * @param[in]       nFlag       : flag for read operation
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_READ_DISTURBANCE
 * @return          FSR_LLD_PREV_READ_ERROR
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
_LLDRead(BmlVolCxt     *pstVol,
         UINT32         nPDev, 
         UINT32         nPbn, 
         UINT32         nPgOffset, 
         BmlReservoir  *pstRsv,
         UINT8         *pMBuf,
         FSRSpareBuf   *pSBuf,
         UINT32         nDataType,
         BOOL32         bRecovery,
         UINT32         nFlag)
{
    INT32           nLLDRe; 
    UINT32          nReadCnt;
    UINT32          nLoadFlag; 

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPbn: %d, nPgOff: %d, nDataType: %d, bRecovery: %d, nFlag: 0x%x)\r\n"),
                                     __FSR_FUNC__, nPbn, nPgOffset, nDataType, bRecovery, nFlag));

    /* When meta data is written to MLC Reservoir (MLC only type)*/
    if ((nDataType == BML_META_DATA) && 
        (pstRsv->nRsvrType == BML_MLC_RESERVOIR))
    {
        /* convert SLC page offset to LSB page offset in MLC */       
        nPgOffset = pstVol->pLSBPgMap[nPgOffset];
    }

    nReadCnt = 0;

    do
    {
        /* 
         * load the data to recover corrupted data in LSB page 
         * (recovery load can be used only for Flex-OneNAND device)
         */
        if (bRecovery == TRUE32)
        {
            nLoadFlag = FSR_LLD_FLAG_LSB_RECOVERY_LOAD;
        }
        /* use normal load operation */
        else
        {
            nLoadFlag = FSR_LLD_FLAG_1X_LOAD;
        }

        /* set the load and transfer flag with ECC OFF */
        if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_OFF)
        {
            nLoadFlag |= FSR_LLD_FLAG_ECC_OFF;
        }

        /* default flags for transfer */
        nFlag |= (FSR_LLD_FLAG_NO_LOADCMD | FSR_LLD_FLAG_TRANSFER | FSR_LLD_FLAG_USE_SPAREBUF);

        /* 
         * Load from NAND cell and ignore return value 
         */
        pstVol->LLD_Read(nPDev,
                         nPbn,
                         nPgOffset,
                         pMBuf,
                         pSBuf,
                         nLoadFlag);

        /* transfer data */
        nLLDRe = pstVol->LLD_Read(nPDev,
                                  nPbn,
                                  nPgOffset,
                                  pMBuf,
                                  pSBuf,
                                  nFlag);

        /* 
         * If read operation is done successfully, process next step 
         * If not, retry
         */
        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_READ_ERROR)
        {
            break;
        }

    } while (++nReadCnt < BML_MAX_UNCORRECTABLE_ERR);

    if (nReadCnt >= BML_MAX_UNCORRECTABLE_ERR)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Uncorrectable read error occurs (nPDev:%d, nPbn:%d, nOffset:%d)\r\n"), nPDev, nPbn, nPgOffset));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}

/*
 * @brief           This function compares nCmpData with pSrc array
 *
 * @param[in]      *pSrc        : source array pointer
 * @param[in]       nCmpData    : data to be compared with source 
 * @param[in]       bSpare      : Is spare buffer or not
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
PUBLIC BOOL32
_CmpData(BmlVolCxt *pstVol,
         UINT8     *pSrc, 
         UINT8      nCmpData,
         BOOL32     bSpare)
{
    UINT32          nIdx;
    UINT32          n32Value;
    UINT32          nExtIdx;
    UINT32         *p32Src;
    FSRSpareBuf    *pSBuf;

    FSR_STACK_VAR;

    FSR_STACK_END;

    n32Value = nCmpData | (nCmpData << 8) | (nCmpData << 16) | (nCmpData << 24);

    /* compare spare area */
    if (bSpare == TRUE32)
    {
        pSBuf = (FSRSpareBuf *) pSrc;

        p32Src = (UINT32 *) pSBuf->pstSpareBufBase;

        /* check base area */
        for (nIdx = 0; nIdx < FSR_SPARE_BUF_BASE_SIZE / sizeof(UINT32); nIdx++)
        {
            if (p32Src[nIdx] != n32Value)
            {
                return FALSE32;
            }
        }

        /* check extension area */
        for (nExtIdx = 0; nExtIdx < pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT; nExtIdx++)
        {
            p32Src = (UINT32 *) (pSBuf->pstSTLMetaExt + nExtIdx);

            for (nIdx = 0; nIdx < FSR_SPARE_BUF_EXT_SIZE / sizeof(UINT32); nIdx++)
            {
                if (p32Src[nIdx] != n32Value)
                {
                    return FALSE32;
                }
            }
        }
    }
    /* compare main area */
    else
    {
        p32Src = (UINT32 *)pSrc;

        /* check main buffer */
        for (nIdx = 0; nIdx < pstVol->nSizeOfPage / sizeof(UINT32); nIdx++)
        {
            if (p32Src[nIdx] != n32Value)
            {
                return FALSE32;
            }
        }
    }

    return TRUE32;
}

/*
 * @brief           This function allocate memory dynamically and initialize it
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstDev      : device context pointer
 *
 * @return          none
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC VOID 
_InitBBM(BmlVolCxt *pstVol,
         BmlDevCxt *pstDev)
{
    BmlReservoir   *pstRsv;
    BmlReservoirSh *pstRsvSh;
    UINT32          nDieIdx;
    UINT32          nNumOfRsvrBlks;
    UINT32          n1stSbnOfRsvr;
    UINT32          nLastSbnOfRsvr;
    UINT32          nNumOfBMFs;
    UINT32          nSizeOfMaxRsvr;
    UINT32          nSizeOfBABitMap;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s(nPDev: %d)\r\n"),
                                     __FSR_FUNC__, pstDev->nDevNo));

    /* DDP has reservoirs for each die */
    for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
    {
        pstRsv      = pstDev->pstDie[nDieIdx]->pstRsv;
        pstRsvSh    = pstDev->pstDie[nDieIdx]->pstRsvSh;

        if (pstRsv != NULL)
        {
            pstRsvSh->nUPCBSbn            = 0;
            pstRsvSh->nLPCBSbn            = 0;
            pstRsvSh->nTPCBSbn            = 0;
            pstRsvSh->nREFSbn             = 0;

            pstRsvSh->nGlobalPCBAge       = 0;
            pstRsvSh->nLPcbAge            = 0;
            pstRsvSh->nUPcbAge            = 0;

            pstRsvSh->nNextUPCBPgOff      = 0;
            pstRsvSh->nNextLPCBPgOff      = 0;
            pstRsvSh->nNextREFPgOff       = 0;

            pstRsvSh->nFirstUpdateUPCB       = 0;
            pstRsvSh->nFirstUpdateLPCB       = 0;

            pstRsvSh->nERLCntInRAM        = 0;

            pstRsv->pstBMI->nNumOfBMFs    = 0;
            pstRsv->pstBMI->nNumOfRCBs    = 0;

            pstRsvSh->bKeepLPCB         = FALSE32;
            pstRsvSh->bKeepUPCB         = FALSE32;

            /* set number of Reservoir blocks in a die */
            nNumOfRsvrBlks = pstVol->nNumOfRsvrBlks >> pstVol->nSftDDP;

            nNumOfBMFs = nNumOfRsvrBlks;

            /* multi plane device */
            if (pstVol->nNumOfPlane > 1)
            {
                /* Clear all Reservoir Candidate block Field. 0xFFFF means novmapping information */
                FSR_OAM_MEMSET(pstRsv->pstRCB, 0xFF, ((nNumOfRsvrBlks / 2) * sizeof(UINT16)));

                nNumOfBMFs += (nNumOfRsvrBlks / 2);
            }

            /* Clear all Block Map Field. 0xFFFF means no mapping information */
            FSR_OAM_MEMSET(pstRsv->pstBMF, 0xFF, nNumOfBMFs * sizeof(BmlBMF));

            
            /* 
             * max number of BMF entries = 3/2 * max number of reservoir blocks
             *                           = nNumOfBMS * BML_BMFS_PER_BMS
             * 
             * max number of reservoir blocks = 2/3 *  max number of BMF entries 
             */

            /* max number of reservoir blocks for 2KB page device */
            if (pstVol->nSctsPerPg == BML_2KB_PG)
            {
                nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_2KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
            }
            /* max number of reservoir blocks for 4KB page device */
            else
            {
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

            /* initialize pstRsv->pBABitMap by 0x00 */
            FSR_OAM_MEMSET(pstRsv->pBABitMap, 0x00, nSizeOfBABitMap);

            /* initialize Erase Refresh List by 0xFF */
            FSR_OAM_MEMSET(pstRsv->pstERL, 0xFF, sizeof(BmlERL));

            pstRsv->pstERL->nCnt = 0;

            /* SLC_RESERVOIR, BML_MLC_RESERVOIR, HYBRID_Reservoir (SLC_Reservoir | BML_MLC_RESERVOIR) */
            pstRsv->nRsvrType = 0;

            /* Check number of SLC blocks except Block #0 of Flex-OneNAND */
            if (pstDev->nNumOfSLCBlksInDie[nDieIdx] > pstVol->nNumOfPlane)
            {
                /* SLC only device and Hybrid type Flex-OneNAND */
                pstRsv->nRsvrType |= BML_SLC_RESERVOIR;
                pstRsv->n1stSbnOfMLC = 0xFFFFFFFF;
            }

            /* Check number of MLC blocks */
            if (pstVol->nNumOfBlksInDie - pstDev->nNumOfSLCBlksInDie[nDieIdx] > 0)
            {
                /* MLC only device and Hybrid type Flex-OneNAND */
                pstRsv->nRsvrType |= BML_MLC_RESERVOIR;

                /* number of SLC block = 0 or x(Flex-OneNAND) */
                pstRsv->n1stSbnOfMLC = pstDev->nNumOfSLCBlksInDie[nDieIdx];
            }

            /* SLC+MLC type Flex-OneNAND , HYBRID_Reservoir */
            if (pstRsv->nRsvrType == BML_HYBRID_RESERVOIR)
            {
                /* pstVol->nNumOfSLCBlksInDie is already set in FSR_BBM_CalcSLCBoundary() */

                /* Calculate first SBN of MLC in a die */
                pstRsv->n1stSbnOfMLC = pstDev->nNumOfSLCBlksInDie[nDieIdx] + pstVol->nNumOfBlksInDie * nDieIdx;

                /* Set range of Reservoir area */
                n1stSbnOfRsvr  = pstRsv->n1stSbnOfMLC - pstRsv->nNumOfRsvrInSLC - BML_RSV_PCB_BLKS;
                nLastSbnOfRsvr = pstRsv->n1stSbnOfMLC + pstRsv->nNumOfRsvrInMLC + BML_RSV_REF_BLKS - 1; 

            }
            /* SLC only or  MLC only */
            else
            {
                /* Set range of Reservoir area */
                nLastSbnOfRsvr = pstVol->nNumOfBlksInDie * (nDieIdx + 1) - 1;
                n1stSbnOfRsvr  = nLastSbnOfRsvr + 1 - nNumOfRsvrBlks - BML_RSV_META_BLKS;

                /* SLC only */
                if (pstRsv->nRsvrType == BML_SLC_RESERVOIR)
                {
                    pstDev->nNumOfSLCBlksInDie[nDieIdx] = pstVol->nNumOfBlksInDie;
                }
                /* MLC only */
                else
                {
                    /* Flex-OneNAND */
                    if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
                    {
                        pstDev->nNumOfSLCBlksInDie[nDieIdx] = pstVol->nNumOfPlane;
                    }
                    /* MLC device */
                    else
                    {
                        pstDev->nNumOfSLCBlksInDie[nDieIdx] = 0;
                    }                    
                }
            }

            /* Set first block and last block of Reservoir */
            pstRsv->n1stSbnOfRsvr  = n1stSbnOfRsvr;
            pstRsv->nLastSbnOfRsvr = nLastSbnOfRsvr;
            
            /* initialize main buffer */ 
            FSR_OAM_MEMSET(pstRsv->pMBuf, 0xFF, pstVol->nSizeOfPage);

        } /* end of if */
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s()\r\n"), __FSR_FUNC__));
}

/*
 * @brief           _LookUpPartAttr
 * @n               This function looks up an attribute of partition
 *
 * @param[in]      *pstVol      : BmlVolCxt structure pointer
 * @param[in]       nDieIdx     : Die index
 * @param[in]       nSbn        : semi physical block number
 *
 * @return          FSR_LLD_BLK_STAT_LOCKED 
 * @return          FSR_LLD_BLK_STAT_UNLOCKED
 * @return          FSR_LLD_BLK_STAT_LOCKED_TIGHT
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC UINT16
_LookUpPartAttr(BmlVolCxt   *pstVol,
                UINT32       nDieIdx,
                UINT16       nSbn)
{
    FSRPartI       *pstPartI;
    FSRPartEntry   *pstPEntry;
    UINT32          n1stSbn;
    UINT32          nPIdx;
    UINT32          nNumOfBlks;
    UINT16          nLockState = FSR_LLD_BLK_STAT_UNLOCKED;

    FSR_STACK_VAR;

    FSR_STACK_END;

    pstPartI  = pstVol->pstPartI;
    pstPEntry = pstPartI->stPEntry;
    
    for (nPIdx = 0; nPIdx < pstPartI->nNumOfPartEntry; nPIdx++)
    {
        n1stSbn    = pstPEntry[nPIdx].n1stVun * pstVol->nNumOfPlane + nDieIdx * pstVol->nNumOfBlksInDie;
        nNumOfBlks = pstPEntry[nPIdx].nNumOfUnits * pstVol->nNumOfPlane;

        if ((n1stSbn <= nSbn) && (nSbn < n1stSbn + nNumOfBlks))
        {
            /* RO or RW */
            if ((pstPEntry[nPIdx].nAttr & FSR_BML_PI_ATTR_RO) || 
                (pstPEntry[nPIdx].nAttr & FSR_BML_PI_ATTR_RW)) 
            {        
                nLockState = FSR_LLD_BLK_STAT_UNLOCKED;
                break;
            }

            if (pstPEntry[nPIdx].nAttr & FSR_BML_PI_ATTR_LOCKTIGHTEN)
            {
                nLockState = FSR_LLD_BLK_STAT_LOCKED_TIGHT;
                break;
            }

            if (pstPEntry[nPIdx].nAttr & FSR_BML_PI_ATTR_LOCK)
            {
                nLockState = FSR_LLD_BLK_STAT_LOCKED;
                break;
            }
        }
    }

    return nLockState;
}

/*
 * @brief           This function sorts by ascending power of pstBMI->pstBMF[x].nSbn
 *
 * @param[in]      *pstBMI      : BMI structure pointer
 *
 * @return          none
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC VOID
_SortBMI(BmlReservoir  *pstRsv)
{
    UINT32  nIdx1;
    UINT32  nIdx2;
    UINT16  nMinSbn;
    BmlBMF  stBmf;

    FSR_STACK_VAR;

    FSR_STACK_END;

    /* Sorts by ascending power of pstBMI->pstBMF[x].nSbn */

    for (nIdx1 = 0; nIdx1 < pstRsv->pstBMI->nNumOfBMFs; nIdx1++)
    {
        nMinSbn = pstRsv->pstBMF[nIdx1].nSbn;

        for (nIdx2 = nIdx1 + 1; nIdx2 < pstRsv->pstBMI->nNumOfBMFs; nIdx2++)
        {
            if (nMinSbn > pstRsv->pstBMF[nIdx2].nSbn)
            {
                nMinSbn               = pstRsv->pstBMF[nIdx2].nSbn;

                stBmf                 = pstRsv->pstBMF[nIdx1];
                pstRsv->pstBMF[nIdx1] = pstRsv->pstBMF[nIdx2];
                pstRsv->pstBMF[nIdx2] = stBmf;
            }
        }
    }
}

/*
 * @brief           This function sorts by ascending power of pstBMI->pstBMF[x].nSbn
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstRsv      : Reservoir structure pointer
 *
 * @return          none
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC VOID
_ReconstructBUMap(BmlVolCxt    *pstVol,
                  BmlReservoir *pstRsv,
                  UINT32        nDieIdx)
{
    UINT32 nIdx;
    UINT32 nBUIdx;
    UINT32 nMallocSize;
    UINT32 nNumOfBlks;

    FSR_STACK_VAR;

    FSR_STACK_END;

    nNumOfBlks = pstVol->nNumOfBlksInDie;

    if (nNumOfBlks > BML_BLKS_PER_BADUNIT)
    {
        nMallocSize = nNumOfBlks / BML_BLKS_PER_BADUNIT;
    }
    else
    {
        nMallocSize = 1;
    }

    FSR_OAM_MEMSET(pstRsv->pBUMap, 0x00, sizeof(BmlBadUnit) * nMallocSize);

    /* pstBMI->pstBMF[].nSbn should be sorted by ascending power */
    for (nIdx = 0; nIdx < pstRsv->pstBMI->nNumOfBMFs; nIdx++)
    {
        nBUIdx = (pstRsv->pstBMF[nIdx].nSbn - pstVol->nNumOfBlksInDie * nDieIdx) / BML_BLKS_PER_BADUNIT;

        if (pstRsv->pBUMap[nBUIdx].nNumOfBMFs == 0)
            pstRsv->pBUMap[nBUIdx].n1stBMFIdx = (UINT16) nIdx;

        pstRsv->pBUMap[nBUIdx].nNumOfBMFs++;
    }
}

/*
 * @brief           This function translates physical block number to semi-physical block number
 *
 * @param[in]      *pstRsv      : Reservoir structure pointer
 * @param[in]       nPbn        : physical block number
 * @param[in]      *pstBMI      : pointer of BMI structure that current BMI is stored
 *
 * @return          semi physical block number 
 *
 * @author          MinYoung Kim
 * @version         1.0.0
 * @remark          none
 *
 * @since           since v1.0.0
 * @exception       none
 *
 */
PUBLIC UINT32
_TransPbn2Sbn(BmlReservoir *pstRsv,
              UINT32        nPbn,
              BmlBMI       *pstBMI)
{
    UINT32  nSbn;
    UINT32  nPrevSbn;
    UINT32  nIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    nSbn = nPbn;
   
    do
    {
        nPrevSbn = nSbn;

        for (nIdx = 0; nIdx < pstBMI->nNumOfBMFs; nIdx++)
        {
            if (nSbn == pstRsv->pstBMF[nIdx].nRbn)
            {
                nSbn = pstRsv->pstBMF[nIdx].nSbn;
                break;
            }
        } 

    /* 
     * loop until the first replaced sbn 
     * Ex. A->B->C nPbn: C, nSbn: A should be returned
     */
    } while (nSbn != nPrevSbn);  

    /* If the sbn has replaced */
    if (nSbn != nPbn)
    {
        /* SLC + MLC (hybrid type) */
        if (pstRsv->nRsvrType == BML_HYBRID_RESERVOIR)
        {
            if ((nSbn < pstRsv->n1stSbnOfRsvr) ||
                (nSbn > pstRsv->nLastSbnOfRsvr))
            {
                /* Sbn is in the user area */
                return nSbn;
            }
        }
        else
        {
             if (nSbn < pstRsv->n1stSbnOfRsvr)
             {
                /* Sbn is in the user area */
                return nSbn;
             }
        }

        /* If nSbn is in the reservoir area, it returns the nPbn */
        nSbn = nPbn;
    }

    return nSbn;
}

#if !defined(FSR_NBL2)
/*
 * @brief           This function sort partition entry by ascending order of n1stVun
 * @n               and calculate last pbn of SLC block for Flex-OneNAND
 *
 * @param[in]      *pstVol      : volume context pointer
 * @param[in]      *pstPartI    : partition info structure pointer
 * @param[out]     *pnLastSLCPbn: pbn of last SLC block
 *
 * @return          FSR_BML_SUCCESS
 * @return          FSR_BML_INVALID_PARTITION | minor return
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
_CheckPartInfo(BmlVolCxt *pstVol, 
               FSRPartI  *pstPartI, 
               UINT32    *pnLastSLCPbn)
{
    FSRPartEntry   *pstPEntry;
    FSRPartEntry    stTmpPEntry;
    UINT32          nCnt;
    UINT32          nIdx;
    UINT32          nLastSLCPartIdx;
    UINT32          nFirstMLCVun;
    UINT32          nFirstMLCPbn;
    UINT32          nLastUnit;
    INT32           nRet            = FSR_BML_SUCCESS;
    INT32           nPEIdx;
    BOOL32          bFoundSLCPart;
    BOOL32          bFoundMLCPart;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:IN ] ++%s()\r\n"), __FSR_FUNC__));

    pstPEntry = pstPartI->stPEntry;
    
    if (pstPartI->nNumOfPartEntry > 1)
    {
        /* sort partition info by ascending order of n1stVun */
        for (nCnt = 0; nCnt < pstPartI->nNumOfPartEntry; nCnt++)
        {       
            for (nPEIdx = pstPartI->nNumOfPartEntry - 2; nPEIdx > (INT32)nCnt - 1; --nPEIdx)
            {
                if (pstPEntry[nPEIdx + 1].n1stVun < pstPEntry[nPEIdx].n1stVun)
                {
                    /* swap partition entry */
                    FSR_OAM_MEMCPY(&stTmpPEntry, &pstPEntry[nPEIdx], sizeof(FSRPartEntry));
                    FSR_OAM_MEMCPY(&pstPEntry[nPEIdx], &pstPEntry[nPEIdx + 1], sizeof(FSRPartEntry));
                    FSR_OAM_MEMCPY(&pstPEntry[nPEIdx + 1], &stTmpPEntry, sizeof(FSRPartEntry));
                }
            }
        }

        /* check partition boundary */
        for (nCnt = 0; nCnt < pstPartI->nNumOfPartEntry - 1; nCnt++)
        {
            /* All usable space is used. If not, returns error */
            if ((pstPEntry[nCnt].n1stVun + pstPEntry[nCnt].nNumOfUnits) < pstPEntry[nCnt + 1].n1stVun)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Unused space between partition(%d) and partition(%d)\r\n"), 
                                               pstPEntry[nCnt].nID, pstPEntry[nCnt + 1].nID));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PARAM));
                return FSR_BML_INVALID_PARTITION | FSR_BML_UNUSED_AREA;
            }

            /* Partition can not be overlapped. If not, returns error */
            if ((pstPEntry[nCnt].n1stVun + pstPEntry[nCnt].nNumOfUnits) > pstPEntry[nCnt + 1].n1stVun)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] partition boundary is overlapped (ID %d & %d)\r\n"), 
                                               pstPEntry[nCnt].nID, pstPEntry[nCnt + 1].nID));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PARAM));
                return FSR_BML_INVALID_PARTITION | FSR_BML_OVERLAPPED_PARTITION;
            }
        }
    }

    nLastUnit = pstPEntry[pstPartI->nNumOfPartEntry - 1].n1stVun + 
                pstPEntry[pstPartI->nNumOfPartEntry - 1].nNumOfUnits - 1;

    if (nLastUnit > pstVol->nLastUnit)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Number of used unit(%d) is larger than number of total usable unit(%d)\r\n"), 
                                       nLastUnit, pstVol->nLastUnit));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARTITION | FSR_BML_UNSUPPORTED_UNITS;
    }

    if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
    {

        /* 
         * All usable blocks should be used to calculate ratio of Reservoir corretly
         * If some blocks are remained as unused, BBM returns error (Flex-OneNAND only)
         */
        if (nLastUnit < pstVol->nLastUnit)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] All usable units should be used (Usable:%d, Used:%d)\r\n"), 
                                           pstVol->nLastUnit, nLastUnit));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, FSR_BML_INVALID_PARAM));
            return FSR_BML_INVALID_PARTITION | FSR_BML_UNUSED_AREA;
        }

        /* SLC partition is not found yet */
        nLastSLCPartIdx = 0xFFFFFFFF;
        bFoundSLCPart   = FALSE32;

        /* MLC partition is not found yet */
        bFoundMLCPart   = FALSE32;

        for (nIdx = 0; nIdx < pstPartI->nNumOfPartEntry; nIdx++)
        {   
            /* SLC partition */
            if (pstPEntry[nIdx].nAttr & FSR_BML_PI_ATTR_SLC)
            {
                /* SLC partition is found */
                bFoundSLCPart = TRUE32;

                /* update last SLC partition index */
                if (bFoundMLCPart == FALSE32)
                {
                    nLastSLCPartIdx = nIdx;
                }
                /* error : MLC partition exists ahead of SLC partition */
                else
                {
                    nRet = FSR_BML_INVALID_PARAM;
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] MLC partition exists ahead of SLC partition (ID:%d)\r\n"), pstPEntry[nIdx].nID));
                    break;
                }
            }
            /* MLC partition */
            else if (pstPEntry[nIdx].nAttr & FSR_BML_PI_ATTR_MLC)
            {
                /* MLC partition is found */
                bFoundMLCPart = TRUE32;
            }
            /* error : other case */
            else
            {
                nRet = FSR_BML_INVALID_PARAM;
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[BBM:ERR] Invalid partition attribute (ID:%d, nAttr: 0x%x)\r\n"), 
                                               pstPEntry[nIdx].nID, pstPEntry[nIdx].nAttr));
                break;
            }
        }

        if (nRet == FSR_BML_SUCCESS) 
        {
            if (bFoundSLCPart == TRUE32)
            {
                /* Calculate pbn of last SLC block */
                nFirstMLCVun  = pstPEntry[nLastSLCPartIdx].n1stVun + pstPEntry[nLastSLCPartIdx].nNumOfUnits;

                /* get first MLC pbn in die0 */
                nFirstMLCPbn  = nFirstMLCVun * pstVol->nNumOfPlane;
                *pnLastSLCPbn = nFirstMLCPbn - 1;
            }
            /* MLC only Flex-OneNAND (except block0(SLC)) */
            else
            {
                *pnLastSLCPbn = 0;
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_BBM, (TEXT("[BBM:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nRet));
    
    return nRet;
}
#endif /* FSR_NBL2 */

/*
 * Exported Functions and Macros for Changing Byte-Order(Big-Endian <-> Little-Endian)
 */
#if defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA)

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of BmlPoolCtlHdr structure
 *
 * @param[in]       *pstPCH     : BmlPoolCtlHdr structure pointer
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderBmlPoolCtlHdr( BmlPoolCtlHdr *pstPCH )
{
    FSR_ASSERT( pstPCH != NULL );

    ENDIAN_SWAP16( pstPCH->nREFPbn );
    ENDIAN_SWAP16( pstPCH->nTPCBPbn );
    ENDIAN_SWAP16( pstPCH->nNumOfMLCRsvr );
    ENDIAN_SWAP16( pstPCH->nRsv );
    ENDIAN_SWAP32( pstPCH->nAge );
    ENDIAN_SWAP32( pstPCH->nGlobalAge );

    ENDIAN_SWAP32( pstPCH->stDevInfo.nNumOfPlane );
    ENDIAN_SWAP32( pstPCH->stDevInfo.nNumOfDieInDev );
    ENDIAN_SWAP32( pstPCH->stDevInfo.nNumOfBlksInDie );
    ENDIAN_SWAP32( pstPCH->stDevInfo.nNumOfBlksInDev );
}

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of FSRPIExt structure
 *
 * @param[in]       *pstPIExt   : FSRPIExt structure pointer
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderFSRPIExt( FSRPIExt *pstPIExt )
{
    FSR_ASSERT( pstPIExt != NULL );

    ENDIAN_SWAP32( pstPIExt->nID );
    ENDIAN_SWAP32( pstPIExt->nSizeOfData );
}

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of FSRPartI structure
 *
 * @param[in]       *pstPartI   : FSRPartI structure pointer
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderFSRPartI( FSRPartI *pstPartI )
{
    UINT32      nIdx;

    FSR_ASSERT( pstPartI != NULL );

    ENDIAN_SWAP32( pstPartI->nVer );
    ENDIAN_SWAP32( pstPartI->nNumOfPartEntry );

    for ( nIdx = 0; nIdx < FSR_BML_MAX_PARTENTRY; nIdx ++ )
    {
        ENDIAN_SWAP16( pstPartI->stPEntry[ nIdx ].nID );
        ENDIAN_SWAP16( pstPartI->stPEntry[ nIdx ].nAttr );
        ENDIAN_SWAP16( pstPartI->stPEntry[ nIdx ].n1stVun );
        ENDIAN_SWAP16( pstPartI->stPEntry[ nIdx ].nNumOfUnits );
        ENDIAN_SWAP32( pstPartI->stPEntry[ nIdx ].nLoadAddr );
        ENDIAN_SWAP32( pstPartI->stPEntry[ nIdx ].nReserved );
    }
}

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of BmlERL structure
 *
 * @param[in]       *pstERL     : BmlERL structure pointer
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderBmlERL( BmlERL *pstERL )
{
    UINT32  nIdx;

    FSR_ASSERT( pstERL != NULL );

    ENDIAN_SWAP16( pstERL->nCnt );
    ENDIAN_SWAP16( pstERL->nRsv );

    for ( nIdx = 0; nIdx < BML_MAX_ERL_ITEM; nIdx ++ )
    {
        ENDIAN_SWAP16( pstERL->astERBlkInfo[ nIdx ].nSbn );
        ENDIAN_SWAP16( pstERL->astERBlkInfo[ nIdx ].nProgInfo );
    }
}

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of BmlBMS structure
 *
 * @param[in]       *pstBMS     : BmlBMS structure pointer
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderBmlBMS( BmlBMS *pstBMS )
{
    UINT32  nIdx;

    FSR_ASSERT( pstBMS != NULL );

    ENDIAN_SWAP16( pstBMS->nInf );
    ENDIAN_SWAP16( pstBMS->nRsv );

    for ( nIdx = 0; nIdx < BML_BMFS_PER_BMS; nIdx ++ )
    {
        ENDIAN_SWAP16( pstBMS->stBMF[ nIdx ].nSbn );
        ENDIAN_SWAP16( pstBMS->stBMF[ nIdx ].nRbn );
    }
}

/*
 * @brief           This function changes byte order(Big Endian <-> Little Endian) of RCBs Array
 *
 * @param[in]       *pnRCB      : RCBs Array
 *
 * @return          none
 *
 * @author          Byungyong Ahn
 * @version         1.2.1
 *
 */
VOID FSR_BML_ChangeByteOrderRCB( UINT16 *pnRCB )
{
    UINT32  nIdx;

    FSR_ASSERT( pnRCB != NULL );

    for ( nIdx = 0; nIdx < BML_RCBFS_PER_RCBS; nIdx ++, pnRCB ++ )
    {
        ENDIAN_SWAP16( *pnRCB );
    }
}
#endif  /* defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) */
