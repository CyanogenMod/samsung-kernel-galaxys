#include <FSR.h>

VOID    memcpy32 (VOID       *pDst,
                  VOID       *pSrc,
                  UINT32     nSize)
{
	int index;
	UINT32  *pSrc32;
	UINT32  *pDst32;
	UINT32   nSize32;

	pSrc32  = (UINT32 *)(pSrc);
	pDst32  = (UINT32 *)(pDst);
	nSize32 = nSize / sizeof (UINT32);

	for(index = 0; index < nSize32; index++)
	{
		pDst32[index] = pSrc32[index];
	}
}

