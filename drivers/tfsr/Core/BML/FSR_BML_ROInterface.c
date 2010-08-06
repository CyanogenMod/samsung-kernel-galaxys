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
 *  @file       FSR_BML_ROInterface.c
 *  @brief      This file consists of FSR_BML functions for RO operation.
 *  @author     SuRuyn Lee
 *  @date       15-JAN-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  15-JAN-2007 [SuRyun Lee] : first writing
 *  @n  04-JUN-2007 [SuRyun Lee] : seperate original FSR_BML_Interfsce file
 *
 */

/**
 *  @brief  Header file inclusions
 */
#define  FSR_NO_INCLUDE_STL_HEADER
#include "FSR.h"

#include "FSR_BML_Config.h"
#include "FSR_BML_Types.h"
#include "FSR_BML_BIFCommon.h"
#include "FSR_BML_BBMCommon.h"
#include "FSR_BML_NonBlkMgr.h"
#include "FSR_BML_BBMMount.h"
#include "FSR_BML_BadBlkMgr.h"


/**
 *  @brief      Static function prototypes
 */

PRIVATE INT32   _ProtectLockedArea  (UINT32     nVol,
                                     FSRPartI  *pstPartI);

#if !(defined(TINY_FSR) || defined(FSR_NBL2))
PRIVATE VOID    _PrintBMI       (UINT32      nVol,
                                 BmlVolCxt  *pstVol);
#endif /* FSR_NBL2 */

/**
 *  @brief  Code Implementation
 */

#if !(defined(TINY_FSR) || defined(FSR_NBL2))
/**
 *  @brief      This function prints BMI in the given volume.
 *
 *  @param [in]  nVol    : volume number
 *  @param [in] *pstVol  : pointer to VolCxt structure 
 *
 *  @return     none
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PRIVATE VOID
_PrintBMI(UINT32     nVol,
          BmlVolCxt *pstVol)
{
    UINT32      nDevIdx = 0;    /* Device Index             */
    UINT32      nPDev   = 0;    /* Physical device number   */
    BOOL32      nRe     = TRUE32;
    BmlDevCxt  *pstDev;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* this function assume that volume has always device,
       thus, this function doesn't check whether volume has device or not */
    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev   = nVol * DEVS_PER_VOL + nDevIdx;

        nRe     = _IsOpenedDev(nPDev);
        if (nRe == FALSE32)
        {
            continue;
        }

        pstDev = _GetDevCxt(nPDev);

        FSR_BBM_PrintBMI(pstVol, pstDev);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));
}

#endif /* TINY_FSR, FSR_NBL2 */

/**
 *  @brief      This function sets the partial area of volume to Locked Area
 *  @n          by Write Protection Scheme to protect system data such as OS Image.
 *
 *  @param [in]  nVol    : Volume number
 *  @param [in] *pstPartI: Pointer to FSRPartI structure
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     LLD Errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PRIVATE INT32
_ProtectLockedArea(UINT32     nVol, 
                   FSRPartI  *pstPartI)
{
    UINT32      nPDev       = 0;    /* Physical device number               */
    UINT32      nPEntryIdx  = 0;    /* PartEntry Index                      */
    UINT32      nDevIdx     = 0;    /* Device Index                         */
    UINT32      nDieIdx     = 0;    /* Die Index                            */
    UINT32      nNumOfPEntry= 0;    /* # of PartEntries                     */
    INT32       nBMLRe      = FSR_BML_SUCCESS;  /* BML Return value         */
    UINT16      nPEAttr     = 0;    /* The attributes of PartEntry          */
    BmlVolCxt  *pstVol;
    BmlDevCxt  *pstDev;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

#if defined(BML_CHK_PARAMETER_VALIDATION)
    /* check the main and spare buffer pointer */
    if (pstPartI == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Pointer of PartI is NULL \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstPartI: %d)) / %d line\r\n"),
                                        __FSR_FUNC__, pstPartI, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));
        return FSR_BML_INVALID_PARAM;
    }
#endif

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Set # of part entries */
    nNumOfPEntry = pstPartI->nNumOfPartEntry;
    
    /* Unlocked area of device is set as R/W area */
    for (nPEntryIdx = 0 ; nPEntryIdx < nNumOfPEntry ; nPEntryIdx++)
    {
        nPEAttr = pstPartI->stPEntry[nPEntryIdx].nAttr;

        /* If the attribute of a partition is 
         * neither FSR_BML_PI_ATTR_LOCK nor FSR_BML_PI_ATTR_LOCKTIGHTEN,
         * this partition is unlocked as R/W area*/
        if (!(nPEAttr & BML_LOCK_ATTR_MASK))
        {
            nBMLRe = _SetBlkState(nVol,
                                  pstPartI->stPEntry[nPEntryIdx].n1stVun,
                                  pstPartI->stPEntry[nPEntryIdx].nNumOfUnits,
                                  FSR_LLD_IOCTL_UNLOCK_BLOCK);
            if (nBMLRe != FSR_BML_SUCCESS)
            {
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));
                return nBMLRe;
            }
        } /* End of "if (!(nPEAttr & BML_LOCK_ATTR_MASK))" */
    } /* End of "for (nPEntryIdx = 0 ;...)" */

    /* Unlock reservoir by die */
    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev   = nVol * DEVS_PER_VOL + nDevIdx;
        pstDev  = _GetDevCxt(nPDev);

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            nBMLRe = FSR_BBM_UnlockRsvr(pstVol,
                                        pstDev,
                                        nDieIdx);
            if (nBMLRe != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d) / %d line\r\n"),
                                                    __FSR_FUNC__, nVol, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   FSR_BBM_UnlockRsvr(nPDev: %d, nDieIdx: %d)\r\n"),
                                                nPDev, nDieIdx));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));
                return nBMLRe;
            }
        } /* End of "for (nDieIdx = 0; nDieIdx...)" */
    } /* End of "for (nDevIdx = 0; nDevIdx...)" */

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    return nBMLRe;
}

/**
 *  @brief      This function reads virtual pages.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nVpn        : First virtual page number for reading
 *  @param [in]  nNumOfPgs   : The number of pages to read
 *  @param [in] *pMBuf       : Main page buffer for reading
 *  @param [in] *pSBuf       : Spare page buffer for reading
 *  @param [in]  nFlag       : FSR_BML_FLAG_ECC_ON
 *  @n                         FSR_BML_FLAG_ECC_OFF
 *  @n                         FSR_BML_FLAG_LSB_RECOVERY_LOAD
 *  @n                         FSR_BML_FLAG_USE_SPAREBUF
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_READ_ERROR
 *  @return     FSR_BML_CRITICAL_ERROR
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_ACQUIRE_SM_ERROR
 *  @return     FSR_BML_RELEASE_SM_ERROR
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PRIVATE INT32
_BML_Read(UINT32        nVol,
          UINT32        nVpn,
          UINT32        nNumOfPgs,
          UINT8        *pMBuf,
          FSRSpareBuf  *pSBuf,
          UINT32        nFlag)
{
    UINT32        nVun          = 0;    /* Virtual Unit number                                  */
    UINT32        nBaseSbn      = 0;    /* Base Sbn                                             */
    UINT32        nPDev         = 0;    /* Physical Device Number                               */
    UINT32        nDevIdx       = 0;    /* Device Index in volume                               */
    UINT32        nPgOffset     = 0;    /* Physical Page Offset (SLC Blk:0~63, MLC Blk: 0~127)  */
    UINT32        nPlnIdx       = 0;    /* Plane Index                                          */
    UINT32        nDieIdx       = 0;    /* Die Index                                            */
    UINT32        nWayIdx       = 0;    /* Way Index                                            */
    UINT32        nNumOfActWay  = 0;    /* # of used ways in actuality                          */
    UINT32        nRdFlag   = FSR_LLD_FLAG_1X_LOAD; /* LLD flag for Load/Pload/Transfer         */
    UINT32        nLLDFlag  = 0x00000000;   /* LLD flag for R1/OTP_read                             */
    UINT32        nSctNumOf1stVPg   = 0;    /* the given sector number of first page from STL       */
    UINT32        nSctNumOfLastVPg  = 0;    /* the given sector number of last page from STL        */
    UINT32        n1stSctOff        = 0;    /* the start sector offset for LLD_Read                 */
    UINT32        nLastSctOff       = 0;    /* the end sector offset for LLD_Read                   */
    UINT32        nMaxSctsPerVpg    = 0;    /* the total number of sectors per vpn                  */
    UINT32        nNumOfLoop        = 0;    /* # of times in proportion to vpn                      */
    UINT32        nNumOfBlksInRsvr  = 0;    /* # of reservec blks for Flex-OneNAND                  */
    UINT32        nIdx              = 0;    /* Temporary index                                      */
    UINT32        nRestPgs          = 0;    /* The rest pages to read                               */
    UINT32        nTINYFlag = FSR_LLD_FLAG_NONE;    /* LLD Flag for TINY FSR                        */
    BOOL32        bFirstAccess  = TRUE32;   /* First aceess to a die of device                      */
    BOOL32        bLastAccess[FSR_MAX_WAYS];/* Last access to a die of device                       */
    BOOL32        bLastPage[FSR_MAX_WAYS];  /* Last page to read                                    */
    BOOL32        bReadErr      = FALSE32;  /* Flag for indicating uncorrectable read error         */
    INT32         nMajorErr;                /* Major Return Error                                   */
    INT32         nRet = FSR_BML_SUCCESS;   /* Temporary return for BBM function                    */
    BOOL32        bReadDisturbErr = FALSE32;/* is read disturbance error ?                          */
    BOOL32        bChkRdDisturbErr = FALSE32;/* is read disturbance error ?                          */
    INT32         nBMLRe = FSR_BML_SUCCESS; /* BML Return value                                     */
    INT32         nLLDRe = FSR_LLD_SUCCESS; /* LLD Return value                                     */
    UINT8        *pOrgMBuf;                /* 1st sector buffer pointer of 1st main page            */
    FSRSpareBuf  *pOrgSBuf;                /* Original spare buffer pointer                         */
    BmlVolCxt    *pstVol;
    BmlDevCxt    *pstDev;
    BmlDieCxt    *pstDie;
    BmlShCxt     *pstShCxt;
    BmlAddrLog   *pstAddr[FSR_MAX_WAYS];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nVpn: %d, nNumOfPgs: %d, nFlag: 0x%x)\r\n")
                                    , __FSR_FUNC__, nVol, nVpn, nNumOfPgs, nFlag));

    /* check volume range */
    CHK_VOL_RANGE(nVol);

    /* Get pointer to Volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Get pointer to Shared context */
    pstShCxt = _GetShCxt();

    /* check volume open */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    FSR_ASSERT(nVol < FSR_MAX_VOLS);

    /* nFlag value for FSR_BML_Read & FSR_LLD_Read
     * --------------------------------------
     * FSR_BML_FLAG_R1_LOAD     (0x00000010)
     * --------------------------------------
     *  FSR_LLD_FLAG_R1_LOAD    (0x00000006)
     * --------------------------------------
     */
    /* Translate BML flag to LLD flag */

    nLLDFlag = 0x00000000;
    /* Set ECC ON/OFF flag */
    nLLDFlag |= nFlag & FSR_BML_FLAG_ECC_MASK;

    /* ADD 2007/07/10 
     * CHANGE 2007/07/19 */
    if ((nFlag & FSR_BML_FLAG_USE_SPAREBUF) == FSR_BML_FLAG_USE_SPAREBUF)
    {
        nLLDFlag |= FSR_LLD_FLAG_USE_SPAREBUF;
    }

    /* Set the nSctNumOf1stVPg, nSctNumOfLastVPg */
    nSctNumOf1stVPg   = (nFlag & FSR_BML_FLAG_1ST_SCTOFFSET_MASK) >>
                        FSR_BML_FLAG_1ST_SCTOFFSET_BASEBIT;
    nSctNumOfLastVPg  = (nFlag & FSR_BML_FLAG_LAST_SCTOFFSET_MASK) >>
                        FSR_BML_FLAG_LAST_SCTOFFSET_BASEBIT;

    /* Get the nMaxSctsPerVpg to calculate the sector offset of the last page */
    nMaxSctsPerVpg    = pstVol->nSctsPerPg * pstVol->nNumOfPlane;

#if defined(BML_CHK_PARAMETER_VALIDATION)
    /* check the boundaries of input parameter*/
    if ((nVpn + nNumOfPgs - 1) > pstVol->nLastVpn)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Input page is bigger than last Vpn \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVpn:%d, nNumOfPgs:%d, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nVpn, nNumOfPgs, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }

    /* check the boundary of input parameter */
    if (nNumOfPgs == 0)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Number of page is NULL \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nNumOfPgs:%d, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nNumOfPgs, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }

    /* check the boundaries of 1st sector offset and last sector offset*/
    if ((nSctNumOf1stVPg  >= (nMaxSctsPerVpg)) ||
        (nSctNumOfLastVPg >= (nMaxSctsPerVpg)))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] 1st sector or last sector offset is greater than number ofMax sector \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nSctNumOf1stVPg:%d, nSctNumOfLastVPg:%d, nMaxSctsPerVpg:%d, nRe:FSR_BML_INVALID_PARAM)\r\n"),
                                        __FSR_FUNC__, nSctNumOf1stVPg, nSctNumOfLastVPg, nMaxSctsPerVpg, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }

    /* check the main and spare buffer pointer */
    if ((pMBuf == NULL) && (pSBuf == NULL))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Main and Spare buffer point is NULL \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pMBuf:0x%x, pSBuf:0x%x, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, pMBuf, pSBuf, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }
#endif /* BML_CHK_PARAMETER_VALIDATION */

    /* Initialize bLastAccess[FSR_MAX_WAYS] */
    bLastAccess[0] = FALSE32;
    bLastAccess[1] = FALSE32;

    /* Initialize bLastPage[FSR_MAX_WAYS] */
    bLastPage[0] = FALSE32;
    bLastPage[1] = FALSE32;

    FSR_ASSERT(FSR_MAX_WAYS == 2);

    /* Determine # of actual ways by # of pages*/
    if (nNumOfPgs <= pstVol->nNumOfWays)
    {
        nNumOfActWay = nNumOfPgs;
    }
    else
    {
        nNumOfActWay = pstVol->nNumOfWays;
    }

    if (nNumOfPgs < (pstVol->nNumOfWays * BML_NUM_OF_LLD_READ_PER_PAGE))
    {
        bLastAccess[nNumOfActWay-1]  = TRUE32;

        if (nNumOfPgs == nNumOfActWay)
        {
            bLastPage[nNumOfActWay - 1] = TRUE32;

            bLastAccess[0]              = TRUE32;
            bLastAccess[1]              = TRUE32;
        }
    }

    /* Increase nNumOfPgs */
    nNumOfPgs += nNumOfActWay;

    /* Because the total number of loop increases in nNumOfLoad,
     * the buffer pointer should be changed as nNumOfLoad.
     */
    /* Set the pOrgMBuf*/ 
    if (pMBuf != NULL)
    {
        pOrgMBuf = pMBuf - (FSR_SECTOR_SIZE * nSctNumOf1stVPg) - (nNumOfActWay * pstVol->nSizeOfVPage);
        if (pstVol->nNumOfPlane != 1)
        {
            pOrgMBuf += pstVol->nSizeOfPage;
        }
    }
    else  /* If pMBuf is NULL, it should not change the buffer pointer */
    {
        pOrgMBuf = pMBuf;
    }

    /* Set the pOrgSBuf*/
    if (pSBuf != NULL)
    {
        pOrgSBuf = pSBuf - nNumOfActWay;
    }
    else /* If pSBuf is NULL, it should not change the buffer pointer */
    {
        pOrgSBuf = pSBuf;
    }

    /* Pre-operation for pstAddr[nWayIdx] and pNextSBuf[nWayIdx] */
    nWayIdx    = 0;
    do
    {
        /* create pstAddr[nWayIdx]*/
        pstAddr[nWayIdx]            = pstVol->pstAddr[nWayIdx];

        /* Set nRdFlag.
         * The first LLD_Read should use Load flag or LSB recovery load flag */
        if ((nFlag & FSR_BML_FLAG_LSB_RECOVERY_LOAD) == FSR_BML_FLAG_LSB_RECOVERY_LOAD)
        {
            pstAddr[nWayIdx]->nRdFlag = FSR_LLD_FLAG_LSB_RECOVERY_LOAD;
        }
        else
        {
            pstAddr[nWayIdx]->nRdFlag   = FSR_LLD_FLAG_1X_LOAD;
        }

    } while (++nWayIdx < nNumOfActWay);

    do
    {
        nWayIdx = 0;
        do
        {
            /********************************************************/
            /* Address translation                                  */
            /********************************************************/
            /* STEP1: translate PgOffset */
            /* page offset for SLC area  */
            nPgOffset = (nVpn >> (pstVol->nSftNumOfWays)) & pstVol->nMaskSLCPgs;

            if (nVpn >= pstVol->n1stVpnOfMLC)
            {
                /* page offset for MLC area */
                nPgOffset = ((nVpn - pstVol->n1stVpnOfMLC) >> (pstVol->nSftNumOfWays)) & pstVol->nMaskMLCPgs;
            }

            /* STEP2: translate PDev */
            nDevIdx  = (nVpn >> pstVol->nDDPMask) & pstVol->nMaskPDev;
            nPDev    = (nVol << DEVS_PER_VOL_SHIFT) + nDevIdx;
            pstDev   = _GetDevCxt(nPDev);

            /* STEP3: translate nDieIdx */
            nDieIdx  = nVpn & pstVol->nDDPMask;
            pstDie   = pstDev->pstDie[nDieIdx];

            /* Handle previous error using LLD_FlushOp and _HandlePrevError*/
            if (bFirstAccess == TRUE32)
            {
                nIdx = pstVol->nNumOfDieInDev - 1;
                do
                {
                    nLLDRe = pstVol->LLD_FlushOp(nPDev,
                                                 nIdx,  /* nDieIdx */
                                                 FSR_LLD_FLAG_NONE | nTINYFlag);
                    if (nLLDRe != FSR_LLD_SUCCESS)
                    {
#if !defined(FSR_NBL2)
                        if(pstShCxt->HandlePrevError)
                        {
                            nBMLRe = pstShCxt->HandlePrevError(nVol,
                                                               nPDev,
                                                               nIdx, /* nDieIdx */
                                                               nLLDRe);
                            if (nBMLRe != FSR_BML_SUCCESS)
                            {
                                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nVpn: %d, nNumOfPgs: %d, nFlag: 0x%x, nRe: 0x%x)\r\n"), 
                                                                __FSR_FUNC__, nVol, nVpn, nNumOfPgs, nFlag, nBMLRe));
                                break;
                            }
                        }
                        else
#endif /* FSR_NBL2 */
                        {
                            nBMLRe = nLLDRe;
                            break;
                        }
                    }
                } while (nIdx-- > 0);

                /* Error case */
                if (nBMLRe != FSR_BML_SUCCESS)
                {
                    break;
                }
            } /* End of "if (bFirstAccess == TRUE32)" */

            if ((nPgOffset == 0) || (bFirstAccess == TRUE32))
            {
                /* STEP4: translate Vun */
                /* Vun for SLC area */
                nVun                = nVpn >> (pstVol->nSftSLC + pstVol->nSftNumOfWays);
                nNumOfBlksInRsvr    = 0;
                if (nVpn >= pstVol->n1stVpnOfMLC)
                {
                    /* Vun for MLC area */
                    nVun        = (pstVol->nNumOfSLCUnit) +
                                  ((nVpn - (pstVol->nNumOfSLCUnit << (pstVol->nSftSLC + pstVol->nSftNumOfWays))) >>
                                  (pstVol->nSftMLC + pstVol->nSftNumOfWays));

                    /* Calculate # of reserved blocks by reservoit type */
                    if (pstDie->pstRsv->nRsvrType == BML_HYBRID_RESERVOIR)
                    {
                        nNumOfBlksInRsvr = pstDie->pstRsv->nLastSbnOfRsvr - pstDie->pstRsv->n1stSbnOfRsvr + 1;
                    }
                }

                /* Decrease nVun */
                /* Read operation needs extra LLD function call for super-load.
                 * When it reads a last page of the last block in a die,
                 * this code indicates out of the current die.
                 * So nVun should be decreased if it is lager than nLastUnit.*/
                if (nVun > pstVol->nLastUnit)
                {
                    nVun--;
                }

                if (bFirstAccess == TRUE32)
                {
                    /* Get the return value by partitionfor current partition */
                    nBMLRe = _GetPIRet(pstVol,
                                       pstDie,
                                       nVun,
                                       nBMLRe);
                    if (nBMLRe != FSR_BML_SUCCESS)
                    {
                        /* message out */
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVpn:%d, nNumOfPgs:%d, nFlag:0x%x, nRe:0x%x)\r\n"), 
                                                        __FSR_FUNC__, nVol, nVpn, nNumOfPgs, nFlag, nBMLRe));
                        break;
                    }

                    /* Store the type of a previous operation*/
                    pstDie->pstPreOp->nOpType = BML_PRELOG_READ;

                } /* End of "if (bFirstAccess == TRUE32)" */

                /* STEP5: translate Sbn[] to Pbn[] */
                nPlnIdx  = pstVol->nNumOfPlane - 1;
                nBaseSbn = ((nDieIdx) << pstVol->nSftNumOfBlksInDie) +
                           (nVun << pstVol->nSftNumOfPln)
                           + nNumOfBlksInRsvr;
                do
                {
                    pstDie->nCurSbn[nPlnIdx] = (UINT16)nBaseSbn + (UINT16)nPlnIdx;
                    pstDie->nCurPbn[nPlnIdx] = (UINT16)nBaseSbn + (UINT16)nPlnIdx;
                } while (nPlnIdx-- > 0);

                /* Second Translation: translate Pbn[] */
                pstDie->nNumOfLLDOp = 0;
                _GetPBN(pstDie->nCurSbn[0], pstVol, pstDie);
            }

            /* store address to an array */
            /* nRdFlag should be set after LLD_Read*/
            pstAddr[nWayIdx]->nPgOffset =  nPgOffset;
            pstAddr[nWayIdx]->nPDev     =  nPDev;
            pstAddr[nWayIdx]->nDieIdx   =  nDieIdx;

            pstAddr[nWayIdx]->pSBuf = pOrgSBuf;
            if (pOrgSBuf != NULL)
            {
                pstAddr[nWayIdx]->pSBuf     =  pOrgSBuf +  nNumOfLoop;
                if ((bFirstAccess != TRUE32) || (nWayIdx != 0))
                {
                    FSR_OAM_MEMCPY(&pstAddr[nWayIdx]->stExtraSBuf, pstAddr[nWayIdx]->pSBuf, sizeof(FSRSpareBuf));
                }
            }

            if (pOrgMBuf == NULL)/* Hold the pMBuf when pOrgMBuf is NULL*/
            {
                pstAddr[nWayIdx]->pMBuf = pOrgMBuf;
            }
            else
            {
                if ((bFirstAccess == TRUE32) || (pstAddr[nWayIdx]->pMBuf != NULL))
                {
                    pstAddr[nWayIdx]->pMBuf         =  pOrgMBuf + (nNumOfLoop * pstVol->nSizeOfVPage);
                    pstAddr[nWayIdx]->pExtraMBuf    =  pstAddr[nWayIdx]->pMBuf;
                }
            }

            /* the # of loops increses in proportion to the number of pages */
            nNumOfLoop++;

            /* Increase the nVpn*/
            nVpn++;

        } while (++nWayIdx < nNumOfActWay);

        if (nBMLRe != FSR_BML_SUCCESS)
        {
            break;
        }

        nPlnIdx = 0;
        do
        {
            nWayIdx     = 0;
            do
            {
                /* Get nPDev, nDieIdx, nPgOffset from pstAddr[nWayIdx]*/
                nPDev       = pstAddr[nWayIdx]->nPDev;
                nDieIdx     = pstAddr[nWayIdx]->nDieIdx;
                nPgOffset   = pstAddr[nWayIdx]->nPgOffset;
                pMBuf       = pstAddr[nWayIdx]->pMBuf;
                nRdFlag     = pstAddr[nWayIdx]->nRdFlag;

                /* Get pstDev, pstDie */
                pstDev   = _GetDevCxt(nPDev);
                pstDie   = pstDev->pstDie[nDieIdx];

                if (nRdFlag != FSR_LLD_FLAG_NONE)
                {
                    /* Set pSBuf */
                    pSBuf = pOrgSBuf;
                    if (pOrgSBuf != NULL)
                    {
                        pSBuf       = &(pstAddr[nWayIdx]->stExtraSBuf);
                        if (pSBuf->nNumOfMetaExt   != 0)
                        {
                            pSBuf->nNumOfMetaExt    = pstVol->nSizeOfPage / FSR_PAGE_SIZE_PER_SPARE_BUF_EXT;
                            if ((pSBuf->nNumOfMetaExt == 1) &&
                                (pstVol->nNumOfPlane  == FSR_MAX_PLANES))
                            {
                                pSBuf->pstSTLMetaExt    += (nPlnIdx + 1) & 0x01;
                            }
                        }
                    }

                    /* LLD_Read should be called when nRdFlag is not dummy */
                    nLLDRe = pstVol->LLD_Read(nPDev,
                                              pstDie->nCurPbn[nPlnIdx],
                                              nPgOffset,
                                              pMBuf,
                                              pSBuf,
                                              nLLDFlag | nRdFlag | nTINYFlag);

                    nMajorErr = FSR_RETURN_MAJOR(nLLDRe);

                    switch (nMajorErr)
                    {
                    case FSR_LLD_SUCCESS:
                        /* Store previous data */
                        pstDie->pstPreOp->nSbn = pstDie->nCurSbn[nPlnIdx];
                        break;

                    case FSR_LLD_PREV_1LV_READ_DISTURBANCE:
                        /* Store previous data */
                        pstDie->pstPreOp->nSbn = pstDie->nCurSbn[nPlnIdx];
                        bChkRdDisturbErr    = TRUE32;
                        break;

                    case FSR_LLD_PREV_2LV_READ_DISTURBANCE:
                        
                        bChkRdDisturbErr    = TRUE32;
                        bReadDisturbErr     = TRUE32;

                        /* Do not handle the disturbance error when deal with this block in upper layer */
                        if ((nFlag & FSR_BML_FLAG_IGNORE_READ_DISTURBANCE) != FSR_BML_FLAG_IGNORE_READ_DISTURBANCE)
                        {
                            /* Handle the disturbance errors using FSR_BBM_UpdateERL of BBM_FLAG_ERL_UPDATE */
                            if(pstShCxt->UpdateERL)
                            {
                                nRet = pstShCxt->UpdateERL(pstVol,
                                                           pstDev,
                                                           nDieIdx,
                                                           pstDie->pstPreOp->nSbn,
                                                           BML_FLAG_ERL_UPDATE);
                                if (nRet != FSR_BML_SUCCESS)/* ignore error */
                                {
                                    /* message out */
                                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   FSR_BBM_UpdateERL(nPDev:%d, nDieIdx:%d, nSbn:%d, nFlag: BBM_FLAG_ERL_UPDATE, nRe:FSR_LLD_PREV_READ_DISTURBANCE) , / %d line\r\n"),
                                                                    nPDev, nDieIdx, pstDie->pstPreOp->nSbn, __LINE__));
                                }
                                bReadDisturbErr     = TRUE32;
                            }
                        }
                        /* Store previous data */
                        pstDie->pstPreOp->nSbn      =   pstDie->nCurSbn[nPlnIdx];
                        break;

                    case FSR_LLD_PREV_READ_ERROR:
                        /* Store previous data */
                        pstDie->pstPreOp->nSbn      =   pstDie->nCurSbn[nPlnIdx];
                        bReadErr = TRUE32;
                        /* message out */
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Read(nPDev: %d, nPbn: %d, nPgOffset: %d, nFlag: 0x%x, nRe:FSR_LLD_PREV_READ_ERROR), / %d line\r\n"), 
                                                        nPDev, pstDie->nCurPbn[nPlnIdx], nPgOffset, nLLDFlag | nRdFlag, __LINE__));
                        break;

                    case FSR_LLD_INVALID_PARAM:     /* This code return LLD error to a upper layer*/
                        nBMLRe = nLLDRe;
                        /* message out */
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Read(nPDev: %d, nPbn: %d, nPgOffset: %d, nFlag: 0x%x, nRe: FSR_LLD_INVALID_PARAM), / %d line\r\n"), 
                                                        nPDev, pstDie->nCurPbn[nPlnIdx], nPgOffset, nLLDFlag | nRdFlag, __LINE__));
                        break;

                    default:
                        /* if code reaches this line, abnormal case.. */
                        nBMLRe = nLLDRe;
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Read(nPDev: %d, nPbn: %d, nPgOffset: %d, nFlag: 0x%x, nRe: 0x%x), / %d line\r\n"), 
                                                        nPDev, pstDie->nCurPbn[nPlnIdx], nPgOffset, nLLDFlag | nRdFlag, nBMLRe, __LINE__));
                        break;
                    }   /* End of switch(nMajorErr)*/

                    /* change the buffer pointer when main buffer pointer is not NULL*/
                    if (pOrgMBuf != NULL)
                    {
                        pstAddr[nWayIdx]->pExtraMBuf += (nNumOfActWay * pstVol->nSizeOfVPage - pstVol->nSizeOfPage);
                        pstAddr[nWayIdx]->pMBuf = pstAddr[nWayIdx]->pExtraMBuf;
                    }

                    if (pOrgSBuf != NULL)
                    {
                        pstAddr[nWayIdx]->pSBuf += nNumOfActWay;

                        FSR_OAM_MEMCPY(&pstAddr[nWayIdx]->stExtraSBuf, pstAddr[nWayIdx]->pSBuf, sizeof(FSRSpareBuf));

                        /* Set nNumOfMetaExt, pstSTLMetaExt */
                        pstAddr[nWayIdx]->stExtraSBuf.nNumOfMetaExt = pstAddr[nWayIdx]->pSBuf->nNumOfMetaExt;
                        pstAddr[nWayIdx]->stExtraSBuf.pstSTLMetaExt = pstAddr[nWayIdx]->pSBuf->pstSTLMetaExt;
                    }
                } /* End of "if (nRdFlag != FSR_LLD_FLAG_NONE)" */

                /****************************************************************/
                /* <Set a nRdFlag>                                              */
                /* 1. Previous nRdFlag == transfer only                         */
                /*  => nRdFlag = FSR_LLD_FLAG_NONE(skip LLD_Read)               */
                /* 2. Previous nRdFlag != transfer only                         */
                /*  => nRdFlag = FSR_LLD_FLAG_1X_PLOAD | FSR_LLD_FLAG_TRANSFER  */
                /****************************************************************/
                /* ADD 2007/07/20 (for recovery load) */
                if ((nRdFlag & FSR_LLD_FLAG_LSB_RECOVERY_LOAD) == FSR_LLD_FLAG_LSB_RECOVERY_LOAD)
                {
                    nRdFlag = FSR_LLD_FLAG_TRANSFER;
                }
                else if (((nRdFlag & FSR_LLD_FLAG_1X_PLOAD)  != FSR_LLD_FLAG_1X_PLOAD) &&
                    ((nRdFlag & FSR_LLD_FLAG_1X_LOAD)   != FSR_LLD_FLAG_1X_LOAD))
                {
                    nRdFlag = FSR_LLD_FLAG_NONE;
                }
                else
                {
                    /* Set Default flag*/
                    nRdFlag    = FSR_LLD_FLAG_1X_PLOAD | FSR_LLD_FLAG_TRANSFER;
                }

                /* Calculate the sector offset for the first page */
                n1stSctOff = 0;
                if ((bFirstAccess == TRUE32) && (nWayIdx == 0)) /* Is this page the 1st page? */
                {
                    if (nSctNumOf1stVPg != 0)
                    {
                        if (nSctNumOf1stVPg >= (pstVol->nSctsPerPg * (nPlnIdx+1)))
                        {
                            pstAddr[nWayIdx]->pMBuf = NULL;
                        }
                        else
                        {
                            if (nSctNumOf1stVPg >= (pstVol->nSctsPerPg * nPlnIdx))
                            {
                                n1stSctOff = nSctNumOf1stVPg - (pstVol->nSctsPerPg * nPlnIdx);
                            }
                        }
                    }
                }

                /* Calculate the sector offset for the last page */
                nLastSctOff = 0;
                /* Is the next loop the last access to a die?*/
                if (bLastAccess[nWayIdx] == TRUE32)
                {
                    if ((nSctNumOfLastVPg != 0) && (bLastPage[nWayIdx] == TRUE32))
                    {
                        if ((nMaxSctsPerVpg-nSctNumOfLastVPg) <= (pstVol->nSctsPerPg * nPlnIdx))
                        {
                            pstAddr[nWayIdx]->pMBuf = NULL;
                        }
                        if ((nSctNumOfLastVPg <  (nMaxSctsPerVpg - (pstVol->nSctsPerPg * nPlnIdx))) &&
                            (nSctNumOfLastVPg >= (nMaxSctsPerVpg - (pstVol->nSctsPerPg * (nPlnIdx+1)))))
                        {
                            nLastSctOff = nSctNumOfLastVPg - (nMaxSctsPerVpg - (pstVol->nSctsPerPg * (nPlnIdx+1)));
                        }
                    }

                    if (nPlnIdx == (pstVol->nNumOfPlane -1))
                    {
                        /* Chang the LLD Flag to NO_LOADCMD | TRANSFER */
                        nRdFlag = FSR_LLD_FLAG_NO_LOADCMD | FSR_LLD_FLAG_TRANSFER;
                        /* Initialize bLastAccess[nWayIdx] */
                        bLastAccess[nWayIdx] = FALSE32;
                        bLastPage[nWayIdx]   = FALSE32;
                    }
                }

                /* Store nRdFlag including n1stSctOff and nLastSctOff*/
                pstAddr[nWayIdx]->nRdFlag = (nRdFlag |
                                            (n1stSctOff << FSR_BML_FLAG_1ST_SCTOFFSET_BASEBIT) |
                                            (nLastSctOff << FSR_BML_FLAG_LAST_SCTOFFSET_BASEBIT));

                /* Handle the disturbance errors using FSR_BBM_UpdateERL of BBM_FLAG_ERL_PROGRAM */
                if ((nNumOfPgs          <= nNumOfActWay)            &&
                    (nPlnIdx            == pstVol->nNumOfPlane - 1) &&
                    (bReadDisturbErr    == TRUE32)                  &&
                    ((nFlag & FSR_BML_FLAG_IGNORE_READ_DISTURBANCE) != FSR_BML_FLAG_IGNORE_READ_DISTURBANCE))
                {
                    pstVol->LLD_FlushOp(pstDev->nDevNo,
                                        nDieIdx,
                                        FSR_LLD_FLAG_NONE);

                    if(pstShCxt->UpdateERL)
                    {
                        nRet = pstShCxt->UpdateERL(pstVol,
                                                   pstDev,
                                                   nDieIdx,
                                                   0,
                                                   BML_FLAG_ERL_PROGRAM);
                        /* ignore error */
                        if (nRet != FSR_BML_SUCCESS)
                        {
                            /* message out */
                            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   FSR_BBM_UpdateERL(nPDev:%d, nDieIdx:%d, nFlag: BBM_FLAG_ERL_PROGRAM, nRe:0x%x) / %d line\r\n"),
                                                            pstDev->nDevNo, nDieIdx, nRet, __LINE__));
                        }
                    }

#if !defined(TINY_FSR)
                    if ((nFlag & FSR_BML_FLAG_FORCED_ERASE_REFRESH) ==
                        FSR_BML_FLAG_FORCED_ERASE_REFRESH)
                    {
                        nRet = FSR_BBM_RefreshByErase(pstVol,
                                                      pstDev,
                                                      nDieIdx,
                                                      BML_FLAG_REFRESH_ALL);
                        if (nRet == FSR_BML_READ_ERROR)
                        {
                            bReadErr = TRUE32;
                        }
                        else if (nRet != FSR_BML_SUCCESS)
                        {
                            /* message out */
                            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   FSR_BBM_RefreshByErase(nPDev:%d, nDieIdx:%d, nRe:0x%x) / %d line\r\n"),
                                                            pstDev->nDevNo, nDieIdx, nRet, __LINE__));
                        }
                    }
#endif /* TINY_FSR */

                }
            } while (++nWayIdx < nNumOfActWay);

            if (nBMLRe != FSR_BML_SUCCESS)
            {
                break;
            }

        } while (++nPlnIdx < pstVol->nNumOfPlane);

        /* Set bFirstAccess to FALSE32 */
        bFirstAccess = FALSE32;

        /* Error case */
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            break;
        }

        /* Reduce # of pages in proportion to nNumOfActWay*/
        nNumOfPgs -= nNumOfActWay;

        /* Reduce vpn to hold the nVpn at last aceess*/
        if (nNumOfPgs <= nNumOfActWay)
        {
            nVpn -= nNumOfActWay;
        }

        /* Calculate the next nNumOfActWay*/
        if (nNumOfPgs > pstVol->nNumOfWays)
        {
            nNumOfActWay = pstVol->nNumOfWays;
            for (nIdx = 0 ; nIdx < nNumOfActWay; nIdx++)
            {
                nRestPgs = nNumOfPgs - nIdx;
                if ((nRestPgs > pstVol->nNumOfWays)                             &&
                    (nRestPgs <= (pstVol->nNumOfWays * BML_NUM_OF_LLD_READ_PER_PAGE)))
                {
                    bLastAccess[nIdx] = TRUE32;
                }
            }
            if (nNumOfPgs <= (pstVol->nNumOfWays * BML_NUM_OF_LLD_READ_PER_PAGE)) /* Is the next loop the last access to a die?*/
            {
                bLastPage[nNumOfPgs - pstVol->nNumOfWays - 1] = TRUE32;
            }
        }
        else /* The last loop */
        {
            /* The next loop is the dummy loop for transferring data from a dataRAM to DRAM */
            nNumOfActWay = nNumOfPgs;
        }

    } while (nNumOfPgs > 0);

    /*
     * if the uncorrectable read error exists, FSR_BML_READ_ERROR should be returned.
     * if the disturbance error occurs, FSR_BML_READ_DISTURBANCE_ERROR should be returned.
     * if both the uncorrectable error and the disturbance error occurs,
     * FSR_BML_READ_ERROR should be returned.
     */
    if (bReadErr == TRUE32)
    {
        nBMLRe = FSR_BML_READ_ERROR;
    }
    else
    {
        if (((nFlag & FSR_BML_FLAG_FORCED_ERASE_REFRESH)    != FSR_BML_FLAG_FORCED_ERASE_REFRESH)  &&
            ((nFlag & FSR_BML_FLAG_INFORM_DISTURBANCE_ERROR) == FSR_BML_FLAG_INFORM_DISTURBANCE_ERROR))
        {
            if (bChkRdDisturbErr == TRUE32)
            {
                if (bReadDisturbErr == TRUE32)
                {
                    nBMLRe = FSR_BML_2LV_READ_DISTURBANCE_ERROR;
                }
                else
                {
                    nBMLRe = FSR_BML_1LV_READ_DISTURBANCE_ERROR;
                }
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function initializes the data structure of BML and
 *  @n          calls LLD_Init.
 *
 *  @param [in]  nFlag       : FSR_BML_FLAG_NONE
 *  @n                         FSR_BML_FORCED_INIT
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_OAM_ACCESS_ERROR
 *  @return     FSR_BML_ALREADY_INITIALIZED
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_Init(UINT32    nFlag)
{
    INT32             nBMLRe    = FSR_BML_SUCCESS;
    INT32             nPAMRe    = FSR_PAM_SUCCESS;
    INT32             nOAMRe    = FSR_OAM_SUCCESS;
    PRIVATE BOOL32    bBMLInitFlag = FALSE32;   /* Flag to inform calling FSR_BML_init */
    FsrVolParm        stPAM[FSR_MAX_VOLS];
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nFlag: 0x%x)\r\n"),
        __FSR_FUNC__,
        nFlag));

    if (nFlag == FSR_BML_FLAG_NONE)
    {
        /* Check that FSR_BML_Init is already called*/
        if (bBMLInitFlag == TRUE32)
        {
            /* Message out */
            FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]   %s(nFlag:0x%x, nRe:FSR_BML_ALREADY_INITIALIZED) / %d line\r\n"),
                                            __FSR_FUNC__, nFlag, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ALREADY_INITIALIZED));
            return FSR_BML_ALREADY_INITIALIZED;
        }

    } /* End of "if (nFlag == FSR_BML_FLAG_NONE)" */

    nPAMRe = FSR_PAM_Init();
    if (nPAMRe != FSR_PAM_SUCCESS)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nPAMRe));
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    nOAMRe = FSR_OAM_Init();
    if (nOAMRe != FSR_OAM_SUCCESS)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nOAMRe));
        return FSR_BML_OAM_ACCESS_ERROR;
    }

    nOAMRe = FSR_OAM_InitDMA();
    if (nOAMRe != FSR_OAM_SUCCESS)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nOAMRe));
        return FSR_BML_OAM_ACCESS_ERROR;
    }

    nPAMRe = FSR_PAM_GetPAParm(stPAM);
    if (nPAMRe != FSR_PAM_SUCCESS)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nPAMRe));
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    nBMLRe = _InitBIF(stPAM, nFlag);
    if (nBMLRe != FSR_BML_SUCCESS)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nBMLRe));
        return nBMLRe;
    }

    bBMLInitFlag = TRUE32;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT ] --%s\r\n"), __FSR_FUNC__));

    return FSR_BML_SUCCESS;
}


/**
 *  @brief      This function opens the volume.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nFlag       : FSR_BML_FLAG_REFRESH_PARTIAL
    @n                         FSR_BML_FLAG_REFRESH_ALL
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_UNLOCK_ERROR
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     some BML errors
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_Open(UINT32    nVol,
             UINT32    nFlag)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32      nVersion;                               /* Version name                         */
#endif
#if !(defined(TINY_FSR) || defined(FSR_NBL2))
    UINT32      nLockFlag= FSR_BML_OTP_OTP_BLK_LOCKED;  /* nFlag for lock state of OTP Block    */
#endif /* TINY_FSR, FSR_NBL2 */
#if !defined(TINY_FSR)
    UINT32      nERFlag = BML_FLAG_REFRESH_PARTIAL;     /* nFlag for erase-refresh operation    */
#endif /* TINY_FSR */
    UINT32      nLockLayer;
    INT32       nBMLRe  = FSR_BML_SUCCESS;
    INT32       nRet    = FSR_BML_SUCCESS;
    BOOL32      bRe     = TRUE32;
    BmlVolCxt  *pstVol;
#if !defined(TINY_FSR)
    BmlShCxt   *pstShCxt;
#endif /* TINY_FSR */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]   FSR VERSION: %s\r\n"), 
        FSR_Version(&nVersion)));

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nFlag: 0x%x)\r\n"), __FSR_FUNC__, nVol, nFlag));

    /* check volume range */
    CHK_VOL_RANGE(nVol);

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

#if !defined(TINY_FSR)
    /* Get pointer of Shared context */
    pstShCxt = _GetShCxt();
#endif /* TINY_FSR */

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    nLockLayer = _GetLockLayer(nVol);

    /* Acquire global lock */
    /* If single core, return value must be always TRUE32 from PAM                 */
    bRe = pstVol->AcquireLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ACQUIRE_SM_ERROR));
        return FSR_BML_ACQUIRE_SM_ERROR;
    }

#if !defined(TINY_FSR)
    /* Set BBMFlag for erase-refresh operation  */
    nERFlag = BML_FLAG_REFRESH_PARTIAL;
    if (nFlag & FSR_BML_FLAG_REFRESH_ALL)
    {
        nERFlag = BML_FLAG_REFRESH_ALL;
    }
#endif /* TINY_FSR */

    do
    {
        /* If BML_Open is already called, BML_Open just is returned. */
        if (pstVol->bVolOpen == TRUE32)
        {
            pstVol->nOpenCnt++;
            FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]   %s(nVol:%d, nFlag:0x%x, nOpenCnt:%d) / %d line\r\n"),
                                            __FSR_FUNC__, nVol, nFlag, pstVol->nOpenCnt, __LINE__));
            break;
        }

        /* Open a volume */
        nBMLRe = _Open(nVol);
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, nFlag, nBMLRe));
            nRet    = _Close(nVol);
            if (nRet != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, nFlag, nRet));
            }
            break;
        }

        /* Mount the Reservoir */
        nBMLRe = _MountRsvr(nVol,
                            pstVol->pstPartI,
                            pstVol->pPIExt);
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, nFlag, nBMLRe));
            nRet    = _Close(nVol);
            if (nRet != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, nFlag, nRet));
            }
            break;
        }

        /* Protect RO-Area */
        nBMLRe = _ProtectLockedArea(nVol,
                                    pstVol->pstPartI);
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            /*
             * If lock operation returns error, BML open procedure should be stopped
             * (FSR has changed the policy, 2008/01/11)
             */
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, nFlag, nBMLRe));
            break;
        }

        /* Initialize pstVol->bOTPEnable*/
        pstVol->bOTPEnable = FALSE32;

#if !(defined(TINY_FSR) || defined(FSR_NBL2))
        /* ONLY support OTP functionality for volume #0 */
        if (nVol == 0) 
        {
            if ((pstVol->nNANDType == FSR_LLD_FLEX_ONENAND) ||
                (pstVol->nNANDType == FSR_LLD_SLC_ONENAND))
            {
                /* enable OTP functionality */
                pstVol->bOTPEnable = TRUE32;

                /* <get OTP lock info from device 0>
                 * FSR_BML_Open should success except _Open() and _MountRsvr().
                 * So, this error is ignored.
                 */
                nRet = _GetOTPInfo(nVol,
                                   &nLockFlag);
                if(nRet != FSR_BML_SUCCESS)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, nFlag, nRet));
                }

                if (nLockFlag & FSR_BML_OTP_1ST_BLK_LOCKED)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]  1st block is locked\r\n")));
                }
                if (nLockFlag & FSR_BML_OTP_OTP_BLK_LOCKED)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]  OTP block is locked\r\n")));
                }
            } /* End of "if ((pstVol->nNANDType == ...)" */
        } /* End of "if (nVol == 0)" */

        _PrintBMI(nVol, pstVol);

        _PrintPartI(nVol, pstVol->pstPartI);

#endif /* TINY_FSR, FSR_NBL2 */

        /* diable pre-programming flag */
        pstVol->bPreProgram = FALSE32;

        /* enable volume open flag */
        pstVol->bVolOpen    = TRUE32;

        /* set open count */
        pstVol->nOpenCnt    = 1;

#if !defined(TINY_FSR)
        if (!(nFlag & FSR_BML_FLAG_DO_NOT_REFRESH))
        {
            /*
             * FSR_BML_Open should success except _Open() and _MountRsvr().
             * So, this error is ignored.
             */
            nRet = _ProcessEraseRefresh(nVol,
                                        nERFlag);
            if (nRet != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nFlag:0x%x, nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, nFlag, nRet));
                if (nRet == FSR_BML_CANT_UNLOCK_BLOCK)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:ERR] Unlock fail occurs during Erase Refresh \r\n")));
                    nBMLRe = nRet;
                }
                else if (nRet == FSR_BML_NO_RSV_BLK_POOL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:ERR] No Reservior pool error occurs during Erase Refresh \r\n")));
                    nBMLRe = FSR_BML_CRITICAL_ERROR;
                }
            }
        }

        /* Alloc shared function pointer */
#if !defined(FSR_NBL2)
        pstShCxt->HandlePrevError = _HandlePrevError;
#endif /* FSR_NBL2 */
        pstShCxt->UpdateERL       = FSR_BBM_UpdateERL;

#endif /* TINY_FSR */

    } while (0);

    bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_RELEASE_SM_ERROR));
        nBMLRe = FSR_BML_RELEASE_SM_ERROR;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    return nBMLRe;
}

#if !defined(FSR_NBL2)
/**
 *  @brief      This function closes the current volume.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nFlag       : FSR_BML_FLAG_NONE
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     FSR_BML_CRITICAL_ERROR
 *  @return     some BML errors
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_Close(UINT32    nVol,
              UINT32    nFlag)
{
    UINT32          nDevIdx = 0;                    /* Device Index               */
    UINT32          nDieIdx = 0;                    /* Die Index                  */
    UINT32          nPDev   = 0;                    /* Physical Device Index      */
    UINT32          nTINYFlag = FSR_LLD_FLAG_NONE;  /* LLD flag for TINY FSR      */
    UINT32          nLockLayer;
    BOOL32          bRe     = TRUE32;
    INT32           nLLDRe  = FSR_LLD_SUCCESS;
    INT32           nBMLRe  = FSR_BML_SUCCESS;
    BmlVolCxt      *pstVol;
    BmlShCxt       *pstShCxt;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nFlag: 0x%x)\r\n"), __FSR_FUNC__, nVol, nFlag));

    /* checking volume range */
    CHK_VOL_RANGE(nVol);

    /* Get the pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Get the pointer of shared context */
    pstShCxt = _GetShCxt();

    /* check volume open */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    FSR_ASSERT(nVol < FSR_MAX_VOLS);

    if ((nFlag & FSR_BML_FLAG_CLOSE_ALL) == FSR_BML_FLAG_CLOSE_ALL)
    {
        pstVol->nOpenCnt = 0;
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ] --%s(Open count of volume sets 0.)\r\n"),__FSR_FUNC__));
    }
    else
    {
        if (--pstVol->nOpenCnt != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:   ]   %s(nVol: %d, nFlag: 0x%x, nOpenCnt: %d)\r\n"),
                __FSR_FUNC__, nVol, nFlag, pstVol->nOpenCnt));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_SUCCESS));
            return FSR_BML_SUCCESS;
        }
    }

    nLockLayer = _GetLockLayer(nVol);

    /* Acquire semaphore */
    bRe = pstVol->AcquireLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ACQUIRE_SM_ERROR));
        return FSR_BML_ACQUIRE_SM_ERROR;
    }

    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        bRe = _IsOpenedDev(nPDev);
        if (bRe == FALSE32)
        {
            continue;
        }

        for (nDieIdx = 0; nDieIdx <pstVol->nNumOfDieInDev; nDieIdx++)
        {
            nLLDRe = pstVol->LLD_FlushOp(nPDev,
                                         nDieIdx,
                                         FSR_LLD_FLAG_NONE | nTINYFlag);
            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                if(pstShCxt->HandlePrevError)
                {
                    nBMLRe = pstShCxt->HandlePrevError(nVol,
                                                       nPDev,
                                                       nDieIdx,
                                                       nLLDRe);
                    if (nBMLRe != FSR_BML_SUCCESS)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nFlag: 0x%x, nRe: 0x%x)\r\n"),
                                                         __FSR_FUNC__, nVol, nFlag, nBMLRe));
                        break;
                    }
                }
                else
                {
                    nBMLRe = nLLDRe;
                }
            }
        } /* End of "for (nDieIdx = 0;...)" */

        /* handle an error of _HandlePrevError */
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            break;
        }

    } /* End of "for (nDevIdx = 0;...)"*/

    bRe = _Close(nVol);
    if (bRe != FSR_BML_SUCCESS)
    {
        nBMLRe = FSR_BML_CRITICAL_ERROR;
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nFlag: 0x%x, nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, nFlag, nBMLRe));
    }

#if !defined(TINY_FSR)
    /* Free shared function pointer */
    pstShCxt->HandlePrevError = NULL;
    pstShCxt->UpdateERL       = NULL;
#endif

    bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_RELEASE_SM_ERROR));
        nBMLRe = FSR_BML_RELEASE_SM_ERROR;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nBMLRe));

    return nBMLRe;
}
#endif /* #if !(defined(TINY_FSR) */

/**
 *  @brief      This function get the volume and device information.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in] *pstVolSpec  : Pointer to store the volume information
 *  @param [in]  nFlag       : FSR_BML_FLAG_NONE
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_PAM_ACCESS_ERROR
 *  @return     FSR_BML_DEVICE_ACCESS_ERROR
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_GetVolSpec(UINT32        nVol,
                   FSRVolSpec   *pstVolSpec,
                   UINT32        nFlag)
{
    INT32        nBMLRe         = FSR_BML_SUCCESS;  /* BML Return value             */
    INT32        nRe            = FSR_BML_SUCCESS;  /* Temporary return value       */
    UINT32       nIdx           = 0;                /* Temporary index              */
    UINT32       nMaxPgsInBlk   = 0;                /* Maximum # of pages in block  */
    UINT32       nLockLayer;
    BOOL32       bRe            = TRUE32;
    BOOL32       bOpenStat      = FALSE32;          /* Current Open status of BML   */
#if defined(FSR_OAM_DBGMSG_ENABLE)
    UINT8       *pstNANDType;
#endif
    BmlVolCxt   *pstVol;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nFlag: 0x%x)\r\n"), __FSR_FUNC__, nVol, nFlag));

    /* Check the volume range */
    CHK_VOL_RANGE(nVol);

    /* Get the pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    nLockLayer = _GetLockLayer(nVol);

    /* Acquire semaphore */
    bRe = pstVol->AcquireLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ACQUIRE_SM_ERROR));
        return FSR_BML_ACQUIRE_SM_ERROR;
    }

    do
    {
        /* save current Open status of BML   */
        bOpenStat = pstVol->bVolOpen;

        /* In case that FSR_BML_Open is not called yet */
        if (pstVol->bVolOpen != TRUE32)
        {
            /* Open a volume */
            nBMLRe = _Open(nVol);
            if (nBMLRe != FSR_BML_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, nBMLRe));
                break;
            }
        }

        if (pstVolSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Pointer of the volume spec  is NULL \r\n")));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, pstVolSpec: 0x%x, nRe: FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                            __FSR_FUNC__, nVol, pstVolSpec, __LINE__));
            nBMLRe = FSR_BML_INVALID_PARAM;
            break;
        }


        /* Set the value of volume context for FSRVolSpec */
        pstVolSpec->nNumOfUsUnits   =  (UINT16) (pstVol->nNumOfUsBlks / (pstVol->nNumOfPlane * pstVol->nNumOfWays));
        pstVolSpec->nNumOfWays      =  (UINT8)  pstVol->nNumOfWays ;
        pstVolSpec->nPgsPerMLCUnit  =  (UINT16) pstVol->nNumOfPgsInMLCBlk * (UINT16)pstVol->nNumOfWays;
        pstVolSpec->nPgsPerSLCUnit  =  (UINT16) pstVol->nNumOfPgsInSLCBlk * (UINT16)pstVol->nNumOfWays;
        pstVolSpec->nSctsPerPg      =  (UINT8)  pstVol->nSctsPerPg * (UINT8)pstVol->nNumOfPlane;
        pstVolSpec->nDiesPerDev     =  (UINT8)  pstVol->nNumOfDieInDev;
        pstVolSpec->nPlnsPerDie     =  (UINT8)  pstVol->nNumOfPlane;
        pstVolSpec->nNANDType       =  pstVol->nNANDType;
        pstVolSpec->nSparePerSct    =  (UINT16)pstVol->nSparePerSct;

        pstVolSpec->nFeature = FSR_BML_FEATURE_NONE;
        if (pstVol->nNANDType == FSR_LLD_FLEX_ONENAND)
        {
            pstVolSpec->nFeature   = FSR_BML_FEATURE_LSB_RECOVERY_LOAD;
        }

        pstVolSpec->b1stBlkOTP      =  pstVol->b1stBlkOTP;

        pstVolSpec->bVolOpen        =  bOpenStat;

        pstVolSpec->nSLCTLoadTime   =  pstVol->nSLCTLoadTime;
        pstVolSpec->nMLCTLoadTime   =  pstVol->nMLCTLoadTime;
        pstVolSpec->nTEraseTime     =  pstVol->nTEraseTime;
        pstVolSpec->nSLCTProgTime   =  pstVol->nSLCTProgTime;

        pstVolSpec->nMLCTProgTime[0]=  pstVol->nMLCTProgTime[0];
        pstVolSpec->nMLCTProgTime[1]=  pstVol->nMLCTProgTime[1];

        pstVolSpec->nSLCPECycle     = pstVol->nSLCPECycle;
        pstVolSpec->nMLCPECycle     = pstVol->nMLCPECycle;

        nMaxPgsInBlk    = pstVol->nNumOfPgsInSLCBlk;
        if (pstVol->nNumOfPgsInMLCBlk > pstVol->nNumOfPgsInSLCBlk)
        {
            nMaxPgsInBlk= pstVol->nNumOfPgsInMLCBlk;
        }
        pstVolSpec->nSizeOfDumpBuf  = nMaxPgsInBlk * (pstVol->nSizeOfPage + (pstVol->nSparePerSct*pstVol->nSctsPerPg));

        pstVolSpec->nRsvrBlksPerDie = (UINT16) (pstVol->nNumOfRsvrBlks / pstVol->nNumOfDieInDev);

        pstVolSpec->nUserOTPScts    = (UINT32) pstVol->nUserOTPScts;

        FSR_OAM_MEMCPY(pstVolSpec->nUID, pstVol->nUID, FSR_LLD_UID_SIZE);

        /* Message out */
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   << Volume(%d) Specification >>\r\n"), nVol));

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nPgsPerSLCUnit    : %12d\r\n"),
                                        pstVolSpec->nPgsPerSLCUnit));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nPgsPerMLCUnit    : %12d\r\n"),
                                        pstVolSpec->nPgsPerMLCUnit));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSctsPerPg        : %12d\r\n"),
                                        pstVolSpec->nSctsPerPg));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nNumOfWays        : %12d\r\n"),
                                        pstVolSpec->nNumOfWays));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nNumOfUsUnits     : %12d\r\n"),
                                        pstVolSpec->nNumOfUsUnits));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSparePerSct      : %12d\r\n"),
                                        pstVolSpec->nSparePerSct));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nDiesPerDev       : %12d\r\n"),
                                        pstVolSpec->nDiesPerDev));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nPlnsPerDie       : %12d\r\n"),
                                        pstVolSpec->nPlnsPerDie));

#if defined(FSR_OAM_DBGMSG_ENABLE)
        pstNANDType = (UINT8 *) "";
        if (pstVolSpec->nNANDType == FSR_LLD_FLEX_ONENAND)
        {
            pstNANDType = (UINT8 *) "Flex-OneNAND";
        }
        else if (pstVolSpec->nNANDType == FSR_LLD_SLC_ONENAND)
        {
            pstNANDType = (UINT8 *) "     OneNAND";
        }
#endif

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nNANDType         : %s\r\n"),
                                        pstNANDType));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nFeature          : %12d\r\n"),
                                        pstVolSpec->nFeature));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSLCTLoadTime     : %12d us\r\n"),
                                        pstVolSpec->nSLCTLoadTime));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nMLCTLoadTime     : %12d us\r\n"),
                                        pstVolSpec->nMLCTLoadTime));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSLCTProgTime     : %12d us\r\n"),
                                        pstVolSpec->nSLCTProgTime));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nMLCTProgTime[0]  : %12d us\r\n"),
                                        pstVolSpec->nMLCTProgTime[0]));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nMLCTProgTime[1]  : %12d us\r\n"),
                                        pstVolSpec->nMLCTProgTime[1]));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nTEraseTime       : %12d us\r\n"),
                                        pstVolSpec->nTEraseTime));

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSLCPECycle       : %12d\r\n"),
                                        pstVolSpec->nSLCPECycle));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nMLCPECycle       : %12d\r\n"),
                                        pstVolSpec->nMLCPECycle));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   b1stBlkOTP        : %12d\r\n"),
                                        pstVolSpec->b1stBlkOTP));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nUserOTPScts      : %12d\r\n"),
                                        pstVolSpec->nUserOTPScts));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nSizeOfDumpBuf    : %12d\r\n"),
                                        pstVolSpec->nSizeOfDumpBuf));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   bVolOpen          : %12d\r\n"),
                                        pstVolSpec->bVolOpen));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   nRsvrBlksPerDie   : %12d\r\n"),
                                        pstVolSpec->nRsvrBlksPerDie));

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("[BIF:   ]   UID :")));

        /* Get Unique ID */
        for (nIdx = 0; nIdx < FSR_LLD_UID_SIZE; nIdx++)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT(" %02x"), pstVolSpec->nUID[nIdx]));
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_INF, (TEXT("\r\n")));

    } while(0);

    /* In case that FSR_BML_Open is not called yet */
    if (pstVol->bVolOpen != TRUE32)
    {
        /* Close a volume */
        nRe    = _Close(nVol);
        if (nRe != FSR_BML_SUCCESS)
        {
            nBMLRe = nRe;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, nBMLRe));
        }
    }

    bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_RELEASE_SM_ERROR));
        nBMLRe = FSR_BML_RELEASE_SM_ERROR;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function loads partition entry that corresponds with the given nID.
 *
 *  @param [in]   nVol        : Volume number
 *  @param [in]   nID         : ID of Partition Entry
 *  @param [out] *pn1stVpn    : pointer to the first virtual page number
 *  @param [out] *pnPgsPerUnit: pointer to the number of pages per unit
 *  @param [out] *pstPartEntry: pointer to FSRPartEntry structure
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     FSR_BML_NO_PIENTRY
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_LoadPIEntry(UINT32        nVol,
                    UINT32        nID,
                    UINT32       *pn1stVpn,
                    UINT32       *pnPgsPerUnit,
                    FSRPartEntry *pstPartEntry)
{
    UINT32      nPEIdx      = 0;        /* Partition Entry Index                     */
    UINT32      n1stVpn     = 0;        /* The first virtual page number             */
    UINT32      nPgsPerUnit = 0;        /* # of pages in unit                        */
    BOOL32      bFound = FALSE32;       /* if bFound is FALSE32, no PartEntry exists */
    INT32       nBMLRe = FSR_BML_SUCCESS;
    BmlVolCxt  *pstVol;
    FSRPartI   *pstPI;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d,nID: 0x%x)\r\n"), __FSR_FUNC__, nVol, nID));

    /* check the volume range */
    CHK_VOL_RANGE(nVol);

    /* Get the pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Check whether this volume is opened */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* Get the pointer of partition information context */
    pstPI  = pstVol->pstPartI;

    if (pstPartEntry == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Pointer of the partition entry is NULL \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nID:0x%x, pstPartEntry:0x%x, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nVol, nID, pstPartEntry, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: FSR_BML_INVALID_PARAM) \r\n"), __FSR_FUNC__));
        return FSR_BML_INVALID_PARAM;
    }

    for (nPEIdx = 0; nPEIdx < pstPI->nNumOfPartEntry; nPEIdx++)
    {
        if (pstPI->stPEntry[nPEIdx].nID == nID)
        {
            FSR_OAM_MEMCPY(pstPartEntry, &(pstPI->stPEntry[nPEIdx]), sizeof(FSRPartEntry));

             if ((pstPartEntry->nAttr & BML_PART_ATTR_NANDTYPE_MASK) == FSR_BML_PI_ATTR_SLC)
             {
                 nPgsPerUnit = (pstVol->nNumOfPgsInSLCBlk * pstVol->nNumOfWays);
                 n1stVpn     = (pstPartEntry->n1stVun) * nPgsPerUnit;
             }
             else   /* MLC NAND */
             {
                 nPgsPerUnit = (pstVol->nNumOfPgsInMLCBlk * pstVol->nNumOfWays);
                 n1stVpn     = (pstVol->n1stVpnOfMLC) +
                               (pstPartEntry->n1stVun - pstVol->nNumOfSLCUnit) * nPgsPerUnit;
             }

             *pnPgsPerUnit = nPgsPerUnit;
             *pn1stVpn     = n1stVpn;

             bFound = TRUE32;
             break;
        }
    }

    if (bFound == FALSE32)
    {
        nBMLRe = FSR_BML_NO_PIENTRY;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x) \r\n"), __FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function get the 1st virtual page number and
 *  @n          the number of pages per unit for input nVun.
 *
 *  @param [in]   nVol        : Volume number
 *  @param [in]   nVun        : Virtual Unit Number
 *  @param [out] *pn1stVpn    : 1st virtual page number of nVun
 *  @param [out] *pnPgsPerUnit: The number of page per unit

 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_GetVirUnitInfo(UINT32        nVol,
                       UINT32        nVun,
                       UINT32       *pn1stVpn,
                       UINT32       *pnPgsPerUnit)
{
    UINT32       n1stVpn        = 0;    /* The 1st virtual page number  */
    UINT32       nPgsPerUnit    = 0;    /* # of pages per unit          */
    INT32        nBMLRe         = FSR_BML_SUCCESS;
    BmlVolCxt   *pstVol;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nVun: %d)\r\n"), __FSR_FUNC__, nVol, nVun));

    /* Check the volume range */
    CHK_VOL_RANGE(nVol);

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Check whether this volume is opened */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    do
    {
        if ((pn1stVpn == NULL) || (pnPgsPerUnit == NULL))
        {
            nBMLRe = FSR_BML_INVALID_PARAM;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Pointer of the 1st Vpn or PgsPerUnit is NULL \r\n")));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVun:%d, pn1stVpn:0x%x, pnPgsPerUnit:0x%x, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                            __FSR_FUNC__, nVol, nVun, pn1stVpn, pnPgsPerUnit, __LINE__));
            break;
        }

        /* if nVun is include in MLC area */
        if (nVun > (pstVol->nNumOfSLCUnit - 1))
        {
            nPgsPerUnit = (pstVol->nNumOfPgsInMLCBlk * pstVol->nNumOfWays);
            n1stVpn     = (pstVol->n1stVpnOfMLC) +
                          (nVun - pstVol->nNumOfSLCUnit) * nPgsPerUnit;
        }
        else  /* if nVun is include in SLC area */
        {
            nPgsPerUnit = (pstVol->nNumOfPgsInSLCBlk * pstVol->nNumOfWays);
            n1stVpn     = nVun * nPgsPerUnit;
        }

        *pnPgsPerUnit = nPgsPerUnit;
        *pn1stVpn     = n1stVpn;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function get the partition information.
 *
 *  @param [in]   nVol      : Volume number
 *  @param [out] *pstPartI  : pointer to FSRPartI structure
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
FSR_BML_GetFullPartI(UINT32     nVol,
                     FSRPartI  *pstPartI)
{
    INT32        nBMLRe = FSR_BML_SUCCESS;
    BmlVolCxt   *pstVol;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Check the volume range */
    CHK_VOL_RANGE(nVol);

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Check whether this volume is opened */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* Set pBufOut using pstVol->pstPartI */
    FSR_OAM_MEMCPY(pstPartI, pstVol->pstPartI, sizeof(FSRPartI));

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"), __FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function reads some sectors.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nVpn        : Virtual page number for reading
 *  @param [in]  n1stSecOff  : The first sector offset for reading
 *  @param [in]  nNumOfScts  : The number of secters to read
 *  @param [in] *pMBuf       : Main page Buffer pointer
 *  @param [in] *pSBuf       : Spare page Buffer pointe
 *  @param [in]  nFlag       : FSR_BML_FLAG_ECC_ON  || FSR_BML_FLAG_USE_SPAREBUF
 *  @n                         FSR_BML_FLAG_ECC_OFF || FSR_BML_FLAG_USE_SPAREBUF
 *  @n                         FSR_BML_FLAG_ECC_ON
 *  @n                         FSR_BML_FLAG_ECC_OFF
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     Some BML errors
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32 
FSR_BML_ReadScts(UINT32         nVol,
                 UINT32         nVpn,
                 UINT32         n1stSecOff,
                 UINT32         nNumOfScts,
                 UINT8         *pMBuf,
                 FSRSpareBuf   *pSBuf,
                 UINT32         nFlag)
{
    UINT32        nLastSecOff   = 0;    /* Last sector offset           */
    UINT32        nNumOfPgs     = 0;    /* # of pages                   */
    UINT32        nMaxSctsPerVpg= 0;    /* # of sectors per Vpn         */
    UINT32        nSftSctsPerPg = 0;    /* Shift bit per Vpn            */
    UINT32        nMaskSctsPerPg= 0;    /* Mask bit per Vpn             */
    UINT32        nLockLayer;
    INT32         nBMLRe = FSR_BML_SUCCESS;
    BOOL32        bRe;
    BmlVolCxt    *pstVol;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nVpn: %d, n1stSecOff: %d, nNumOfScts: %d, nFlag: 0x%x)\r\n"),
                                    __FSR_FUNC__, nVol, nVpn, n1stSecOff, nNumOfScts));

    /* check the volume range */
    CHK_VOL_RANGE(nVol);

    /* Get the pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* Check whether this volume is opened */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* Set nMaxSctsPerVpg by # of plane and # of sectors per physical page*/
    nMaxSctsPerVpg = pstVol->nSctsPerPg * pstVol->nNumOfPlane;

#if defined(BML_CHK_PARAMETER_VALIDATION)
    /* check the boundaries of input parameter*/
    if (nVpn > pstVol->nLastVpn)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] Vpn is bigger than LastVpn \r\n")));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVpn:%d(Invaild), n1stSecOff:%d, nNumOfScts:%d, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nVol, nVpn, n1stSecOff, nNumOfScts, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }

    /* check the boundaries of 1st sector offset and last sector offset*/
    if (n1stSecOff  >= (nMaxSctsPerVpg))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] 1st Sector Offset is greater than last sector offset \r\n")));    
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVpn:%d, n1stSecOff:%d(Invaild), nNumOfScts:%d, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nVol, nVpn, n1stSecOff, nNumOfScts, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }

    /* check the main and spare buffer pointer */
    if ((pMBuf == NULL) && (pSBuf == NULL))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR] main and spare buffer pointer is NULL \r\n")));    
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVpn:%d, n1stSecOff:%d, nNumOfScts:%d, pMBuf:0x%x, pSBuf:0x%x, nRe:FSR_BML_INVALID_PARAM) / %d line\r\n"),
                                        __FSR_FUNC__, nVol, nVpn, n1stSecOff, nNumOfScts, pMBuf, pSBuf, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_INVALID_PARAM));
        return FSR_BML_INVALID_PARAM;
    }
#endif /* BML_CHK_PARAMETER_VALIDATION */

    /* Set nSftSctsPerPg, nMaskSctsPerPg */
    nSftSctsPerPg  = _GetShfValue(nMaxSctsPerVpg);

    if (nMaxSctsPerVpg == 4)
    {
        nMaskSctsPerPg = 3;
    }
    else if (nMaxSctsPerVpg == 8)
    {
        nMaskSctsPerPg = 7;
    }
    else /* if (nMaxSctsPerVpg == 16)*/
    {
        nMaskSctsPerPg = 15;
    }

    /* Calculate nNumOfPgs and nLastSecOff*/
    nNumOfPgs   = (n1stSecOff + nNumOfScts) >> nSftSctsPerPg;
    nLastSecOff = (nMaxSctsPerVpg - (n1stSecOff + nNumOfScts)) & nMaskSctsPerPg;

    /* Update nNumOfPgs*/
    if (nLastSecOff)
    {
        nNumOfPgs++;
    }

    /* Set nFlag by the 1st sector offset and the last sector offset */
    nFlag   = FSR_BML_FLAG_MAKE_SCTOFFSET(n1stSecOff, nLastSecOff);

    nLockLayer = _GetLockLayer(nVol);

    /* Acquire semaphore */
    bRe = pstVol->AcquireLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ACQUIRE_SM_ERROR));
        nBMLRe = FSR_BML_ACQUIRE_SM_ERROR;
        return nBMLRe;
    }

    nBMLRe  = _BML_Read(nVol,
                        nVpn,
                        nNumOfPgs,
                        pMBuf,
                        pSBuf,
                        nFlag);
    if (nBMLRe != FSR_BML_SUCCESS)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nVpn:%d, n1stSecOff:%d, nNumOfScts:%d, nRe:0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, nVpn, n1stSecOff, nNumOfScts, nBMLRe));
    }

    /* Release semaphore */
    bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
    if (bRe == FALSE32)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        if (nBMLRe == FSR_BML_SUCCESS)
        {
           nBMLRe = FSR_BML_RELEASE_SM_ERROR;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nBMLRe));

    return nBMLRe;
}

/**
 *  @brief      This function reads virtual pages.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nVpn        : First virtual page number for reading
 *  @param [in]  nNumOfPgs   : The number of pages to read
 *  @param [in] *pMBuf       : Main page buffer for reading
 *  @param [in] *pSBuf       : Spare page buffer for reading
 *  @param [in]  nFlag       : FSR_BML_FLAG_ECC_ON
 *  @n                         FSR_BML_FLAG_ECC_OFF
 *  @n                         FSR_BML_FLAG_LSB_RECOVERY_LOAD
 *  @n                         FSR_BML_FLAG_USE_SPAREBUF
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_READ_ERROR
 *  @return     FSR_BML_CRITICAL_ERROR
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     FSR_BML_ACQUIRE_SM_ERROR
 *  @return     FSR_BML_RELEASE_SM_ERROR
 *  @return     FSR_BML_VOLUME_NOT_OPENED
 *  @return     Some LLD errors
 *
 *  @author     DongHoon Ham
 *  @version    1.1.0
 *
 */
PUBLIC INT32
FSR_BML_Read(UINT32        nVol,
             UINT32        nVpn,
             UINT32        nNumOfPgs,
             UINT8        *pMBuf,
             FSRSpareBuf  *pSBuf,
             UINT32        nFlag)
{
    UINT32        nSplitVpn;
    UINT32        nNumOfSplitPgs;
    UINT32        nRemainPgs;
    UINT32        nLockLayer;
    UINT8        *pSplitMBuf;
    FSRSpareBuf  *pSplitSBuf;
    BmlVolCxt    *pstVol;
    INT32         nBMLRe = FSR_BML_SUCCESS;
    INT32         nRe    = FSR_BML_SUCCESS;
    BOOL32        bRe;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nVpn: %d, nNumOfPgs: %d, nFlag: 0x%x)\r\n")
                                    , __FSR_FUNC__, nVol, nVpn, nNumOfPgs, nFlag));

    /* check volume range */
    CHK_VOL_RANGE(nVol);

    /* Get pointer to Volume context */
    pstVol = _GetVolCxt(nVol);

    /* Check the pointer to volume context */
    CHK_VOL_POINTER(pstVol);

    /* check volume open */
    CHK_VOL_OPEN(pstVol->bVolOpen);

    FSR_ASSERT(nVol < FSR_MAX_VOLS);

    nRemainPgs = nNumOfPgs;
    nSplitVpn  = nVpn;
    pSplitMBuf = pMBuf;
    pSplitSBuf = pSBuf;

    do
    {
        if (nRemainPgs > FSR_BML_PGS_PER_SM_CYCLE)
        {
            nNumOfSplitPgs = FSR_BML_PGS_PER_SM_CYCLE;
        }
        else
        {
            nNumOfSplitPgs = nRemainPgs;
        }

        nLockLayer = _GetLockLayer(nVol);

        /* Acquire semaphore */
        bRe = pstVol->AcquireLock(pstVol->nLockHandle, nLockLayer);
        if (bRe == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            nRe = FSR_BML_ACQUIRE_SM_ERROR;
            break;
        }

        nBMLRe = _BML_Read(nVol,
                          nSplitVpn,
                          nNumOfSplitPgs,
                          pSplitMBuf,
                          pSplitSBuf,
                          nFlag);
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            if (!((nRe == FSR_BML_2LV_READ_DISTURBANCE_ERROR) && 
                  (nBMLRe == FSR_BML_1LV_READ_DISTURBANCE_ERROR)))
            {
                nRe = nBMLRe;
            }

            if (!(nBMLRe & FSR_BML_1LV_READ_DISTURBANCE_ERROR) &&
                !(nBMLRe & FSR_BML_2LV_READ_DISTURBANCE_ERROR))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol:%d, nSplitVpn:%d, nNumOfSplitPgs:%d, nRe:0x%x)\r\n"),
                    __FSR_FUNC__, nVol, nSplitVpn, nNumOfSplitPgs, nBMLRe));

                /* Release semaphore */
                bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
                if (bRe == FALSE32)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                        __FSR_FUNC__, __LINE__));
                }
                break;
            }
        }

        /* Release semaphore */
        bRe = pstVol->ReleaseLock(pstVol->nLockHandle, nLockLayer);
        if (bRe == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            nRe = FSR_BML_RELEASE_SM_ERROR;
            break;
        }

        nSplitVpn  += nNumOfSplitPgs;

        if (pSplitMBuf != NULL)  /* If main buffer is NULL, buffer should be not moved. */
        {
            pSplitMBuf += (pstVol->nSizeOfVPage * nNumOfSplitPgs);
        }

        if (pSplitSBuf != NULL)
        {
            pSplitSBuf += nNumOfSplitPgs;
        }
        nRemainPgs -= nNumOfSplitPgs;

    } while (nRemainPgs > 0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, nRe));

    return nRe;
}

#if defined(FSR_BML_SUPPORT_SUSPEND_RESUME)
/*
 * @brief       This function acquires lock and performs flush op for entering the suspend state
 *
 * @param[in]   nVol    : Volume Number
 * @param[in]   nFlag   : Don't care( for future use )
 *
 * @return      FSR_BML_SUCCESS
 * @return      FSR_BML_ACQUIRE_SM_ERROR
 * @return      FSR_BML_FlushOp()'s Error Code
 *
 * @author      Byungyong Ahn
 * @version     1.2.1
 *
 */
PUBLIC INT32
FSR_BML_Suspend( UINT32 nVol, UINT32 nFlag )
{
    BmlVolCxt   *pstVol;
    INT32       nBMLRe  = FSR_BML_SUCCESS;

    FSR_STACK_VAR;
    FSR_STACK_END;  

    FSR_DBZ_DBGMOUT( FSR_DBZ_BML_IF, ( TEXT( "[BIF:IN ] ++%s(nVol: %d, nFlag: 0x%x)\r\n" ),
                                        __FSR_FUNC__, nVol, nFlag ) );

    /* Check Volume Range, whether BML Open or Not */
    CHK_VOL_RANGE( nVol );
    pstVol  = _GetVolCxt( nVol );
    CHK_VOL_POINTER( pstVol );
    CHK_VOL_OPEN( pstVol->bVolOpen );

    do
    {
        /* Acquire Lock */
        if ( pstVol->AcquireLock( pstVol->nLockHandle, _GetLockLayer( nVol ) ) == FALSE32 )
        {
            FSR_DBZ_RTLMOUT( FSR_DBZ_ERROR, ( TEXT( "[BIF:ERR]   %s(nVol: %d, nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n" ),
                                                __FSR_FUNC__, nVol, __LINE__ ) );
            nBMLRe  = FSR_BML_ACQUIRE_SM_ERROR;
            break;
        }

        /* Flush Op */
        nBMLRe  = FSR_BML_FlushOp( nVol, FSR_BML_FLAG_NO_SEMAPHORE );
        if ( nBMLRe != FSR_BML_SUCCESS )
        {
            FSR_DBZ_RTLMOUT( FSR_DBZ_ERROR, ( TEXT( "[BIF:   ]   %s(nVol: %d, nRe:0x%x)/ %d line\r\n" ),
                                                __FSR_FUNC__, nVol, nBMLRe, __LINE__ ) );
            break;
        }
    } while ( 0 );

    FSR_DBZ_DBGMOUT( FSR_DBZ_BML_IF, ( TEXT( "[BIF:OUT] --%s\r\n" ), __FSR_FUNC__ ) );

    return  nBMLRe;
} // FSR_BML_Suspend()

/*
 * @brief       This function invalidates LLD Read-Cache
 *              and restores Device Register/Block State for leaving the suspend state
 *
 * @param[in]   nVol    : Volume Number
 * @param[in]   nFlag   : FSR_BML_FLAG_NONE
 *                        FSR_BML_FLAG_RESTORE_BLOCK_STATE
 *
 * @return      FSR_BML_SUCCESS
 * @return      _ProtectLockedArea()'s Error Code
 * @return      FSR_BML_RELEASE_SM_ERROR
 * @return      FSR_BML_DEVICE_ACCESS_ERROR
 *
 * @author      Byungyong Ahn
 * @version     1.2.1
 *
 */
PUBLIC INT32
FSR_BML_Resume( UINT32 nVol, UINT32 nFlag )
{
    BmlVolCxt   *pstVol;
    INT32       nBMLRe  = FSR_BML_SUCCESS;
    UINT32      nDevIdx;
    UINT32      nPDev;
    INT32       nLLDRe;

    FSR_STACK_VAR;
    FSR_STACK_END;  

    FSR_DBZ_DBGMOUT( FSR_DBZ_BML_IF, ( TEXT( "[BIF:IN ] ++%s(nVol: %d, nFlag: 0x%x)\r\n" ),
                                        __FSR_FUNC__, nVol, nFlag ) );

    /* Check Volume Range, whether BML Open or Not */
    CHK_VOL_RANGE( nVol );
    pstVol  = _GetVolCxt( nVol );
    CHK_VOL_POINTER( pstVol );
    CHK_VOL_OPEN( pstVol->bVolOpen );

    do
    {
        /* Invalidate LLD Read-Cache, and Restore Device Registers */
        for ( nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx ++ )
        {
            nPDev   = nVol * DEVS_PER_VOL + nDevIdx;

            /* LLD invalidates LLD Read-Cache and restores Device Registers after Hot-Reset */
            nLLDRe  = pstVol->LLD_IOCtl( nPDev, FSR_LLD_IOCTL_HOT_RESET, NULL, 0, NULL, 0, NULL );
            if ( nLLDRe != FSR_LLD_SUCCESS )
            {
                FSR_DBZ_RTLMOUT( FSR_DBZ_ERROR, ( TEXT( "[BIF:   ]   %s(nVol: %d, nRe:0x%x)/ %d line\r\n" ),
                                                    __FSR_FUNC__, nVol, nLLDRe, __LINE__ ) );
                nBMLRe  = FSR_BML_DEVICE_ACCESS_ERROR;
                break;
            }
        }

        /* Restore Block State */
        if ( ( nFlag & FSR_BML_FLAG_RESTORE_BLOCK_STATE ) == FSR_BML_FLAG_RESTORE_BLOCK_STATE )
        {
            nBMLRe  = _ProtectLockedArea( nVol, pstVol->pstPartI );
            if ( nBMLRe != FSR_BML_SUCCESS )
            {
                FSR_DBZ_RTLMOUT( FSR_DBZ_ERROR, ( TEXT( "[BIF:   ]   %s(nVol: %d, nRe:0x%x)/ %d line\r\n" ),
                                                    __FSR_FUNC__, nVol, nBMLRe, __LINE__ ) );
                break;
            }
        }

        /* Release Lock */
        if ( pstVol->ReleaseLock( pstVol->nLockHandle, _GetLockLayer( nVol ) ) == FALSE32 )
        {
            FSR_DBZ_RTLMOUT( FSR_DBZ_ERROR, ( TEXT( "[BIF:ERR]   %s(nVol: %d, nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n" ),
                                                __FSR_FUNC__, nVol, __LINE__ ) );
            nBMLRe  = FSR_BML_RELEASE_SM_ERROR;
            break;
        }
    } while ( 0 );

    FSR_DBZ_DBGMOUT( FSR_DBZ_BML_IF, ( TEXT( "[BIF:OUT] --%s\r\n" ), __FSR_FUNC__ ) );

    return  nBMLRe;
} // FSR_BML_Resume()
#endif  // #if defined(FSR_BML_SUPPORT_SUSPEND_RESUME)
