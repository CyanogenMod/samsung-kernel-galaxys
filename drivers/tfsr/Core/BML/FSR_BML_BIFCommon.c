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
 *  @file       FSR_BML_BIFCommon.c
 *  @brief      This file consists of common FSR_BML functions
 *  @author     SuRuyn Lee
 *  @date       15-JAN-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  15-JAN-2007 [SuRyun Lee] : first writing
 *  @n  30-MAY-2007 [SuRyun Lee] : seperate original FSR_BML_Interfsce file
 *
 */

/**
 *  @brief  Header file inclusions
 */

#define  FSR_NO_INCLUDE_STL_HEADER
#include "FSR.h"

#include "FSR_BML_Config.h"
#include "FSR_BML_Types.h"
#include "FSR_BML_NonBlkMgr.h"
#include "FSR_BML_BIFCommon.h"
#include "FSR_BML_BBMCommon.h"
#include "FSR_BML_BadBlkMgr.h"
#include "FSR_BML_BBMMount.h"

/**
 *  @brief      Static variavles definition
 */
PRIVATE  BmlDevCxt       *gpstDevCxt[FSR_MAX_DEVS];
PRIVATE  BmlVolCxt        gstVolCxt [FSR_MAX_VOLS];
PRIVATE  FSRLowFuncTbl    gstLFT    [FSR_MAX_VOLS];

#if defined(WITH_TINY_FSR)
extern
#elif defined(TINY_FSR)
PUBLIC
#else
PRIVATE
#endif
BmlShCxt                  gstShCxt;


PRIVATE  BOOL32           gnPartAttrChg[FSR_MAX_VOLS][FSR_MAX_STL_PARTITIONS];

/**
 *  @brief  Code Implementation
 */
/**
 *  @brief      This function gets pointer to handle for attribute change of each partitions.
 *
 *  @param [in] nVol     : volume number
 *  @param [in] nPartID  : STL partition ID
 *
 *  @return     pointer to handle for attribute changing of partition
 *
 *  @author     DongHoon Ham
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32*
_GetPartAttrChgHdl(UINT32   nVol,
                   UINT32   nPartID)
{
    UINT32 nPartIdx;

    nPartIdx = nPartID - FSR_PARTID_STL0;
    return &gnPartAttrChg[nVol][nPartIdx];
}


/**
 *  @brief  Code Implementation
 */
/**
 *  @brief      This function gets pointer to VolCxt data structure.
 *
 *  @param [in] nVol     : volume number
 *
 *  @return     pointer to VolCxt data structure
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BmlVolCxt*
_GetVolCxt(UINT32   nVol)
{
    return &gstVolCxt[nVol];
}

/**
 *  @brief      This function gets the pointer to DevCxt data structure.
 *
 *  @param [in] nPDev     : physical device number
 *
 *  @return     pointer to DevCxt data structure
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BmlDevCxt*
_GetDevCxt(UINT32   nDev)
{
    return gpstDevCxt[nDev];
}

/**
 *  @brief      This function gets the pointer to Shared Cxt data structure.
 *
 *  @param [None] 
 *
 *  @return     pointer to SharedCxt data structure
 *
 *  @author     Kyungho Shin
 *  @version    1.2.0
 *
 */
PUBLIC BmlShCxt*
_GetShCxt(VOID)
{
    return &gstShCxt;
}

/**
 *  @brief      This function gets the pointer to FSRLowFuncTbl.
 *
 *  @param [in] nVol     : volume number
 *
 *  @return     pointer to FSRLowFuncTbl structure
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC FSRLowFuncTbl*
_GetLFT(UINT32  nVol)
{
    return &gstLFT[nVol];
}

/**
 *  @brief      This function allocate and initilaize memory for volume context.
 *
 *  @param [in] stPAM   : pointer to FsrVolParm structure
 *  @param [in] nFlag   : FSR_BML_FLAG_NONE
 *  @n                    FSR_BML_FORCED_INIT
 *
 *  @return             FSR_BML_SUCCESS
 *  @return             FSR_BML_OAM_ACCESS_ERROR
 *  @return             FSR_OAM_NOT_ALIGNED_MEMPTR
 *  @return             Some LLD return value
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_InitBIF(FsrVolParm stPAM[FSR_MAX_VOLS],
         UINT32     nFlag)
{
    UINT32            nVol  = 0;                    /* Volume number                        */
    UINT32            nLLDFlag  = FSR_LLD_FLAG_NONE;/* LLD Flag                             */
    UINT32            nTINYFlag = FSR_LLD_FLAG_NONE;/* LLD Flag for TINY FSR                */
    UINT32            nMemType;
    UINT32            nLockLayer;
    BOOL32            bRe       = TRUE32;
    INT32             nLLDRe    = FSR_LLD_SUCCESS;  /* FSR_LLD return value                 */
    FSRLowFuncTbl    *pstLFT[FSR_MAX_VOLS];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nFlag: 0x%x)\r\n"),__FSR_FUNC__));

    /* Memset the LLD Function Table */
    FSR_OAM_MEMSET(gstLFT, 0x00, sizeof(gstLFT));  

    pstLFT[0] = &gstLFT[0]; 
    pstLFT[1] = &gstLFT[1];
    
    /* Registering the LLD function into the LLD Function Table */
    FSR_PAM_RegLFT(pstLFT);

    /* Memset Partition Attribute Change Handle Array */
    FSR_OAM_MEMSET(gnPartAttrChg, 0x00, sizeof(gnPartAttrChg));

    /* Memset volume context */
    FSR_OAM_MEMSET(gstVolCxt, 0x00, sizeof(gstVolCxt));

    /* Memset pstDevCxt */
    FSR_OAM_MEMSET(gpstDevCxt, 0x00, sizeof(gpstDevCxt));

    for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
    {
        /* if the current volume has no NAND device, skip */
        if (stPAM[nVol].nDevsInVol == 0)
        {
            continue;
        }

        /* Get nNumOfDev */
        gstVolCxt[nVol].nNumOfDev   = stPAM[nVol].nDevsInVol;

        /* Set some parameters of volume context */
        gstVolCxt[nVol].bVolOpen        = FALSE32;
        gstVolCxt[nVol].nOpenCnt        = 0;
        gstVolCxt[nVol].bOTPEmul        = FALSE32;
        gstVolCxt[nVol].bOTPEnable      = FALSE32;
        
        nMemType = FSR_OAM_LOCAL_MEM;
        if (stPAM[nVol].bProcessorSynchronization == TRUE32)
        {
            nMemType = FSR_OAM_SHARED_MEM;
        }

        /* allocate shared memory */
        gstVolCxt[nVol].pbUseSharedMemory   = (BOOL32 *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                sizeof(BOOL32),
                                                                nMemType);
        gstVolCxt[nVol].pnSharedOpenCnt     = (UINT32 *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                sizeof(UINT32),
                                                                nMemType);
        if ((gstVolCxt[nVol].pbUseSharedMemory  == NULL) ||
            (gstVolCxt[nVol].pnSharedOpenCnt    == NULL))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pbUseSharedMemory, pnSharedOpenCnt) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: FSR_BML_OAM_ACCESS_ERROR)\r\n"), __FSR_FUNC__));
            return FSR_BML_OAM_ACCESS_ERROR;
        }
        /* Check mis-aligned pointer */
        if ((((UINT32)gstVolCxt[nVol].pbUseSharedMemory & 0x3)   != 0) ||
            (((UINT32)gstVolCxt[nVol].pnSharedOpenCnt & 0x3)     != 0))
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: FSR_OAM_NOT_ALIGNED_MEMPTR)\r\n"), __FSR_FUNC__));
            return FSR_OAM_NOT_ALIGNED_MEMPTR;
        }

        nLockLayer = _GetLockLayer(nVol);

        /* create lock */
        if (stPAM[nVol].bProcessorSynchronization == FALSE32)
        {
            bRe = FSR_OAM_CreateSM (&(gstVolCxt[nVol].nLockHandle), nLockLayer);
            if (bRe == FALSE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_CreateSM Error) / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }
            
            gstVolCxt[nVol].AcquireLock = FSR_OAM_AcquireSM;
            gstVolCxt[nVol].ReleaseLock = FSR_OAM_ReleaseSM;
        }
        else
        {
            bRe = FSR_PAM_CreateSL(&(gstVolCxt[nVol].nLockHandle), nLockLayer);
            if (bRe == FALSE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_PAM_CreateSM Error) / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_PAM_ACCESS_ERROR;
            }

            gstVolCxt[nVol].AcquireLock = FSR_PAM_AcquireSL;
            gstVolCxt[nVol].ReleaseLock = FSR_PAM_ReleaseSL;
        }

        /* Acquire lock */
        bRe = gstVolCxt[nVol].AcquireLock(gstVolCxt[nVol].nLockHandle, nLockLayer);
        if (bRe == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[_InitBIF:ERR]   %s(nRe: FSR_BML_ACQUIRE_SM_ERROR) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_ACQUIRE_SM_ERROR));
            return FSR_BML_ACQUIRE_SM_ERROR;
        }

        /* Get device parameter value */
        nLLDRe = pstLFT[nVol]->LLD_Init(nLLDFlag | nTINYFlag);
        if ((nLLDRe != FSR_LLD_SUCCESS) && (nLLDRe != FSR_LLD_ALREADY_INITIALIZED))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s / %d line \r\n"), __FSR_FUNC__, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Init(nFlag: 0x%x, nRe: 0x%x)\r\n"),
                                            nLLDFlag, nLLDRe));
            return nLLDRe;
        }

        /* Reset shared memory
           Case1) any process does not opened 
           Case2) the shared open count got to nSharedMemoryInitCycle 
           */
        if ((*(gstVolCxt[nVol].pnSharedOpenCnt) == 0 )||
            (*(gstVolCxt[nVol].pnSharedOpenCnt) == stPAM[nVol].nSharedMemoryInitCycle))
        {
            *(gstVolCxt[nVol].pbUseSharedMemory) = FALSE32;
        }

        /* release lock */
        bRe = gstVolCxt[nVol].ReleaseLock(gstVolCxt[nVol].nLockHandle, nLockLayer);
        if (bRe == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[_InitBIF:ERR]   %s(nRe: FSR_BML_RELEASE_SM_ERROR) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nRe: 0x%x)\r\n"),__FSR_FUNC__, FSR_BML_RELEASE_SM_ERROR));
            return FSR_BML_RELEASE_SM_ERROR;
        }

        FSR_OAM_SetMResetPoint(stPAM[nVol].nMemoryChunkID);

    } /* End of "for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)" */

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FSR_BML_SUCCESS;
}

/**
 *  @brief      This function checks whether the given device is valid or notexception.
 *
 *  @param [in]  nPDev     : physical device number
 *
 *  @return     TRUE32
 *  @return     FALSE32
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32
_IsOpenedDev(UINT32 nPDev)
{
    BmlDevCxt *pstDev;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nDev:%d)\r\n"), __FSR_FUNC__, nPDev));

    pstDev = _GetDevCxt(nPDev);

    if (pstDev == NULL)
    {
        return FALSE32;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return TRUE32;
}

/**
 *  @brief      This function gets partition ID of the current vun.
 *
 *  @param [in]  *pstVol  : pointer to VolCxt structure
 *  @param [in]   nVun    : virtual unit number
 *  @param [out] *pPartID : pointer to partition ID
 *
 *  @return     TRUE32
 *  @return     FALSE32
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32
_GetPartID(BmlVolCxt   *pstVol,
           UINT32       nVun,
           UINT32      *pPartID)
{
    UINT32         n1stVun      = 0;    /* the 1st Vun                      */
    UINT32         nLastVun     = 0;    /* the last Vun                     */
    UINT32         nNumOfPEntry = 0;    /* # of part entry                  */
    FSRPartEntry  *pstPEntry;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVun: %d, *pPartID: 0x%x)\r\n"),
                                    __FSR_FUNC__, nVun, *pPartID));

    /********************************************/
    /* Use a sequential search method           */
    /********************************************/
    /* Get pointer of PartEntry and nNumOfPartEntry */
    nNumOfPEntry     = pstVol->pstPartI->nNumOfPartEntry;
    pstPEntry        = pstVol->pstPartI->stPEntry + (nNumOfPEntry-1);

    while (nNumOfPEntry--)
    {
        n1stVun     = pstPEntry->n1stVun;
        nLastVun    = (n1stVun + pstPEntry->nNumOfUnits) -1;

        if ((n1stVun <= nVun) && (nLastVun >= nVun))
        {
            *pPartID = pstPEntry->nID;
            return TRUE32;
        }

        pstPEntry--;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FALSE32;
}

/**
 *  @brief      This function stores the return value for previous operation.
 *
 *  @param [in]  *pstVol    : pointer to VolCxt structure
 *  @param [in]  *pstDie    : pointer to DieCxt structure
 *  @param [in]   nBMLRe    : BML Return value for previous partition
 *  @param [in]   nCurPartID: patition ID for current partition
 *
 *  @return     TRUE32
 *  @return     FALSE32
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32
_StorePrevPIRet(BmlVolCxt  *pstVol,
                BmlDieCxt  *pstDie,
                INT32       nBMLRe,
                UINT32      nCurPartID)
{
    UINT32      nID         = FSR_PARTID_BML0;  /* Index for partition ID               */
    UINT32      nPrevPartID = FSR_PARTID_BML0;  /* Partition ID for previous operation  */
    UINT32      nNumOfPEntry= 0;                /* # of partition entries               */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nBMLRe: 0x%x, nCurPartID: 0x%x)\r\n"),
                                    __FSR_FUNC__, nBMLRe, nCurPartID));

    /* Get the previous partition ID*/
    if (pstDie->nPrevPartID == 0xffff)
    {
        nPrevPartID = nCurPartID;
    }
    else
    {
        nPrevPartID = pstDie->nPrevPartID;
    }

    nNumOfPEntry = pstVol->pstPartI->nNumOfPartEntry;

    /* Store BML return value of previous operation */
    while (nNumOfPEntry--)
    {
        nID = pstVol->pstPartI->stPEntry[nNumOfPEntry].nID;
        if (nID == nPrevPartID)
        {
            pstDie->pnRetOfPartI[nNumOfPEntry] = nBMLRe;
            return TRUE32;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FALSE32;
}

/**
 *  @brief      This function gets return value for current partition.
 *
 *  @param [in]  *pstVol    : pointer to VolCxt structure
 *  @param [in]  *pstDie    : pointer to DieCxt structure
 *  @param [in]   nVun      : Virtual Unit Number
 *  @param [in]   nBMLRe    : BML return value
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_CANT_GET_RETURN_VALUE
 *  @return     FSR_BML_CANT_FIND_PART_ID
 *  @return     some BML errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_GetPIRet(BmlVolCxt *pstVol,
          BmlDieCxt *pstDie,
          UINT32     nVun,
          INT32      nBMLRe)
{
    UINT32      nID         = FSR_PARTID_BML0;  /* Index for partition ID               */
    UINT32      nCurPartID  = FSR_PARTID_BML0;  /* Partition ID for current opearation  */
    UINT32      nNumOfPEntry= 0;                /* # of partition entries               */
    BOOL32      bRet        = TRUE32;           /* Temporary return value               */
    INT32       nRe         = FSR_BML_CANT_GET_RETURN_VALUE;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVun: %d, nBMLRe: 0x%x)\r\n"),
                                    __FSR_FUNC__, nVun, nBMLRe));

    /********************************************************/
    /* <Get return value by partition >                     */
    /* STEP1. Get the current partition ID                  */
    /* STEP1. Store the return value for previous partition */
    /* STEP3. Get the return value for current partition    */
    /********************************************************/

    /* STEP1. Get the current partition ID */
    bRet = _GetPartID(pstVol, nVun, &nCurPartID);
    if (bRet != TRUE32)
    {
        /* message out */
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s / %d line \r\n"), __FSR_FUNC__, __LINE__));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   _GetPartID(nVun:%d, nRe:FSR_BML_CANT_FIND_PART_ID)\r\n"), nVun));
        return FSR_BML_CANT_FIND_PART_ID;
    }

    /* STEP2. Store the return value for previous partition */
    bRet = _StorePrevPIRet(pstVol, pstDie, nBMLRe, nCurPartID);
    if (bRet != TRUE32)
    {
        /* message out */
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s / %d line \r\n"), __FSR_FUNC__, __LINE__));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   _StorePrevPIRet(nCurPartID: %d, nRe: FSR_BML_CANT_FIND_PART_ID)\r\n"),
                                        nCurPartID));
        return FSR_BML_CANT_FIND_PART_ID;
    }

    nNumOfPEntry = pstVol->pstPartI->nNumOfPartEntry;

    /* STEP3. Get the return value for current partition */
    while (nNumOfPEntry--)
    {
        nID = pstVol->pstPartI->stPEntry[nNumOfPEntry].nID;
        if (nID == nCurPartID)
        {
            nRe = pstDie->pnRetOfPartI[nNumOfPEntry];
            return nRe;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return nRe;
}

/**
 *  @brief      This function checks whether LLDSpec is valid or not.
 *
 *  @param [in] *pstVol      : pointer to VolCxt structure
 *  @param [in] *pstLLDSpec  : pointer of LLDSpec structure
 *  @n                        the specification of NAND device
 *
 *  @return     TRUE32
 *  @return     FALSE32
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32
_ChkLLDSpecValidity(BmlVolCxt   *pstVol,
                    FSRDevSpec  *pstLLDSpec)
{
    UINT32      nDieIdx     = 0;        /* Die Index                */
    UINT16      nBlksInRsv  = 0;        /* # of blocks in reservoir */
    BOOL32      bRe         = TRUE32;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s\r\n"), __FSR_FUNC__));
  
    do {
        /* if nNANDType is not a FSR_LLD_MLC_NAND, 
         * # of blocks for SLC area(nBlksForSLCArea), # of pages per SLC block(nPgsPerBlkForSLC)
         * , # of reserved block for SLC(except FSR_LLD_FLEX_ONENAND) should not be zero  */
        if (pstLLDSpec->nNANDType != FSR_LLD_MLC_NAND)
        {
            for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
            {
                /* Check # of blocks in SLC Area */
                if (pstLLDSpec->nBlksForSLCArea[nDieIdx] == 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nBlksForSLCArea(%d) is out of bound) / %d line\r\n"),
                                                    __FSR_FUNC__, pstLLDSpec->nBlksForSLCArea, __LINE__));
                    bRe = FALSE32;
                    break;
                }
            }

            /* error case of # of blocks in SLC Area */
            if (bRe == FALSE32)
            {
                break;
            }

            /* check pages per SLC block validity */
            if (pstLLDSpec->nPgsPerBlkForSLC != 64)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nPgsPerBlkForSLC(%d) is invalid) / %d line\r\n"),
                                                __FSR_FUNC__, pstLLDSpec->nPgsPerBlkForSLC, __LINE__));
                bRe = FALSE32;
                break;
            }
        }

        /* if nNANDType is a FSR_LLD_MLC_NAND,
         * nPgsPerBlkForMLC and nNumOfRsvrInMLC should not be zero. */
        if (pstLLDSpec->nNANDType == FSR_LLD_MLC_NAND)
        {
            /* check pages per MLC block validity */
            if (pstLLDSpec->nPgsPerBlkForMLC != 128)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nPgsPerBlkForMLC(%d) is invalid) / %d line\r\n"),
                                                __FSR_FUNC__, pstLLDSpec->nPgsPerBlkForMLC, __LINE__));
                bRe = FALSE32;
                break;
            }
        }

        /* Check sectors per Page validity.
         * only support 2kpage, 4kpage. */
        if ((pstLLDSpec->nSctsPerPG != 4) &&
            (pstLLDSpec->nSctsPerPG != 8))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nSctsPerPG(%d) is invalid) / %d line\r\n"),
                                             __FSR_FUNC__, pstLLDSpec->nSctsPerPG, __LINE__));
            bRe = FALSE32;
            break;
        }

        /* Check # of blocks of Reservoir size limitation */
        nBlksInRsv = pstLLDSpec->nRsvBlksInDev;
        if ((pstLLDSpec->nNumOfBlks / 4) <= nBlksInRsv)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s (pstLLDSpec->nNumOfBlks / 4)(= %d) <= pstLLDSpec->nBlksInRsv(%d) / %d line\r\n"),
                                            __FSR_FUNC__, pstLLDSpec->nNumOfBlks / 4, nBlksInRsv, __LINE__));
            bRe = FALSE32;
            break;
        }

        /* Check # of blocks of Reservoir validity */
        if ((nBlksInRsv > BML_MAX_ERL_ITEM) || (nBlksInRsv == 0))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nBlksInRsv(%d) is out of bound) / %d line\r\n"),
                                            __FSR_FUNC__, nBlksInRsv, __LINE__));
            bRe = FALSE32;
            break;
        }

        /* Check # of blocks validity */
        if (pstLLDSpec->nNumOfBlks == 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nNumOfBlks(%d) is out of bound) / %d line\r\n"),
                                            __FSR_FUNC__, pstLLDSpec->nNumOfBlks, __LINE__));
            bRe = FALSE32;
            break;
        }

        /* Check # of dies in a block */
        if ((pstLLDSpec->nNumOfDies != 1) &&
            (pstLLDSpec->nNumOfDies != 2))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nNumOfDies(%d) is invalid) / %d line\r\n"),
                                            __FSR_FUNC__, pstLLDSpec->nNumOfDies, __LINE__));
            bRe = FALSE32;
            break;
        }

        /* Check # of planes (# of planes = 1 or 2)*/
        if ((pstLLDSpec->nNumOfPlanes != 1)
            && (pstLLDSpec->nNumOfPlanes != 2))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(pstLLDSpec->nNumOfPlanes(%d) is invalid) / %d line\r\n"),
                                            __FSR_FUNC__, pstLLDSpec->nNumOfPlanes, __LINE__));
            bRe = FALSE32;
            break;
        }

    } while(0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return bRe;
}

/**
 *  @brief      This function initializes gstLFT (LLD Function Table).
 *
 *  @param [in]   nVol       : volume number
 *
 *  @return     TRUE32
 *  @return     FALSE32
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC BOOL32
_InitLFT(UINT32 nVol)
{
    UINT32           nIdx   = 0;    /* Temporary Index  */
    UINT32          *pBuf;
    BmlVolCxt       *pstVol;
    FSRLowFuncTbl   *pstLFT[FSR_MAX_VOLS];
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer of volume context */
    pstVol    = _GetVolCxt(nVol);

    pstLFT[0] = &gstLFT[0];
    pstLFT[1] = &gstLFT[1];

    /* Registering the LLD function into the LLD Function Table */
    FSR_PAM_RegLFT(pstLFT);

    pstVol->LLD_Init                = pstLFT[nVol]->LLD_Init;
    pstVol->LLD_Open                = pstLFT[nVol]->LLD_Open;
    pstVol->LLD_Close               = pstLFT[nVol]->LLD_Close;
    pstVol->LLD_Read                = pstLFT[nVol]->LLD_ReadOptimal;
    pstVol->LLD_Write               = pstLFT[nVol]->LLD_Write;
    pstVol->LLD_CopyBack            = pstLFT[nVol]->LLD_CopyBack;
    pstVol->LLD_Erase               = pstLFT[nVol]->LLD_Erase;
    pstVol->LLD_GetDevSpec          = pstLFT[nVol]->LLD_GetDevSpec;
    pstVol->LLD_ChkBadBlk           = pstLFT[nVol]->LLD_ChkBadBlk;
    pstVol->LLD_FlushOp             = pstLFT[nVol]->LLD_FlushOp;
    pstVol->LLD_GetPrevOpData       = pstLFT[nVol]->LLD_GetPrevOpData;
    pstVol->LLD_IOCtl               = pstLFT[nVol]->LLD_IOCtl;
    pstVol->LLD_InitLLDStat         = pstLFT[nVol]->LLD_InitLLDStat;
    pstVol->LLD_GetStat             = pstLFT[nVol]->LLD_GetStat;
    pstVol->LLD_GetBlockInfo        = pstLFT[nVol]->LLD_GetBlockInfo;
    pstVol->LLD_GetNANDCtrllerInfo  = pstLFT[nVol]->LLD_GetNANDCtrllerInfo;

    /* --------------------------------------------------------------------
     * LFT Link completion check
     * --------------------------------------------------------------------
     * Check whether low level function table(gstLFT) is registered or not
     * if gstLFT is not registered, it is CRITICAL ERROR.
     * --------------------------------------------------------------------
     */
    pBuf = (UINT32 *) pstLFT[nVol];

    for (nIdx = 0; nIdx < (sizeof(FSRLowFuncTbl) / sizeof(UINT32)); nIdx++)
    {
        if (pBuf[nIdx] == 0x00000000)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(LLD(%dth) is not registered) / %d line\r\n"),
                                            __FSR_FUNC__, nIdx, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));
            return FALSE32;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return TRUE32;
}


/**
 *  @brief      This function opens the given volume.
 *
 *  @param [in]  nVol       : volume number
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_PAM_ACCESS_ERROR
 *  @return     FSR_BML_OAM_ACCESS_ERROR
 *  @return     FSR_BML_DEVICE_ACCESS_ERROR
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_Open(UINT32    nVol)
{
    UINT32        nDevIdx           = 0;    /* Device Index                 */
    UINT32        nIdx              = 0;    /* Temporary Index              */
    UINT32        nMemType  = FSR_OAM_LOCAL_MEM;
    BOOL32        bRe       = TRUE32;           /* Temporary return value       */
    INT32         nPAMRe    = FSR_PAM_SUCCESS;  /* PAM return value             */
    INT32         nBMLRe    = FSR_BML_SUCCESS;  /* BML Return value             */

    BmlVolCxt    *pstVol;
    FsrVolParm    stPAM[FSR_MAX_VOLS];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* ---------+----------------------------
     * Volume 0 | PDev0, PDev1, PDev2, PDev3
     * Volume 1 | PDev4, PDev5, PDev6, PDev7
     * ---------+----------------------------
     */

    /* Initialize nNumOfDev, nNumOfUsBlks, nCurGroupID */
    pstVol->nNumOfDev    = 0;
    pstVol->nNumOfUsBlks = 0;

    /* Set the bNonBlkMode */
    pstVol->bNonBlkMode = FALSE32;

    nPAMRe  = FSR_PAM_GetPAParm(stPAM);
    if (nPAMRe != FSR_PAM_SUCCESS)
    {
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    /* This code prevents from opening the invalid volume. */
    if (stPAM[nVol].nDevsInVol == 0)
    {
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    /* Registering the LLD function into the LLD Function Table */
    bRe = _InitLFT(nVol);
    if (bRe == FALSE32)
    {
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_BML_PAM_ACCESS_ERROR));
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    /* Check Interrupt ID (ADD 2007/07/24) */
    for (nDevIdx = 0; nDevIdx < (FSR_MAX_DEVS/FSR_MAX_VOLS-1); nDevIdx++)
    {
        if ((stPAM[nVol].nIntID[nDevIdx] > BML_MAX_INT_ID)                  &&
            ((stPAM[nVol].nIntID[nDevIdx] & FSR_INT_ID_NONE) != FSR_INT_ID_NONE))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(Invaild interrupt ID: 0x%x) / %d line \r\n"),
                                            __FSR_FUNC__, stPAM[nVol].nIntID[nDevIdx], __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_PAM_ACCESS_ERROR));
            return FSR_BML_PAM_ACCESS_ERROR;
        }

        for (nIdx = (nDevIdx+1); nIdx < FSR_MAX_DEVS/FSR_MAX_VOLS; nIdx++)
        {
            if ((stPAM[nVol].nIntID[nDevIdx] != FSR_INT_ID_NONE)        &&
                (stPAM[nVol].nIntID[nDevIdx] == stPAM[nVol].nIntID[nIdx]))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(Invaild interrupt ID: 0x%x) / %d line \r\n"),
                                                __FSR_FUNC__, stPAM[nVol].nIntID[nDevIdx], __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_PAM_ACCESS_ERROR));
                return FSR_BML_PAM_ACCESS_ERROR;
            }
        }
    }

    /* Get nNumOfDev */
    pstVol->nNumOfDev     = stPAM[nVol].nDevsInVol;

    /* Set nMemType for the memory allocation */
    nMemType = FSR_OAM_LOCAL_MEM;
    if (stPAM[nVol].bProcessorSynchronization == TRUE32)
    {
        if (*(pstVol->pbUseSharedMemory))
        {
            nMemType = FSR_OAM_LOCAL_MEM;
        }
        else
        {
            nMemType = FSR_OAM_SHARED_MEM;
        }
    }
    else
    {
        nMemType = FSR_OAM_LOCAL_MEM;
    }

    /* Allocate memory (shared + local) */
    nBMLRe = _AllocMem(nVol, stPAM, nMemType);

    if (nBMLRe != FSR_BML_SUCCESS)
    {
        return nBMLRe;
    }

    /*
     * This routine is used only for dual core system
     */
    if (stPAM[nVol].bProcessorSynchronization == TRUE32)
    {
        if (*(pstVol->pbUseSharedMemory))
        {
            /* Memory free in the local memory */
            _FreeLocalMem(nVol, &stPAM[nVol]);

            /* Memory allocation in the shared memory */
            nBMLRe = _AllocSharedMem(nVol, &stPAM[nVol]);
        }

        /*
         * if _Open() is called, pstVol->pbUseSharedMemory is set to TRUE32
         * and pstVol->pnSharedOpenCnt increses in number.
         * _Close changes these values. FSR calls _Open() and _Close()
         **/
        *(pstVol->pbUseSharedMemory) = TRUE32;
        *(pstVol->pnSharedOpenCnt)  = *(pstVol->pnSharedOpenCnt) + 1;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return nBMLRe;
}

/**
 *  @brief      This function allocate memory for _Open.
 *
 *  @param [in]  nVol       : volume number
 *  @param [in]  stPAM      : FsrVolParm strucrture
 *  @param [in]  nMemType   : FSR_OAM_LOCAL_MEM
 *  @n                        FSR_OAM_SHARED_MEM
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_PAM_ACCESS_ERROR
 *  @return     FSR_BML_OAM_ACCESS_ERROR
 *  @return     FSR_BML_DEVICE_ACCESS_ERROR
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_AllocMem(UINT32        nVol,
          FsrVolParm    stPAM[FSR_MAX_VOLS],
          UINT32        nMemType)
{
    UINT32        nDevIdx           = 0;    /* Device Index                 */
    UINT32        nDieIdx           = 0;    /* Die Index                    */
    UINT32        nPDev             = 0;    /* Physical device number       */
    UINT32        nIdx              = 0;    /* Temporary Index              */
    UINT32        nNumOfRsvrBlks    = 0;    /* number of blocks in reservoir*/
    UINT32        nMallocSize       = 0;    /* Malloc size for Bad Unit Map */
    UINT32        nNumOfBMFs        = 0;    /* # of BMF(Block Map Field)    */
    UINT32        nNumOfCandidates  = 0;    /* # of candidates(RCB)         */
    UINT32        nSizeOfMaxRsvr    = 0;    /* size of maximum reservoir    */
    UINT32        nSizeOfBABitMap   = 0;    /* size of BAD Bit Map          */
    UINT32        nTINYFlag = FSR_LLD_FLAG_NONE;/* LLD flag for TINY FSR    */ 
    UINT32       *pRetPartI;
    BOOL32        bRe       = TRUE32;           /* Temporary return value       */
    INT32         nLLDRe    = FSR_OAM_SUCCESS;  /* LLD Return value             */
    BmlVolCxt    *pstVol;
    BmlDevCxt    *pstDev;
    BmlDieCxt    *pstDie;
    FSRDevSpec    stLLDSpec;
    BmlPreOpLog   *pstLog;
    BmlReservoir  *pstRsv;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer to VolCxt structure */
    pstVol = _GetVolCxt(nVol);

    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        nLLDRe = pstVol->LLD_Open(nPDev,
                                 &(stPAM[nVol]),
                                  FSR_LLD_FLAG_NONE | nTINYFlag);

        if (nLLDRe != FSR_LLD_SUCCESS && nLLDRe != FSR_LLD_ALREADY_OPEN)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d) / %d line \r\n"), __FSR_FUNC__, nVol, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Open(PDev: %d, nLLDRe: 0x%x)\r\n"),
                                            nPDev, nLLDRe));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT]  --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_DEVICE_ACCESS_ERROR));
            /* Return nLLDRe directly */
            return nLLDRe;
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT]   LLD_Open(PDev:%d) is completed\r\n"), nPDev));

        /* get NAND device specification using LLD_GetDevInfo() */
        nLLDRe = pstVol->LLD_GetDevSpec(nPDev,
                                        &stLLDSpec,
                                        FSR_LLD_FLAG_NONE | nTINYFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d) / %d line \r\n"), __FSR_FUNC__, nVol, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_GetDevSpec(PDev: %d, nLLDRe: 0x%x)\r\n"),
                                            nPDev, nLLDRe));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT]  --%s(nVol: %d nRe: 0x%x)\r\n"), __FSR_FUNC__, nVol, FSR_BML_DEVICE_ACCESS_ERROR));
            /* Return nLLDRe directly */
            return nLLDRe;
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT]   LLD_GetDevInfo(PDev: %d) is completed\r\n"), nPDev));


        /* check the validity of LLDSpec */
        bRe = _ChkLLDSpecValidity(pstVol, &stLLDSpec);
        if (bRe == FALSE32)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_DEVICE_ACCESS_ERROR));
            /* Return nLLDRe directly */
            return FSR_BML_DEVICE_ACCESS_ERROR;
        }

        /* Set the value of volume context */
        pstVol->nNumOfBlksInDev      = stLLDSpec.nNumOfBlks;
        pstVol->nNumOfPlane          = stLLDSpec.nNumOfPlanes;
        pstVol->nSctsPerPg           = stLLDSpec.nSctsPerPG;
        pstVol->nNumOfPgsInSLCBlk    = stLLDSpec.nPgsPerBlkForSLC;
        pstVol->nNumOfPgsInMLCBlk    = stLLDSpec.nPgsPerBlkForMLC;
        pstVol->nNumOfDieInDev       = stLLDSpec.nNumOfDies;
        pstVol->nNumOfRsvrBlks       = stLLDSpec.nRsvBlksInDev;
        pstVol->pPairedPgMap         = stLLDSpec.pPairedPgMap;
        pstVol->pLSBPgMap            = stLLDSpec.pLSBPgMap;
        pstVol->nNANDType            = stLLDSpec.nNANDType;
        pstVol->bCachedProgram       = stLLDSpec.bCachePgm;
        pstVol->b1stBlkOTP           = stLLDSpec.b1stBlkOTP;
        pstVol->nSLCTLoadTime        = stLLDSpec.nSLCTLoadTime;
        pstVol->nMLCTLoadTime        = stLLDSpec.nMLCTLoadTime;
        pstVol->nSLCTProgTime        = stLLDSpec.nSLCTProgTime;
        pstVol->nMLCTProgTime[0]     = stLLDSpec.nMLCTProgTime[0];
        pstVol->nMLCTProgTime[1]     = stLLDSpec.nMLCTProgTime[1];
        pstVol->nTEraseTime          = stLLDSpec.nTEraseTime;
        pstVol->nWrTranferTime       = stLLDSpec.nWrTranferTime;
        pstVol->nRdTranferTime       = stLLDSpec.nRdTranferTime;

        /* Add nPgBufToDataRAMTime  2007/07/02 */
        pstVol->nPgBufToDataRAMTime  = stLLDSpec.nPgBufToDataRAMTime;

        /* Add size of spare page. 2007/04/30 */
        pstVol->nSparePerSct         = stLLDSpec.nSparePerSct;

        /* Add P/E Cycle for SLC & MLC block 2007/05/02 */
        pstVol->nSLCPECycle          = stLLDSpec.nSLCPECycle;
        pstVol->nMLCPECycle          = stLLDSpec.nMLCPECycle;

        /* ADD nDID, nUID (2007/07/25) */
        pstVol->nDID                 = stLLDSpec.nDID;
        FSR_OAM_MEMCPY(pstVol->nUID, stLLDSpec.nUID, FSR_LLD_UID_SIZE);

        /* ADD nUserOTPScts (2007/08/04) */
        pstVol->nUserOTPScts         = stLLDSpec.nUserOTPScts;

        /* Set the value of volume context for Bad Unit Map */
        pstVol->nSftDDP              =  _GetShfValue(pstVol->nNumOfDieInDev);
        pstVol->nNumOfBlksInDie      =  pstVol->nNumOfBlksInDev >> pstVol->nSftDDP;

        /* Check the error */
        /* if value is odd number, error is generated */
        if (pstVol->nNumOfRsvrBlks & 0x1)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nNumOfRsvrBlks is odd(nNumOfRsvrBlks: %d)) / %d line\r\n"),
                                            __FSR_FUNC__, pstVol->nNumOfRsvrBlks, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_DEVICE_ACCESS_ERROR));
            return FSR_BML_DEVICE_ACCESS_ERROR;
        }

        /* create Device Context */
        gpstDevCxt[nPDev] = (BmlDevCxt *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                            sizeof(BmlDevCxt),
                                                            FSR_OAM_LOCAL_MEM);
        if (gpstDevCxt[nPDev] == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDev[%d]) / %d line\r\n"),
                                            __FSR_FUNC__, nPDev, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
            return FSR_BML_OAM_ACCESS_ERROR;
        }

        /* Check mis-aligned pointer */
        if (((UINT32)gpstDevCxt[nPDev] & 0x3) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDev[%d]) / %d line\r\n"),
                                            __FSR_FUNC__, nPDev, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
            return FSR_OAM_NOT_ALIGNED_MEMPTR;
        }

        /* Get the pointer of device context */
        pstDev            =  gpstDevCxt[nPDev];

        /* Get a device number*/
        pstDev->nDevNo     =  (UINT16) nPDev;

        /* Initialize the elements of device context */
        pstDev->nOTPStatus = 0;

        for(nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            /* create Die Context */
            pstDev->pstDie[nDieIdx] = (BmlDieCxt *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                      sizeof(BmlDieCxt),
                                                                      FSR_OAM_LOCAL_MEM);
            if (pstDev->pstDie[nDieIdx] == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie[%d]) / %d line\r\n"),
                                                 __FSR_FUNC__, nDieIdx, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDev->pstDie[nDieIdx]) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie[%d]) / %d line\r\n"),
                                                 __FSR_FUNC__, nDieIdx, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* Get a pointer of die context and
             * Initialize the nNumOfLLDOp
             */
            pstDie = pstDev->pstDie[nDieIdx];

            /* Initialize the nCntRDErr */
            pstDie->nCntRDErr  = 0;

            /* Initialize the nPrevPartID */
            pstDie->nPrevPartID = 0xffff;

            /* Initialize the main and spare buffer pointer */
            pstDie->pMBuf = NULL;
            pstDie->pSBuf = NULL;

            /* allocate main buffer memory and spare buffer memory */
            pstDie->pMBuf   = (UINT8 *)       FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                pstVol->nSctsPerPg * FSR_SECTOR_SIZE * pstVol->nNumOfPlane,
                                                                nMemType);
            pstDie->pSBuf   = (FSRSpareBuf *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                sizeof(FSRSpareBuf),
                                                                FSR_OAM_LOCAL_MEM);
            if ((pstDie->pMBuf == NULL) || (pstDie->pSBuf == NULL))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if ((((UINT32)(pstDie->pMBuf) & 0x3) != 0) || (((UINT32)(pstDie->pSBuf) & 0x3) != 0))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate spare extension buffer memory */
            pstDie->pSBuf->pstSpareBufBase = (FSRSpareBufBase *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                                sizeof(FSRSpareBufBase),
                                                                                nMemType);
            if (pstDie->pSBuf->pstSpareBufBase == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pSBuf->pstSpareBufBase) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }


            /* allocate spare extension buffer memory */
            pstDie->pSBuf->pstSTLMetaExt = (FSRSpareBufExt *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                                sizeof(FSRSpareBufExt) * FSR_MAX_SPARE_BUF_EXT,
                                                                                nMemType);
            if (pstDie->pSBuf->pstSTLMetaExt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pSBuf->pstSTLMetaExt) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* Memset pMBuf and pSBuf*/
            FSR_OAM_MEMSET(pstDie->pMBuf, 0xFF, sizeof(pstVol->nSctsPerPg * FSR_SECTOR_SIZE * pstVol->nNumOfPlane));
            FSR_OAM_MEMSET(pstDie->pSBuf->pstSpareBufBase, 0xFF, FSR_SPARE_BUF_BASE_SIZE);

            pstDie->pSBuf->nNumOfMetaExt    = FSR_MAX_SPARE_BUF_EXT;

            /* allocate pnRetOfPartI */
            pstDie->pnRetOfPartI = (UINT32 *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                sizeof(UINT32) * FSR_BML_MAX_PARTENTRY,
                                                                nMemType);
            if (pstDie->pnRetOfPartI == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie->pnRetOfPartI) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pnRetOfPartI) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie->pnRetOfPartI) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            pRetPartI = (UINT32 *)pstDie->pnRetOfPartI;
            /* Initialize the nRetOfPartI[]*/
            for (nIdx = 0; nIdx < FSR_BML_MAX_PARTENTRY; nIdx++)
            {
                *pRetPartI = FSR_BML_SUCCESS;
                pRetPartI++;
            }

            /* Initialize the PreOpLog */
            for (nIdx = 0; nIdx < BML_NUM_OF_PREOPLOG; nIdx++)
            {
                if (nIdx == 0)
                {
                    pstDie->pstPreOp     = (BmlPreOpLog *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                             sizeof(BmlPreOpLog),
                                                                             nMemType);
                    pstLog               = pstDie->pstPreOp;
                }
                else
                {
                    pstDie->pstNextPreOp = (BmlPreOpLog *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                             sizeof(BmlPreOpLog),
                                                                             nMemType);
                    pstLog               = pstDie->pstNextPreOp;
                }

                if (pstLog == NULL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:PreOpLog memory) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                    return FSR_BML_OAM_ACCESS_ERROR;
                }

                if (((UINT32)pstLog & 0x3) != 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:PreOpLog memory) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                    return FSR_OAM_NOT_ALIGNED_MEMPTR;
                }

                pstLog->nOpType   = BML_PRELOG_NOP;
                pstLog->nPgOffset = 0;
                pstLog->nSbn      = 0;
                pstLog->nFlag     = FSR_LLD_FLAG_NONE;
            }

            /* allocate the previous & next previous buffer */
            pstDie->pPreOpMBuf      = pstDie->pMBuf;
            pstDie->pNextPreOpMBuf  = pstDie->pMBuf;
            pstDie->pPreOpSBuf      = pstDie->pSBuf;
            pstDie->pNextPreOpSBuf  = pstDie->pSBuf;

            /* 
             * Flex-OneNAND has reservoirs for each die
             * SLC, MLC and OneNAND has only one reservoir in last die
             */

            /* allocate memory for reservoir */
            pstDie->pstRsv = (BmlReservoir *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                sizeof(BmlReservoir),
                                                                FSR_OAM_LOCAL_MEM);
            if (pstDie->pstRsv == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:Reservoir structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pstRsv) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:Reservoir structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate memory for reservoir of shared memory */
            pstDie->pstRsvSh = (BmlReservoirSh *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                    sizeof(BmlReservoirSh),
                                                                    nMemType);
            if (pstDie->pstRsvSh == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:ReservoirSh structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pstRsvSh) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:ReservoirSh structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* Get pointer to reservoir structure */
            pstRsv =pstDie->pstRsv;

            pstRsv->nNumOfRsvrInSLC = 0;
            pstRsv->nNumOfRsvrInMLC = 0;

            /*
             * Set number of bad blocks in a die 
             * In case of MLC only device, nNumOfRsvrInSLC = 0
             * In case of SLC only device, nNumOfRsvrInMLC = 0
             */
            nNumOfRsvrBlks = pstVol->nNumOfRsvrBlks / pstVol->nNumOfDieInDev;

            nNumOfBMFs = nNumOfRsvrBlks;

            if (pstVol->nNumOfPlane > 1)
            {
                /* 
                 * If max number of bad blocks of the device is x,
                 * number of reservoir candidate blocks is (x / 2)
                 */
                nNumOfCandidates = (nNumOfRsvrBlks / 2);

                /* 
                 * If max number of bad blocks of the device is x,
                 * number of BMF entries is (x + x / 2)
                 */
                nNumOfBMFs  += (nNumOfRsvrBlks / 2);
            }
            
            /* allocated memory size should be aligned by 4 */
            if ((nNumOfBMFs & 0x3) != 0)
            {
                nNumOfBMFs = ((nNumOfBMFs >> 0x2) + 1) << 0x2;
            }

            pstRsv->pstBMI = (BmlBMI *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                          sizeof(BmlBMI),
                                                          nMemType);
            if (pstRsv->pstBMI == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMI) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            if (((UINT32)(pstRsv->pstBMI) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMI) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate memory for bad block mapping field */
            pstRsv->pstBMF = (BmlBMF *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                          nNumOfBMFs * sizeof(BmlBMF),
                                                          nMemType);
            if (pstRsv->pstBMF == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMF) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pstBMF) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMF) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* This field is only used for multi plane device */
            if (pstVol->nNumOfPlane > 1)
            {
                /* allocated memory size should be aligned by 4 */
                if ((nNumOfCandidates & 0x3) != 0)
                {
                    nNumOfCandidates = ((nNumOfCandidates >> 0x2) + 1) << 0x2;
                }

                /* allocate memory for reservoir candidate block field */
                pstRsv->pstRCB = (UINT16 *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                              nNumOfCandidates * sizeof(UINT16),
                                                              nMemType);
                if (pstRsv->pstRCB == NULL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstRCB) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                    return FSR_BML_OAM_ACCESS_ERROR;
                }

                /* Check mis-aligned pointer */
                if (((UINT32)(pstRsv->pstRCB) & 0x3) != 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstRCB) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                    return FSR_OAM_NOT_ALIGNED_MEMPTR;
                }

            }
            /* single plane device does not allocate memory */
            else
            {
                pstRsv->pstRCB = NULL;
            }

            /* allocate memory for reservoir candidate block field */
            pstRsv->pstERL = (BmlERL *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                          sizeof(BmlERL),
                                                          nMemType);
            if (pstRsv->pstERL == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstERL) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pstERL) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstERL) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* 2KB page device */
            if (pstVol->nSctsPerPg == BML_2KB_PG)
            {
                /* max number of reservoir blocks */
                nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_2KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
            }
            /* 4KB page device */
            else
            { 
                /* max number of reservoir blocks */
                nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_4KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
            }

            /* calculate size of BABitMap using nSizeOfMaxRsvr */
            nSizeOfBABitMap = (nSizeOfMaxRsvr / BML_NUM_OF_BITS_IN_1BYTE) + 
                             ((nSizeOfMaxRsvr % BML_NUM_OF_BITS_IN_1BYTE) ? 1 : 0);

            /* allocated memory size should be aligned by 4 */
            if ((nSizeOfBABitMap & 0x3) != 0)
            {
                nSizeOfBABitMap = ((nSizeOfBABitMap >> 0x2) + 1) << 0x2;
            }

            /* allocate memory for Bad block Allocation Bitmap */
            pstRsv->pBABitMap = (UINT8 *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                            nSizeOfBABitMap,
                                                            nMemType);
            if (pstRsv->pBABitMap == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBABitMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pBABitMap) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBABitMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate memory for Bad Unit Map */
            nMallocSize = (pstVol->nNumOfBlksInDie > BML_BLKS_PER_BADUNIT) ? pstVol->nNumOfBlksInDie / BML_BLKS_PER_BADUNIT : 1;  

            /* allocated memory size should be aligned by 4 */
            if ((nMallocSize & 0x3) != 0)
            {
                nMallocSize = ((nMallocSize >> 0x2) + 1) << 0x2;
            }

            pstRsv->nNumOfBUMap = nMallocSize;
            pstRsv->pBUMap = (BmlBadUnit *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                              nMallocSize * sizeof(BmlBadUnit),
                                                              nMemType);
            if (pstRsv->pBUMap == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBUMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pBUMap) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBUMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* It's the special code for USP */
            /* pstRsv->pBUMap4DP = (BadUnit *) FSR_OAM_MallocExt(nMallocSize * sizeof(BadUnit)); */

            /* allocate buffer for read or write operation */
            pstRsv->pMBuf = pstDie->pMBuf;
            pstRsv->pSBuf = pstDie->pSBuf;
        }

    }

    /* Set the value of volume context */
    pstVol->nSftNumOfDev        =  _GetShfValue(pstVol->nNumOfDev);

    if (pstVol->nNumOfDev == 1)
    {
        pstVol->nMaskPDev = 0;
    }
    else if (pstVol->nNumOfDev == 2)
    {
        pstVol->nMaskPDev = 1;
    }
    else /* if (pstVol->nNumOfDev == 4) */
    {
        pstVol->nMaskPDev = 2;
    }

    pstVol->nSftNumOfPln        =  _GetShfValue(pstVol->nNumOfPlane);

    if (pstVol->nNumOfDieInDev == 1)
    {
        pstVol->nDDPMask = 0;
    }
    else
    {
        pstVol->nDDPMask = 1;
    }

    pstVol->nSftNumOfBlksInDie  =  _GetShfValue(pstVol->nNumOfBlksInDie);

    pstVol->nNumOfWays          =  pstVol->nNumOfDev << pstVol->nSftDDP;

    pstVol->nSftNumOfWays       =  _GetShfValue(pstVol->nNumOfWays);

    if (pstVol->nNumOfWays == 1)
    {
        pstVol->nMaskWays = 0;
    }
    else if (pstVol->nNumOfWays == 2)
    {
        pstVol->nMaskWays = 1;
    }
    else /* if (pstVol->nNumOfWays == 4) */
    {
        pstVol->nMaskWays = 2;
    }

    pstVol->nSftSLC             =  _GetShfValue(pstVol->nNumOfPgsInSLCBlk);

    pstVol->nSftMLC             =  _GetShfValue(pstVol->nNumOfPgsInMLCBlk);

    pstVol->nSizeOfPage         =  pstVol->nSctsPerPg * FSR_SECTOR_SIZE;

    pstVol->nSizeOfVPage        =  pstVol->nSizeOfPage * pstVol->nNumOfPlane;

    pstVol->nMaskSLCPgs         =  pstVol->nNumOfPgsInSLCBlk - 1;
    
    pstVol->nMaskMLCPgs         =  pstVol->nNumOfPgsInMLCBlk - 1;


    pstVol->nNumOfUsBlks        = (pstVol->nNumOfBlksInDev - 
                                  ((nNumOfRsvrBlks + BML_RSV_META_BLKS) * pstVol->nNumOfDieInDev)) *
                                  pstVol->nNumOfDev;

    pstVol->nLastUnit           = (pstVol->nNumOfUsBlks >> (pstVol->nSftNumOfPln+ pstVol->nSftNumOfWays)) - 1;

    /* allocate memory for Address log  */
    for (nIdx = 0; nIdx < pstVol->nNumOfWays; nIdx++)
    {
        pstVol->pstAddr[nIdx] = (BmlAddrLog *) FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                                 sizeof(BmlAddrLog),
                                                                 FSR_OAM_LOCAL_MEM);

        if (pstVol->pstAddr[nIdx] == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for pstAddr) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
            return FSR_BML_OAM_ACCESS_ERROR;
        }

        /* Check mis-aligned pointer */
        if (((UINT32)(pstVol->pstAddr[nIdx]) & 0x3) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for pstAddr) / %d line\r\n"),
                                            __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                            __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
            return FSR_OAM_NOT_ALIGNED_MEMPTR;
        }
    }

    /* Create Dump log */
    pstVol->pstDumpLog      = (BmlDumpLog *)FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                              sizeof(BmlDumpLog),
                                                              FSR_OAM_LOCAL_MEM);
    if (pstVol->pstDumpLog  == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for DumpLog) / %d line\r\n"),
                                         __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
        return FSR_BML_OAM_ACCESS_ERROR;
    }

    if (((UINT32)(pstVol->pstDumpLog) & 0x3) != 0)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for DumpLog) / %d line\r\n"),
                                         __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
        return FSR_OAM_NOT_ALIGNED_MEMPTR;
    }

    /* Create non-blocking context */
    pstVol->pstNBCxt        = (BmlNBCxt *)FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                            sizeof(BmlNBCxt),
                                                            FSR_OAM_LOCAL_MEM);
    if (pstVol->pstNBCxt    == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for BmlNBCxt) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
        return FSR_BML_OAM_ACCESS_ERROR;
    }

    /* Check mis-aligned pointer */
    if (((UINT32)(pstVol->pstNBCxt) & 0x3) != 0)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for BmlNBCxt) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
        return FSR_OAM_NOT_ALIGNED_MEMPTR;
    }

    /* Initialize non-blocking context */
    FSR_OAM_MEMSET(pstVol->pstNBCxt, 0x00, sizeof(BmlNBCxt));

    /* Create PI/PIExt */
    pstVol->pstPartI        = (FSRPartI *)FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                            sizeof(FSRPartI),
                                                            FSR_OAM_LOCAL_MEM);
    pstVol->pPIExt          = (FSRPIExt *)FSR_OAM_MallocExt(stPAM[nVol].nMemoryChunkID,
                                                            sizeof(FSRPIExt),
                                                            FSR_OAM_LOCAL_MEM);
    if ((pstVol->pstPartI      == NULL) ||
        (pstVol->pPIExt        == NULL))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for PI/PIExt) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
        return FSR_BML_OAM_ACCESS_ERROR;
    }

    /* Check mis-aligned pointer */
    if ((((UINT32)(pstVol->pstPartI) & 0x3) != 0) ||
        (((UINT32)(pstVol->pPIExt) & 0x3)   != 0))
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error for PI/PIExt) / %d line\r\n"),
                                        __FSR_FUNC__, __LINE__));
        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                        __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
        return FSR_OAM_NOT_ALIGNED_MEMPTR;
    }

    /* Initialize PIExt */
    FSR_OAM_MEMSET(pstVol->pPIExt, 0xFF, sizeof(FSRPIExt));

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FSR_BML_SUCCESS;
}

/**
 *  @brief      This function closes the volume.
 *
 *  @param [in]  nVol       : volume number
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_CRITICAL_ERROR
 *  @return     FSR_BML_ACQUIRE_SM_ERROR
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_Close(UINT32    nVol)
{
    UINT32          nDevIdx     = 0;                    /* Device Index             */
    UINT32          nDieIdx     = 0;                    /* Die Index                */
    UINT32          nPDev       = 0;                    /* Physical device number   */
    UINT32          nIdx        = 0;                    /* Temporary Index          */
    UINT32          nTINYFlag   = FSR_LLD_FLAG_NONE;    /* LLD flag for TINY FSR    */
    UINT32          nMemType    = FSR_OAM_LOCAL_MEM;
    INT32           nLLDRe      = FSR_LLD_SUCCESS;      /* LLD Return value         */
    INT32           nBMLRe      = FSR_BML_SUCCESS;      /* BML Return value         */
    INT32           nPAMRe      = FSR_PAM_SUCCESS;      /* PAM Return value         */
    BmlVolCxt      *pstVol;
    BmlDevCxt      *pstDev;
    BmlDieCxt      *pstDie;
    BmlReservoir   *pstRsv;
    FsrVolParm      stPAM[FSR_MAX_VOLS];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get the pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    nPAMRe  = FSR_PAM_GetPAParm(stPAM);
    if (nPAMRe != FSR_PAM_SUCCESS)
    {
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    /* This code prevents from opening the invalid volume. */
    if (stPAM[nVol].nDevsInVol == 0)
    {
        return FSR_BML_PAM_ACCESS_ERROR;
    }

    nMemType = FSR_OAM_LOCAL_MEM;
    if (stPAM[nVol].bProcessorSynchronization == TRUE32)
    {
        if (*(pstVol->pbUseSharedMemory))
        {
            nMemType = FSR_OAM_SHARED_MEM;
        }
        else
        {
            nMemType = FSR_OAM_LOCAL_MEM;
        }
    }
    else
    {
        nMemType = FSR_OAM_LOCAL_MEM;
    }

    /* Following code closes all device */
    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev       = nVol * DEVS_PER_VOL + nDevIdx;
        pstDev      = _GetDevCxt(nPDev);

        if (pstDev == NULL)
        {
            continue;
        }

        /* LLD_Close call */
        nLLDRe      = pstVol->LLD_Close(nPDev,
                                        FSR_LLD_FLAG_NONE | nTINYFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            nBMLRe  = FSR_BML_CRITICAL_ERROR;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d) / %d line\r\n"),
                                            __FSR_FUNC__, nVol, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_Close(nPDev: %d, nRe: 0x%x)\r\n"),
                                            nPDev, nLLDRe));
            break;
        }

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDie = pstDev->pstDie[nDieIdx];

            if (pstDie == NULL)
            {
                continue;
            }

            /* Free for Main Buffer */
            if (pstDie->pMBuf != NULL)
            {
                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pMBuf, nMemType);
                pstDie->pMBuf = NULL;
            }

            /* Free for Spare Buffer */
            if (pstDie->pSBuf != NULL)
            {
                /* Free for Spare base. Buffer */
                if (pstDie->pSBuf->pstSpareBufBase != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pSBuf->pstSpareBufBase, nMemType);
                    pstDie->pSBuf->pstSpareBufBase = NULL;
                }
                /* Free for Spare Ext. Buffer */
                if (pstDie->pSBuf->pstSTLMetaExt != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pSBuf->pstSTLMetaExt, nMemType);
                    pstDie->pSBuf->pstSTLMetaExt = NULL;
                }

                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pSBuf, nMemType);
                pstDie->pSBuf = NULL;
            }

            /* Free for return value of each partition */
            if (pstDie->pnRetOfPartI != NULL)
            {
                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pnRetOfPartI, nMemType);
                pstDie->pnRetOfPartI = NULL;
            }

            /* Free for Next Previous Operation data */
            if (pstDie->pstNextPreOp != NULL)
            {
                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pstNextPreOp, nMemType);
                pstDie->pstNextPreOp = NULL;
            }
            
            /* Free for Previous Operation data */
            if (pstDie->pstPreOp != NULL)
            {
                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie->pstPreOp, nMemType);
                pstDie->pstPreOp = NULL;
            }

            pstRsv = pstDie->pstRsv;

            /* If the die has a reservoir, free reservoir structure */
            if (pstRsv != NULL)
            {
                if (pstRsv->pstBMI != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pstBMI, nMemType);
                    pstRsv->pstBMI = NULL;
                }

                if (pstRsv->pstBMF != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pstBMF, nMemType);
                    pstRsv->pstBMF = NULL;
                }

                if (pstRsv->pstRCB != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pstRCB, nMemType);
                    pstRsv->pstRCB  = NULL;
                }

                if (pstRsv->pBABitMap != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pBABitMap, nMemType);
                    pstRsv->pBABitMap = NULL;
                }

                if (pstRsv->pBUMap != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pBUMap, nMemType);
                    pstRsv->pBUMap = NULL;
                }

                if (pstRsv->pstERL != NULL)
                {
                    FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstRsv->pstERL, nMemType);
                    pstRsv->pstERL = NULL;
                }

                pstRsv->pMBuf = NULL;
                pstRsv->pSBuf = NULL;

                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, gpstDevCxt[nPDev]->pstDie[nDieIdx]->pstRsv, FSR_OAM_LOCAL_MEM);
                gpstDevCxt[nPDev]->pstDie[nDieIdx]->pstRsv = NULL;

                FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, gpstDevCxt[nPDev]->pstDie[nDieIdx]->pstRsvSh, nMemType);
                gpstDevCxt[nPDev]->pstDie[nDieIdx]->pstRsvSh = NULL;
            }

            /* Free the allocated memory of die context*/
            FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDie, FSR_OAM_LOCAL_MEM);
            gpstDevCxt[nPDev]->pstDie[nDieIdx] = NULL;
        }

        /* Free the allocated memory of device context*/
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID, pstDev, FSR_OAM_LOCAL_MEM);
        gpstDevCxt[nPDev]   = NULL;
    }

    if (pstVol->pstPartI != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        pstVol->pstPartI,
                        FSR_OAM_LOCAL_MEM);
        pstVol->pstPartI = NULL;
    }

    if (pstVol->pPIExt != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        pstVol->pPIExt,
                        FSR_OAM_LOCAL_MEM);
        pstVol->pPIExt = NULL;
    }

    /*free memory for Address log  */
    for (nIdx = 0; nIdx < pstVol->nNumOfWays; nIdx++)
    {
        if (pstVol->pstAddr[nIdx] != NULL)
        {
            FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                            pstVol->pstAddr[nIdx],
                            FSR_OAM_LOCAL_MEM);
            pstVol->pstAddr[nIdx] = NULL;
        }
    }

    /* free memory for DumpLog */
    if (pstVol->pstDumpLog != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        pstVol->pstDumpLog,
                        FSR_OAM_LOCAL_MEM);
        pstVol->pstDumpLog = NULL;
    }

    /* free memory for non-blocking context */
    if (pstVol->pstNBCxt != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        pstVol->pstNBCxt,
                        FSR_OAM_LOCAL_MEM);
        pstVol->pstNBCxt = NULL;
    }

#if defined(FSR_ONENAND_EMULATOR)
#if !defined(FSR_DUAL)
    /* free memory for non-blocking context */
    if (pstVol->pstNBCxt != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        gstVolCxt[nVol].pnSharedOpenCnt,
                        nMemType);
        gstVolCxt[nVol].pnSharedOpenCnt = NULL;
    }

    /* free memory for non-blocking context */
    if (pstVol->pstNBCxt != NULL)
    {
        FSR_OAM_FreeExt(stPAM[nVol].nMemoryChunkID,
                        gstVolCxt[nVol].pbUseSharedMemory,
                        nMemType);
        gstVolCxt[nVol].pbUseSharedMemory = NULL;
    }
#endif /* #if !defined(FSR_DUAL) */
#endif /* #if defined(FSR_ONENAND_EMULATOR) */


    /* Initialize bVolOpen */
    pstVol->bVolOpen = FALSE32;

    if (stPAM[nVol].bProcessorSynchronization == TRUE32)
    {
        *(pstVol->pnSharedOpenCnt)  = *(pstVol->pnSharedOpenCnt) - 1;
        if (*(pstVol->pnSharedOpenCnt) == 0)
        {
            *(pstVol->pbUseSharedMemory) = FALSE32;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    return nBMLRe;
}

/**
 *  @brief      This function mounts reservoir of device in the given volume.
 *
 *  @param [in]   nVol            : volume number
 *  @param [out] *pstLoadingPI    : Pointer to FSRPartI structure
 *  @param [out] *pstLoadingPExt  : Pointer to FSRPIExt structure
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     Some BBM errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_MountRsvr(UINT32    nVol,
           FSRPartI *pstLoadingPI,
           FSRPIExt *pstLoadingPExt)
{
    UINT32      nDevIdx = 0;                /* Device Index             */
    UINT32      nPDev   = 0;                /* Physical device number   */
    BOOL32      bRe     = TRUE32;           /* Temporary return value   */
    INT32       nBMLRe  = FSR_BML_SUCCESS;  /* BML Return value         */
    BmlVolCxt  *pstVol;
    BmlDevCxt  *pstDev;
    FSRPartI   *pstPI;
    FSRPIExt   *pstPExt;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer of volume context */
    pstVol = _GetVolCxt(nVol);

    /* this function assume that volume has always a device,
     * thus, this function doesn't check whether volume has device or not
     */
    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL ; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        /* Check whether device is opened */
        bRe = _IsOpenedDev(nPDev);
        if (bRe == FALSE32)
        {
            continue;
        }

        /* Get pointer of device context */
        pstDev = _GetDevCxt(nPDev);

        /* Only 1st device has valid partition information */
        if (nDevIdx == 0)
        {
            pstPI   = pstLoadingPI;
            pstPExt = pstLoadingPExt;
        }
        else
        {
            pstPI   = NULL;
            pstPExt = NULL;
        }

        /* Call FSR_BBM_Mount */
        nBMLRe = FSR_BBM_Mount(pstVol,
                               pstDev,
                               pstPI,
                               pstPExt);
        if (nBMLRe != FSR_BML_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d) / %d line\r\n"),
                                            __FSR_FUNC__, nVol, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   FSR_BBM_Mount(nPDev: %d)\r\n"), nPDev));
            break;
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT]   Mounting Reservoir(nPDev: %d) is completed.\r\n"), nPDev));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    return nBMLRe;
}

/**
 *  @brief      This function changes the state of physical block by nCode.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in]  nVun        : Virtual unit number
 *  @param [in]  nNumOfUnits : The number of units
 *  @param [in]  nCode       : FSR_LLD_IOCTL_LOCK_BLOCK
                               FSR_LLD_IOCTL_LOCK_TIGHT
                               FSR_LLD_IOCTL_UNLOCK_BLOCK
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     FSR_BML_INVALID_PARAM
 *  @return     some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_SetBlkState(UINT32   nVol,
             UINT32   nVun,
             UINT32   nNumOfUnits,
             UINT32   nCode)
{
    UINT32          nPDev       = 0;    /* Physical device number               */
    UINT32          nDevIdx     = 0;    /* Device index                         */
    UINT32          nDieIdx     = 0;    /* Die index                            */
    UINT32          nPlnIdx     = 0;    /* Plane index                          */
    UINT32          nByteRet    = 0;    /* # of bytes returned                  */
    UINT32          nSbn        = 0;    /* Sbn to change a state                */
    UINT32          nNumOfBlks  = 0;    /* # of block to change a state         */
    UINT32          nLastSbn    = 0;    /* Last Sbn                             */
    UINT32          nErrPbn     = 0;    /* nPbn in error                        */
    UINT32          nNumOfBlksInRsvr    = 0;    /* # of reservec blks           */
    UINT32          n1stLockState =0;
    INT32           nLLDRe = FSR_LLD_SUCCESS;
    BmlVolCxt      *pstVol;
    BmlDevCxt      *pstDev;
    BmlDieCxt      *pstDie;
    LLDProtectionArg    stLLDIO;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d, nVun: %d, nNumOfUnits: %d, nCode: 0x%x)\r\n")
                                    , __FSR_FUNC__, nVol, nVun, nNumOfUnits, nCode));

    /* Get pointer to BmlVolCxt structure */
    pstVol = _GetVolCxt(nVol);

    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev   = (nVol * DEVS_PER_VOL) + nDevIdx;
        pstDev  = _GetDevCxt(nPDev);

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDie  = pstDev->pstDie[nDieIdx];

            /* Set nNumOfBlksInRsvr by n1stVun and reservoir type */
            nNumOfBlksInRsvr = 0;
            if ((nVun >= pstVol->nNumOfSLCUnit)              &&
                (pstDie->pstRsv->nRsvrType == BML_HYBRID_RESERVOIR))
            {
                nNumOfBlksInRsvr = pstDie->pstRsv->nLastSbnOfRsvr - pstDie->pstRsv->n1stSbnOfRsvr + 1;
            }

            nSbn        = (nDieIdx * pstVol->nNumOfBlksInDie)  +
                          (nVun << pstVol->nSftNumOfPln)       +
                          nNumOfBlksInRsvr;
            nNumOfBlks  = nNumOfUnits * pstVol->nNumOfPlane;
            nLastSbn    = nNumOfBlks + nSbn - 1;

            do
            {
                for (nPlnIdx = 0; nPlnIdx < pstVol->nNumOfPlane; nPlnIdx++)
                {
                    pstDie->nCurSbn[nPlnIdx] = (UINT16)(nSbn + nPlnIdx);
                    pstDie->nCurPbn[nPlnIdx] = (UINT16)(nSbn + nPlnIdx);
                }

                pstDie->nNumOfLLDOp = 0;
                _GetPBN(pstDie->nCurSbn[0], pstVol, pstDie);

                for (nPlnIdx = 0; nPlnIdx < pstVol->nNumOfPlane; nPlnIdx++)
                {
                    /* Set the elements of stLLDIO*/
                    stLLDIO.nStartBlk = (UINT32)pstDie->nCurPbn[nPlnIdx];
                    stLLDIO.nBlks     = 1;

                    if (stLLDIO.nStartBlk == 0 && nCode == FSR_LLD_IOCTL_UNLOCK_BLOCK)
                    {
                        /* Get OTP information */
                        nLLDRe = pstVol->LLD_IOCtl(nPDev,
                                                   FSR_LLD_IOCTL_OTP_GET_INFO,
                                                   NULL,
                                                   0,
                                                   (UINT8 *) &n1stLockState,
                                                   sizeof(n1stLockState),
                                                   &nByteRet);
                        if (nLLDRe != FSR_LLD_SUCCESS)
                        {
                            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   Fail getting OPT Info \r\n")));
                            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s (nVol: %d)/ %d line\r\n"),
                                                             __FSR_FUNC__, nVol, __LINE__));
                            return nLLDRe;
                        }

                        /* 
                        If 1st OTP locked, avoids unlocking of 1st Entry. 
                        1stOTP only is one block, regardless of multi way or plane.
                        And then, this checking code executes one time.
                        */
                        if (n1stLockState & FSR_LLD_OTP_1ST_BLK_LOCKED)
                        {
                            continue;
                        }
                    }

                    nLLDRe = pstVol->LLD_IOCtl(nPDev,
                                               nCode,
                                               (UINT8 *) &stLLDIO,
                                               sizeof(stLLDIO),
                                               (UINT8 *) &nErrPbn,
                                               sizeof(nErrPbn),
                                               &nByteRet);
                    if (nLLDRe != FSR_LLD_SUCCESS)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(nVol: %d, nVun: %d, nNumOfUnits: %d, nCode: 0x%x) / %d line\r\n"),
                                                        __FSR_FUNC__, nVol, nVun, nNumOfUnits, nCode, __LINE__));
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_IOCtl(nPDev: %d, nCode: 0x%x, nRe: 0x%x)\r\n"),
                                                        nPDev, nCode, nLLDRe));
                        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));
                        return nLLDRe;
                    }
                } /* End of "for (nPlnIdx = 0; nPlnIdx...)" */

                /* Increase nSbn by # of plane */
                nSbn += pstVol->nNumOfPlane;

            } while(nSbn <= nLastSbn);

        } /* End of "for (nDieIdx = 0; nDieIdx...)" */
    } /* End of "for (nDevIdx = 0; nDevIdx...)" */

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FSR_BML_SUCCESS;
}

/**
 *  @brief      This function gets shift value.
 *
 *  @param [in]  nValue: Value to look for shift value
 *
 *  @return     Shift value for an input value
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC UINT32
_GetShfValue(UINT32  nValue)
{
    UINT32  nSftValue = 0;          /* Shift value for an input value*/

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nValue: %d)\r\n"), __FSR_FUNC__, nValue));

    /* Special case*/
    if (nValue == 0)
        return nValue;

    /* Normal case*/
    while (~nValue & 1)
    {
        nValue >>= 1;
        nSftValue++;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return nSftValue;
}

/**
 *  @brief      This function gets physical block number from semi-physical block number.
 *
 *  @param [in]  nSbn   : Semi-physical block numeber
 *  @param [in] *pstVol : Pointer to VolCxt structure
 *  @param [in] *pstDie : pointer to DieCxt context
 *
 *  @return     none
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC VOID
_GetPBN(UINT32     nSbn,
        BmlVolCxt *pstVol,
        BmlDieCxt *pstDie)
{
    register UINT32      nIdx       = 0;    /* Temporary Index              */
    register UINT32      nNumOFBMFs = 0;    /* # of Block Map Field         */
    register UINT32      nPlnIdx    = 0;    /* Plane index                  */
    register BmlBMF     *pBMF;              /* pointer to BMF               */
    register BmlBadUnit *pBUMap;            /* pointer to BadUnit           */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s\r\n"), __FSR_FUNC__));

    pBUMap  = pstDie->pstRsv->pBUMap;

    /* find Bad Mapping info position using sbn */
    pBUMap += ((nSbn & (pstVol->nNumOfBlksInDie - 1)) >> BML_SFT_BLKS_PER_BADUNIT);
    
    if (pBUMap->nNumOfBMFs > 0)
    {
        nNumOFBMFs = pBUMap->nNumOfBMFs;
        pBMF       = (BmlBMF *) pstDie->pstRsv->pstBMF + pBUMap->n1stBMFIdx;

        /* 1 plane device */
        if (pstVol->nNumOfPlane == 1)
        {
            for (nIdx = 0; nIdx < nNumOFBMFs; nIdx++)
            {
                if (pBMF->nSbn == nSbn)
                {
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:INF]   Sbn(%d) ==> Pbn(%d)\r\n"), pBMF->nSbn, pBMF->nRbn));
                    pstDie->nCurPbn[nPlnIdx] = pBMF->nRbn;

                    break;
                }

                pBMF++;
            }
        }
        /* 2 plane device */
        else
        {
            for (nIdx = 0; nIdx < nNumOFBMFs; nIdx++)
            {
                /* replaced block info(Sbn) is equal to sbn in plane 0 or plane 1 */
                if ((pBMF->nSbn == nSbn) || (pBMF->nSbn == (nSbn + 1)))
                {
                    /* plane 1 */
                    if (pBMF->nSbn & 0x1)
                    {
                        /* 
                         * Bad mapping info includes only sbn in plane1 
                         * (unpaired replacement case, The block in plane0 is valid)
                         */
                        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:INF]   Sbn(%d) ==> Pbn(%d)\r\n"), pBMF->nSbn, pBMF->nRbn));
                        pstDie->nCurPbn[nPlnIdx + 1] = pBMF->nRbn;
                        pstDie->nNumOfLLDOp = 1;
                        break;
                    }
                    /* plane 0 */
                    else
                    {
                        FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:INF]   Sbn(%d) ==> Pbn(%d)\r\n"), pBMF->nSbn, pBMF->nRbn));
                        pstDie->nCurPbn[nPlnIdx] = pBMF->nRbn;

                        pBMF++;

                        /* block in plane1 is also replaced */
                        if (pBMF->nSbn == (nSbn + 1))
                        {
                            FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:INF]   Sbn(%d) ==> Pbn(%d)\r\n"), pBMF->nSbn, pBMF->nRbn));
                            pstDie->nCurPbn[nPlnIdx + 1] = pBMF->nRbn;

                            /* 
                             * unpaired replacement 
                             * Pbn is not continuous or block number in plane1 is even
                             * 
                             * Sbn:   10 /  11    10 /  11     10 /  11
                             * Rbn:  100 / 101   101 / 102    103 /  101 
                             *        (pair)      (unpair)     (unpair)
                             */
                            if ((pstDie->nCurPbn[nPlnIdx] + 1 != pBMF->nRbn) || 
                                (!(pBMF->nRbn & 0x1)))
                            {
                                pstDie->nNumOfLLDOp = 1;
                            }
                        }
                        /* The block in plane1 is valid (not replaced) */
                        else
                        {
                            /* unpaired replacement */
                            pstDie->nNumOfLLDOp = 1;
                        }

                        break;
                    }
                } /* end of if */
                
                pBMF++;
            } /* end of for */
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));
}

#if !(defined(TINY_FSR) || defined(FSR_NBL2))
/**
 *  @brief      This function gets the lock state of 1st block OTP or OTP block.
 *
 *  @param [in]  nVol        : Volume number
 *  @param [in] *pnLockstat  : Pointer to lock state of nOTPBlk
 *
 *  @return     FSR_BML_SUCCESS
 *  @return     Some LLD errors
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_GetOTPInfo(UINT32  nVol,
            UINT32 *pnLockstat)
{
    UINT32      nPDev       = 0;    /* Physical device number   */
    UINT32      nByteRet    = 0;    /* # of bytes returned      */
    UINT32      nLockstat   = FSR_LLD_OTP_OTP_BLK_LOCKED;   /* Lock state of OTP block  */
    INT32       nLLDRe      = FSR_LLD_SUCCESS;
    INT32       nBMLRe      = FSR_BML_SUCCESS;
    BmlVolCxt  *pstVol;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer to volume context */
    pstVol  = _GetVolCxt(nVol);

    /* Set nPDev using nVol */
    nPDev = (nVol << DEVS_PER_VOL_SHIFT);

    /* Check a state of OTP Block */
    nLLDRe = pstVol->LLD_IOCtl(nPDev,
                               FSR_LLD_IOCTL_OTP_GET_INFO,
                               NULL,
                               0,
                               (UINT8*) &nLockstat,
                               sizeof(nLockstat),
                               &nByteRet);
    if (nLLDRe != FSR_LLD_SUCCESS)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:   ]   %s(nVol: %d) / %d line\r\n"),
                                        __FSR_FUNC__, nVol, __LINE__));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   LLD_IOCtl(nPDev:%d, nCode:FSR_LLD_IOCTL_OTP_GET_INFO, nRe:0x%x)\r\n"),
                                        nPDev, nLLDRe));
        /* Return nLLDRe directly */
        return nLLDRe;
    }

    *pnLockstat = nLockstat;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return nBMLRe;
}

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
PUBLIC VOID
_PrintPartI(UINT32     nVol,
            FSRPartI  *pstPartI)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32      nIdx = 0;
    UINT32      nNumOfPartEntry;
    FSRPartEntry *pstPEntry;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    nNumOfPartEntry = pstPartI->nNumOfPartEntry;

    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF,
        (TEXT("[BIF:   ]   nPartition Information (nVol : %d)\r\n"), nVol));

    FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF,
        (TEXT("            nVer : 0x%x\r\n"),
        pstPartI->nVer));

    for (nIdx = 0; nIdx < nNumOfPartEntry; nIdx++)
    {
        pstPEntry = &(pstPartI->stPEntry[nIdx]);
        FSR_DBZ_RTLMOUT(FSR_DBZ_BML_IF,
            (TEXT("            %02d / nID:0x%02d / nAttr:0x%08x / 1stVun:%5d / Units:%5d\r\n"),
            nIdx,
            pstPEntry->nID,
            pstPEntry->nAttr,
            pstPEntry->n1stVun,
            pstPEntry->nNumOfUnits));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));
#endif
}
#endif /* TINY_FSR, FSR_NBL2 */

/**
 *  @brief      This function frees the local memory.
 *  @n          The local memory should be allocated to shared memory area.
 *
 *  @param [in]  nVol    : the volume number(0, 1) 
 *  @param [in] *pstVol  : pointer to VolCxt structure 
 *
 *  @return     none
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC VOID
_FreeLocalMem(UINT32        nVol,
              FsrVolParm   *pstPAM)
{
    UINT32          nDieIdx;        /* Die index                        */
    UINT32          nDevIdx;        /* Device index                     */
    UINT32          nPDev;          /* Physical device number           */
    BmlVolCxt      *pstVol;         /* pointer to BmlVolCxt structure   */
    BmlDevCxt      *pstDev;         /* pointer to BmlDevCxt structure   */
    BmlDieCxt      *pstDie;         /* pointer to BmlDieCxt structure   */
    BmlReservoir   *pstRsv;         /* pointer to BmlReservoir structure*/

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    /* Get pointer to volume context */
    pstVol = _GetVolCxt(nVol);

    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev       = nVol * DEVS_PER_VOL + nDevIdx;
        pstDev      = _GetDevCxt(nPDev);

        if (pstDev == NULL)
        {
            continue;
        }

        for (nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDie = pstDev->pstDie[nDieIdx];

            if (pstDie == NULL)
            {
                continue;
            }

            /* Free for Main Buffer */
            if (pstDie->pMBuf != NULL)
            {
                FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pMBuf, FSR_OAM_LOCAL_MEM);
                pstDie->pMBuf = NULL;
            }
            /* Free for Spare Ext. buffer */
            if (pstDie->pSBuf->pstSpareBufBase != NULL)
            {
                FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pSBuf->pstSpareBufBase, FSR_OAM_LOCAL_MEM);
                pstDie->pSBuf->pstSpareBufBase = NULL;
            }
            /* Free for Spare Ext. buffer */
            if (pstDie->pSBuf->pstSTLMetaExt != NULL)
            {
                FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pSBuf->pstSTLMetaExt, FSR_OAM_LOCAL_MEM);
                pstDie->pSBuf->pstSTLMetaExt = NULL;
            }
            /* Free for Next Previous Operation data */
            if (pstDie->pstNextPreOp != NULL)
            {
                FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pstNextPreOp, FSR_OAM_LOCAL_MEM);
                pstDie->pstNextPreOp = NULL;
            }
            
            /* Free for Previous Operation data */
            if (pstDie->pstPreOp != NULL)
            {
                FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pstPreOp, FSR_OAM_LOCAL_MEM);
                pstDie->pstPreOp = NULL;
            }

            pstRsv = pstDie->pstRsv;

            /* If the die has a reservoir, free reservoir structure */
            if (pstRsv != NULL)
            {
                if (pstRsv->pstBMI != NULL)
                {
                    FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstRsv->pstBMI, FSR_OAM_LOCAL_MEM);
                    pstRsv->pstBMI = NULL;
                }

                if (pstRsv->pstBMF != NULL)
                {
                    FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstRsv->pstBMF, FSR_OAM_LOCAL_MEM);
                    pstRsv->pstBMF = NULL;
                }

                if (pstRsv->pstRCB != NULL)
                {
                    FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstRsv->pstRCB, FSR_OAM_LOCAL_MEM);
                    pstRsv->pstRCB  = NULL;
                }

                if (pstRsv->pBABitMap != NULL)
                {
                    FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstRsv->pBABitMap, FSR_OAM_LOCAL_MEM);
                    pstRsv->pBABitMap = NULL;
                }

                if (pstRsv->pBUMap != NULL)
                {
                    FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstRsv->pBUMap, FSR_OAM_LOCAL_MEM);
                    pstRsv->pBUMap = NULL;
                }

                pstRsv->pMBuf = NULL;
                pstRsv->pSBuf = NULL;
            } /* End of "if (pstRsv != NULL)" */

            FSR_OAM_FreeExt(pstPAM->nMemoryChunkID, pstDie->pstRsvSh, FSR_OAM_LOCAL_MEM);
            pstDie->pstRsvSh = NULL;
        } /* End of "for (nDieIdx = 0; nDieIdx...)" */
    } /* End of "for (nDevIdx = 0;...)" */

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));
}

/**
 *  @brief      This function allocates the shared memory.
 *
 *  @param [in]  nVol    : volume number
 *  @param [in] *pstPAM  : pointer to FsrVolParm structure 
 *
 *  @return     none
 *
 *  @author     SuRyun Lee
 *  @version    1.0.0
 *
 */
PUBLIC INT32
_AllocSharedMem(UINT32      nVol,
                FsrVolParm *pstPAM)
{
    UINT32          nDevIdx;        /* Device index                         */
    UINT32          nDieIdx;        /* Die index                            */
    UINT32          nPDev;          /* Physical device number               */
    UINT32          nIdx;           /* Temporary index                      */
    UINT32          nNumOfRsvrBlks; /* the number of reserved blocks        */
    UINT32          nNumOfBMFs;     /* the number of BMFs                   */
    UINT32          nNumOfCandidates = 0;   /* the number of blocks for RCB */
    UINT32          nSizeOfMaxRsvr; /* maximum reservoir size               */
    UINT32          nSizeOfBABitMap;/* BAB Bitmap size                      */
    UINT32          nMallocSize;    /* malloc size for Bad Unit Map         */
    UINT32         *pRetPartI;      /* pointer for an array of return values*/
    BmlVolCxt      *pstVol;
    BmlDevCxt      *pstDev;
    BmlDieCxt      *pstDie;
    BmlReservoir   *pstRsv;
    BmlPreOpLog    *pstLog;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:IN ] ++%s(nVol: %d)\r\n"), __FSR_FUNC__, nVol));

    pstVol = _GetVolCxt(nVol);

    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev   = nVol * DEVS_PER_VOL + nDevIdx;
        pstDev  = _GetDevCxt(nPDev);

        for(nDieIdx = 0; nDieIdx < pstVol->nNumOfDieInDev; nDieIdx++)
        {
            pstDie = pstDev->pstDie[nDieIdx];

            /* Initialize the main and spare buffer pointer */
            pstDie->pMBuf = NULL;

            /* allocate main buffer memory and spare buffer memory */
            pstDie->pMBuf   = (UINT8 *)       FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                pstVol->nSctsPerPg * FSR_SECTOR_SIZE * pstVol->nNumOfPlane,
                                                                FSR_OAM_SHARED_MEM);
            if ((pstDie->pMBuf == NULL))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pMBuf) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate spare extension buffer memory */
            pstDie->pSBuf->pstSpareBufBase = (FSRSpareBufBase *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                                sizeof(FSRSpareBufBase),
                                                                                FSR_OAM_SHARED_MEM);
            if (pstDie->pSBuf->pstSpareBufBase == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pSBuf->pstSpareBufBase) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }


            /* allocate spare extension buffer memory */
            pstDie->pSBuf->pstSTLMetaExt = (FSRSpareBufExt *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                                sizeof(FSRSpareBufExt) * FSR_MAX_SPARE_BUF_EXT,
                                                                                FSR_OAM_SHARED_MEM);
            if (pstDie->pSBuf->pstSTLMetaExt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pSBuf->pstSTLMetaExt) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:temporary memory) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate pnRetOfPartI */
            pstDie->pnRetOfPartI = (UINT32 *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                sizeof(UINT32) * FSR_BML_MAX_PARTENTRY,
                                                                FSR_OAM_SHARED_MEM);
            if (pstDie->pnRetOfPartI == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie->pnRetOfPartI) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pnRetOfPartI) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstDie->pnRetOfPartI) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            pRetPartI = (UINT32 *) pstDie->pnRetOfPartI;
            /* Initialize the nRetOfPartI[]*/
            for (nIdx = 0; nIdx < FSR_BML_MAX_PARTENTRY; nIdx++)
            {
                *pRetPartI = FSR_BML_SUCCESS;
                pRetPartI++;
            }

            /* Initialize the PreOpLog */
            for (nIdx = 0; nIdx < BML_NUM_OF_PREOPLOG; nIdx++)
            {
                if (nIdx == 0)
                {
                    pstDie->pstPreOp     = (BmlPreOpLog *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                             sizeof(BmlPreOpLog),
                                                                             FSR_OAM_SHARED_MEM);
                    pstLog               = pstDie->pstPreOp;
                }
                else
                {
                    pstDie->pstNextPreOp = (BmlPreOpLog *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                             sizeof(BmlPreOpLog),
                                                                             FSR_OAM_SHARED_MEM);
                    pstLog               = pstDie->pstNextPreOp;
                }
                if (pstLog == NULL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:PreOpLog memory) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                    return FSR_BML_OAM_ACCESS_ERROR;
                }

                if (((UINT32)pstLog & 0x3) != 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:PreOpLog memory) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                    return FSR_OAM_NOT_ALIGNED_MEMPTR;
                }

                pstLog->nOpType   = BML_PRELOG_NOP;
                pstLog->nPgOffset = 0;
                pstLog->nSbn      = 0;
                pstLog->nFlag     = FSR_LLD_FLAG_NONE;
            }

            /* allocate the previous & next previous buffer */
            pstDie->pPreOpMBuf      = pstDie->pMBuf;
            pstDie->pNextPreOpMBuf  = pstDie->pMBuf;
            pstDie->pPreOpSBuf      = pstDie->pSBuf;
            pstDie->pNextPreOpSBuf  = pstDie->pSBuf;

            /* 
             * Flex-OneNAND has reservoirs for each die
             * SLC, MLC and OneNAND has only one reservoir in last die
             */

            /* allocate memory for reservoir of shared memory */
            pstDie->pstRsvSh = (BmlReservoirSh *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                                    sizeof(BmlReservoirSh),
                                                                    FSR_OAM_SHARED_MEM);
            if (pstDie->pstRsvSh == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:ReservoirSh structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstDie->pstRsvSh) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:ReservoirSh structure) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* Get pointer to reservoir structure */
            pstRsv =pstDie->pstRsv;

            /*
             * Set number of bad blocks in a die 
             * In case of MLC only device, nNumOfRsvrInSLC = 0
             * In case of SLC only device, nNumOfRsvrInMLC = 0
             */
            nNumOfRsvrBlks = pstVol->nNumOfRsvrBlks / pstVol->nNumOfDieInDev;

            nNumOfBMFs = nNumOfRsvrBlks;

            if (pstVol->nNumOfPlane > 1)
            {
                /* 
                 * If max number of bad blocks of the device is x,
                 * number of reservoir candidate blocks is (x / 2)
                 */
                nNumOfCandidates = (nNumOfRsvrBlks / 2);

                /* 
                 * If max number of bad blocks of the device is x,
                 * number of BMF entries is (x + x / 2)
                 */
                nNumOfBMFs  += (nNumOfRsvrBlks / 2);
            }
            
            /* allocated memory size should be aligned by 4 */
            if ((nNumOfBMFs & 0x3) != 0)
            {
                nNumOfBMFs = ((nNumOfBMFs >> 0x2) + 1) << 0x2;
            }

            pstRsv->pstBMI = (BmlBMI *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                          sizeof(BmlBMI),
                                                          FSR_OAM_SHARED_MEM);
            if (pstRsv->pstBMI == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMI) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            if (((UINT32)(pstRsv->pstBMI) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMI) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate memory for bad block mapping field */
            pstRsv->pstBMF = (BmlBMF *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                          nNumOfBMFs * sizeof(BmlBMF),
                                                          FSR_OAM_SHARED_MEM);
            if (pstRsv->pstBMF == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMF) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pstBMF) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstBMF) / %d line \r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* This field is only used for multi plane device */
            if (pstVol->nNumOfPlane > 1)
            {
                /* allocated memory size should be aligned by 4 */
                if ((nNumOfCandidates & 0x3) != 0)
                {
                    nNumOfCandidates = ((nNumOfCandidates >> 0x2) + 1) << 0x2;
                }

                /* allocate memory for reservoir candidate block field */
                pstRsv->pstRCB = (UINT16 *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                              nNumOfCandidates * sizeof(UINT16),
                                                              FSR_OAM_SHARED_MEM);
                if (pstRsv->pstRCB == NULL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstRCB) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                    return FSR_BML_OAM_ACCESS_ERROR;
                }

                /* Check mis-aligned pointer */
                if (((UINT32)(pstRsv->pstRCB) & 0x3) != 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstRCB) / %d line\r\n"),
                                                    __FSR_FUNC__, __LINE__));
                    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                    __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                    return FSR_OAM_NOT_ALIGNED_MEMPTR;
                }

            }
            /* single plane device does not allocate memory */
            else
            {
                pstRsv->pstRCB = NULL;
            }

            /* allocate memory for reservoir candidate block field */
            pstRsv->pstERL = (BmlERL *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                          sizeof(BmlERL),
                                                          FSR_OAM_SHARED_MEM);
            if (pstRsv->pstERL == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstERL) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pstERL) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pstERL) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* 2KB page device */
            if (pstVol->nSctsPerPg == BML_2KB_PG)
            {
                /* max number of reservoir blocks */
                nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_2KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
            }
            /* 4KB page device */
            else
            { 
                /* max number of reservoir blocks */
                nSizeOfMaxRsvr = ((BML_NUM_OF_BMS_4KB_PG * BML_BMFS_PER_BMS) * 2) / 3;
            }

            /* calculate size of BABitMap using nSizeOfMaxRsvr */
            nSizeOfBABitMap = (nSizeOfMaxRsvr / BML_NUM_OF_BITS_IN_1BYTE) + 
                             ((nSizeOfMaxRsvr % BML_NUM_OF_BITS_IN_1BYTE) ? 1 : 0);

            /* allocated memory size should be aligned by 4 */
            if ((nSizeOfBABitMap & 0x3) != 0)
            {
                nSizeOfBABitMap = ((nSizeOfBABitMap >> 0x2) + 1) << 0x2;
            }

            /* allocate memory for Bad block Allocation Bitmap */
            pstRsv->pBABitMap = (UINT8 *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                            nSizeOfBABitMap,
                                                            FSR_OAM_SHARED_MEM);
            if (pstRsv->pBABitMap == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBABitMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pBABitMap) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBABitMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* allocate memory for Bad Unit Map */
            nMallocSize = (pstVol->nNumOfBlksInDie > BML_BLKS_PER_BADUNIT) ? pstVol->nNumOfBlksInDie / BML_BLKS_PER_BADUNIT : 1;  

            /* allocated memory size should be aligned by 4 */
            if ((nMallocSize & 0x3) != 0)
            {
                nMallocSize = ((nMallocSize >> 0x2) + 1) << 0x2;
            }

            pstRsv->nNumOfBUMap = nMallocSize;
            pstRsv->pBUMap = (BmlBadUnit *) FSR_OAM_MallocExt(pstPAM->nMemoryChunkID,
                                                              nMallocSize * sizeof(BmlBadUnit),
                                                              FSR_OAM_SHARED_MEM);
            if (pstRsv->pBUMap == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBUMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_BML_OAM_ACCESS_ERROR));
                return FSR_BML_OAM_ACCESS_ERROR;
            }

            /* Check mis-aligned pointer */
            if (((UINT32)(pstRsv->pBUMap) & 0x3) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,  (TEXT("[BIF:ERR]   %s(FSR_OAM_MallocExt Error:pstRsv->pBUMap) / %d line\r\n"),
                                                __FSR_FUNC__, __LINE__));
                FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s(nVol: %d nRe: 0x%x)\r\n"),
                                                __FSR_FUNC__, nVol, FSR_OAM_NOT_ALIGNED_MEMPTR));
                return FSR_OAM_NOT_ALIGNED_MEMPTR;
            }

            /* It's the special code for USP */
            /* pstRsv->pBUMap4DP = (BadUnit *) FSR_OAM_MallocExt(nMallocSize * sizeof(BadUnit)); */

            /* allocate buffer for read or write operation */
            pstRsv->pMBuf = pstDie->pMBuf;
            pstRsv->pSBuf = pstDie->pSBuf;
        } /* End of "for(nDieIdx = 0; nDieIdx...)" */

    } /* End of "for (nDevIdx = 0; nDevIdx ...)" */

    FSR_DBZ_DBGMOUT(FSR_DBZ_BML_IF, (TEXT("[BIF:OUT] --%s\r\n"), __FSR_FUNC__));

    return FSR_BML_SUCCESS;
}

/**
 *  @brief      This function gets a lock volume information.
 *
 *  @param [in]  nVol    : volume number
 *
 *  @return     FSR_OAM_SM_TYPE_BML_VOL_0
 *  @return     FSR_OAM_SM_TYPE_BML_VOL_1
 *
 *  @author     Heegyu Kim
 *  @version    1.2.0
 *
 */
PUBLIC UINT32
_GetLockLayer(UINT32      nVol)
{
    if (nVol == 0)
    {
        return FSR_OAM_SM_TYPE_BML_VOL_0;
    }
    else
    {
        return FSR_OAM_SM_TYPE_BML_VOL_1;
    }

}
