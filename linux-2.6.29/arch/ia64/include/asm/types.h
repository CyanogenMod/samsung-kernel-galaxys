#ifndef _ASM_IA64_TYPES_H
#define _ASM_IA64_TYPES_H

/*
 * This file is never included by application software unless explicitly requested (e.g.,
 * via linux/types.h) in which case the application is Linux specific so (user-) name
 * space pollution is not a major issue.  However, for interoperability, libraries still
 * need to be careful to avoid a name clashes.
 *
 * Based on <asm-alpha/types.h>.
 *
 * Modified 1998-2000, 2002
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */

#include <asm-generic/int-l64.h>

#ifdef __ASSEMBLY__
# define __IA64_UL(x)		(x)
# define __IA64_UL_CONST(x)	x

# ifdef __KERNEL__
#  define BITS_PER_LONG 64
# endif

#else
# define __IA64_UL(x)		((unsigned long)(x))
# define __IA64_UL_CONST(x)	x##UL

typedef unsigned int umode_t;

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
# ifdef __KERNEL__

#define BITS_PER_LONG 64

/* DMA addresses are 64-bits wide, in general.  */

typedef u64 dma_addr_t;

# endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* _ASM_IA64_TYPES_H */
