/* alloc.h - External definitions for memory allocation subsystem */
/* $Id$ */

#include "copyright.h"

#ifndef M_ALLOC_H
#define M_ALLOC_H

#define	POOL_SBUF	0
#define	POOL_MBUF	1
#define	POOL_LBUF	2
#define	POOL_BOOL	3
#define	POOL_DESC	4
#define	POOL_QENTRY	5
#define POOL_PCACHE	6
#define	NUM_POOLS	7


#define LBUF_SIZE	8000	/* standard lbuf */
#define GBUF_SIZE       1024    /* generic buffer size */
#define MBUF_SIZE	400	/* standard mbuf */
#define PBUF_SIZE       128     /* pathname buffer size */
#define SBUF_SIZE	64	/* standard sbuf, short strings */

/*
#define LBUF_SIZE	4000
#define MBUF_SIZE	200
#define SBUF_SIZE	32
*/

#define strsave(s) strdup(s)

#ifndef STANDALONE

extern void	FDECL(pool_init, (int, int));
extern char *	FDECL(pool_alloc, (int, const char *));
extern void	FDECL(pool_free, (int, char **));
extern void	FDECL(list_bufstats, (dbref));
extern void	FDECL(list_buftrace, (dbref));

#define	alloc_lbuf(s)	pool_alloc(POOL_LBUF,s)
#define	free_lbuf(b)	pool_free(POOL_LBUF,((char **)&(b)))
#define	alloc_mbuf(s)	pool_alloc(POOL_MBUF,s)
#define	free_mbuf(b)	pool_free(POOL_MBUF,((char **)&(b)))
#define	alloc_sbuf(s)	pool_alloc(POOL_SBUF,s)
#define	free_sbuf(b)	pool_free(POOL_SBUF,((char **)&(b)))
#define	alloc_bool(s)	(struct boolexp *)pool_alloc(POOL_BOOL,s)
#define	free_bool(b)	pool_free(POOL_BOOL,((char **)&(b)))
#define	alloc_qentry(s)	(BQUE *)pool_alloc(POOL_QENTRY,s)
#define	free_qentry(b)	pool_free(POOL_QENTRY,((char **)&(b)))
#define alloc_pcache(s)	(PCACHE *)pool_alloc(POOL_PCACHE,s)
#define free_pcache(b)	pool_free(POOL_PCACHE,((char **)&(b)))

#else

#define	alloc_lbuf(s)	(char *)XMALLOC(LBUF_SIZE, "alloc_lbuf")
#define	free_lbuf(b)	if (b) XFREE(b, "alloc_lbuf")
#define	alloc_mbuf(s)	(char *)XMALLOC(MBUF_SIZE, "alloc_mbuf")
#define	free_mbuf(b)	if (b) XFREE(b, "alloc_mbuf")
#define	alloc_sbuf(s)	(char *)XMALLOC(SBUF_SIZE, "alloc_sbuf")
#define	free_sbuf(b)	if (b) XFREE(b, "alloc_sbuf")
#define	alloc_bool(s)	(struct boolexp *)XMALLOC(sizeof(struct boolexp), \
						"alloc_bool")
#define	free_bool(b)	if (b) XFREE(b, "alloc_bool")
#define	alloc_qentry(s)	(BQUE *)XMALLOC(sizeof(BQUE), "alloc_qentry")
#define	free_qentry(b)	if (b) XFREE(b, "alloc_qentry")
#define	alloc_pcache(s)	(PCACHE *)XMALLOC(sizeof(PCACHE), "alloc_pcache")
#define free_pcache(b)	if (b) XFREE(b, "alloc_pcache")
#endif

#define safe_copy_chr(scc__src,scc__buff,scc__bufp,scc__max) {\
    char *scc__tp;\
\
    scc__tp = *scc__bufp;\
    if ((scc__tp - scc__buff) < scc__max) {\
	*scc__tp++ = scc__src;\
	*scc__bufp = scc__tp;\
	*scc__tp = '\0';\
    } else {\
	scc__buff[scc__max] = '\0';\
    }\
}

#define	safe_str(s,b,p)		safe_copy_str((s),(b),(p),(LBUF_SIZE-1))
#define	safe_chr(c,b,p)		safe_copy_chr((c),(b),(p),(LBUF_SIZE-1))
#define safe_long_str(s,b,p)    safe_copy_long_str((s),(b),(p),(LBUF_SIZE-1))
#define	safe_sb_str(s,b,p)	safe_copy_str((s),(b),(p),(SBUF_SIZE-1))
#define	safe_sb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(SBUF_SIZE-1))
#define	safe_mb_str(s,b,p)	safe_copy_str((s),(b),(p),(MBUF_SIZE-1))
#define	safe_mb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(MBUF_SIZE-1))
#define safe_chr_fn(c,b,p)      safe_chr_real_fn((c),(b),(p),(LBUF_SIZE-1))

#endif	/* M_ALLOC_H */
