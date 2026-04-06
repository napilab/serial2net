/*
**++
**
**  FACILITY: A yet another Linux driver to pass serial I/O over network
**
**  ENVIRONMENT: Linux
**
**  DESCRIPTION: Data Srtuctures and constant definitions is supposed to be used across all  modules
**
**
**  AUTHORS: Ruslan R. (The BadAss Sysman) Laishev
**
**  CREATION DATE:  13-JAN-2026
**
**  MODIFICATION HISTORY:
**
**--
*/

#ifndef	__SERIAL2NET_DEFS__
#define __SERIAL2NET_DEFS__	1


#include	<linux/mutex.h>
#include	<linux/wait.h>
#include	<net/sock.h>

#define	SER2NET$K_MAXDEV	32
#define	SER2NET$SZ_DEVNAME	32
#define SER2NET$T_DEVNAME	"ser2net"
#define SER2NET$SZ_IOBUF	4096


#ifdef	__SER2NET_KERNEL



typedef struct ser2net_iobuf_t {
	volatile int	head, tail,
			fl_full;
	char		data[SER2NET$SZ_IOBUF];
} SER2NET$IOBUF;

/*
 *   DESCRIPTION: Reset buffer's internal structure
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	NONE:
 *
 *   RETURNS:
 *	NONE
 */
static inline size_t s_iobuf_reset(
		SER2NET$IOBUF	*a_iobuf
		)
{
	return a_iobuf->tail = a_iobuf->head = 0;
}

/*
 *   DESCRIPTION: Check for data in the given buffer
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	NONE:
 *
 *   RETURNS:
 *	0:	No data
 *	1:	Data is in buffer
 */
static inline size_t s_iobuf_isdata(
		SER2NET$IOBUF	*a_iobuf
		)
{
	return a_iobuf->tail != a_iobuf->head;
}



/*
 *  DESCRIPTION: Return a continues space in the I/O buffer
 *
 *  INPUTS:
 *	a_ring:		Address of the ring I/O buffer
 *
 *  OUTPUTS:
 *	a_free:		An address of first free byte in the buffer
 *
 *  RETURNS:
 *	Size of the space
 */
static inline int s_iobuf_getfree (
		SER2NET$IOBUF	*a_ring,
			char	**a_free
		)
{
	if ( a_ring->fl_full )
		return 0;			/* No free space */


	*a_free = &a_ring->data[a_ring->tail];

	/*
		    Old --->>> New
	 +-----+--------------------+----------------------+
	 | Free|       Data         |  Free                |
	 +-----+--------------------+----------------------+
	       ^                    ^
	      Head                Tail
	*/
	if ( a_ring->tail >= a_ring->head )
		return SER2NET$SZ_IOBUF - a_ring->tail;


	/*
					 Old --->>> New
	 +-----+--------------------+----------------------+
	 | Data|       Free         |  Data                |
	 +-----+--------------------+----------------------+
	       ^                    ^
	      Tail                Head
	*/

	return a_ring->head - a_ring->tail;
}

/*
 *  DESCRIPTION: Correct ring buffer counters according has been allocated space,
 *	set "buffer if full" flag if need.
 *
 *  INPUTS:
 *	a_ring:		Address of the ring I/O buffer
 *	a_count:	A count of has been used octets in the buffer
 *
 *  OUTPUTS:
 *	NONE
 *
 *  RETURNS:
 *	NONE
 */
static inline void s_iobuf_adjfree (
		SER2NET$IOBUF	*a_ring,
			int	a_count
		)
{
	if ( !a_count )
		return;

	a_ring->tail = (a_ring->tail + a_count) % SER2NET$SZ_IOBUF;
	a_ring->fl_full = (a_ring->tail == a_ring->head);
}



static inline int s_iobuf_getdata (
		SER2NET$IOBUF	*a_ring,
			char	**a_data
		)
{
	if ( (!a_ring->fl_full) && (a_ring->head == a_ring->tail) )
		return 0;							/* No data in the buffer */


	*a_data = &a_ring->data[a_ring->head];

	/*
		    Old --->>> New
	 +-----+--------------------+----------------------+
	 | Free|       Data         |  Free                |
	 +-----+--------------------+----------------------+
	       ^                    ^
	      Head                Tail
	*/
	if ( a_ring->head < a_ring->tail )
		return	a_ring->tail - a_ring->head;


	/*
					 Old --->>> New
	 +-----+--------------------+----------------------+
	 | Data|       Free         |  Data                |
	 +-----+--------------------+----------------------+
	       ^                    ^
	      Tail                Head
	*/
	return	SER2NET$SZ_IOBUF - a_ring->head;

}

static inline void s_iobuf_adjdata (
		SER2NET$IOBUF	*a_ring,
			int	a_count
		)
{
	a_ring->head = (a_ring->head + a_count) %SER2NET$SZ_IOBUF;
	a_ring->fl_full = 0;						/* Buffer got free space !!! */
}



enum {
	SER2NET$M_SER_OPEN = (1 << 0),
	SER2NET$M_NET_OPEN = (1 << 1),
	SER2NET$M_NET_HUB = (1 << 2),
};

enum {
	SER2NET$K_SEQ_CUR = 0,
	SER2NET$K_SEQ_OLD,

	SER2NET$K_SEQ_MAX
};


typedef struct ser2net_ctx_t {
	int	flags;

	char	devname[SER2NET$SZ_DEVNAME + 1];	/* Device name like "ser2netXX" */


	SER2NET$IOBUF	ser2net,			/* I/O buffer for serial-to-network direction */
			net2ser;			/*  --//--        network-to-serial --//--    */

	struct wait_queue_head proc_que,		/* Outgoing data in the <ser2net> buffer is ready */
			readq;				/* Incoming data in the <net2ser> buffer is ready */

	struct sockaddr_in sk;				/* Socket for remote peer */

	struct task_struct *net_task;			/* A task\thread to performs network I/O */

		dev_t	devt;
	struct cdev	scdev;

	struct tty_struct tty;


	int		skread_seq,
			indata_seq[SER2NET$K_SEQ_MAX],
			outdata_seq;
} SER2NET$CTX;


#endif	/* __SER2NET_KERNEL */


enum {
	SER2NET$K_CTL_GETINFO = 0,			/* Get generaL about of the module */
	SER2NET$K_CTL_TRACE,				/* Set TRACE=ON|OFF */
	SER2NET$K_CTL_SETDVI,				/* Set Device Information vector for specified <ser2netXX>*/
	SER2NET$K_CTL_GETDVI,				/* Get --//-- */


	SER2NET$K_CTL_EOL				/* End-of-list marker */
};

#define	IOCTL$K_GETINFO		_IOWR(135,	SER2NET$K_CTL_GETINFO, char *)
#define	IOCTL$K_TRACE		_IO(135,	SER2NET$K_CTL_TRACE, int)
#define	IOCTL$K_SETDVI		_IOWR(135,	SER2NET$K_CTL_SETDVI, char *)
#define	IOCTL$K_GETDVI		_IOWR(135,	SER2NET$K_CTL_GETDVI, char *)



#ifndef $IPv4_BYTES
#define $IPv4_BYTES_FMT "%u.%u.%u.%u"
#define $IPv4_BYTES(addr) \
		(uint8_t) ((addr) & 0xFF), \
		(uint8_t) (((addr) >> 8) & 0xFF),\
		(uint8_t) (((addr) >> 16) & 0xFF),\
		(uint8_t) (((addr) >> 24) & 0xFF)
#endif

#ifndef $IPv4_ADDR
#define $IPv4_ADDR(a,b,c,d) htonl(( \
    ((uint32_t)(a) << 24) | \
    ((uint32_t)(b) << 16) | \
    ((uint32_t)(c) <<  8) | \
    ((uint32_t)(d)      )) \
)

#endif



#endif	/* __SERIAL2NET_DEFS__ */
