/*
**++
**
**  FACILITY: A yet another Linux driver to pass serial I/O over network
**
**  ENVIRONMENT: Linux
**
**  DESCRIPTION: Data Srtuctures and constant definitions is supposed to be used across all MODBUS-TCP2RTU's modules
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
	volatile int	head, tail;
	char	buf[SER2NET$SZ_IOBUF];
} SER2NET$IOBUF;







/*
 *   DESCRIPTION: Returns a length  of data in given  buffer.
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	NONE:
 *
 *   RETURNS:
 *	<length of data i nbuffer>
 */
static inline size_t s_buf_len(
		SER2NET$IOBUF	*a_iobuf
		)
{
	if (a_iobuf->head >= a_iobuf->tail)
		return a_iobuf->head - a_iobuf->tail;

	return SER2NET$SZ_IOBUF - a_iobuf->tail + a_iobuf->head;
}

/*
 *   DESCRIPTION: Returns a sizeof of free space in given buffer.
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	NONE:
 *
 *   RETURNS:
 *	<length of free space>
 */
static inline size_t s_buf_space(
		SER2NET$IOBUF	*a_iobuf
		)
{
	return SER2NET$SZ_IOBUF - s_buf_len(a_iobuf) - 1;
}


/*
 *   DESCRIPTION: Returns a pointer to free *continual* area to accept portion of data.
 *	Don't forget correct <head> after placing data in the area!
 *
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	a_ptr:		An address of free space
 *
 *   RETURNS;
 *	0:		No free space
 *	0 >:		A size of the free space
 */
static inline void s_buf_adjust_head (
		SER2NET$IOBUF	*a_iobuf,
			ssize_t	a_len
		)
{
	a_iobuf->head = (a_iobuf->head + a_len) % SER2NET$SZ_IOBUF;		/* Adjust <heade> according has been processed length of data */
}

static inline size_t s_buf_free2put (
		SER2NET$IOBUF	*a_iobuf,
			char	**a_ptr
		)
{
	*a_ptr = a_iobuf->buf + a_iobuf->head;					/* Precompute address of free space */

	/*
	 *        tail               head
		   v                  v
	+---------------------------------------------------------+
	|  free    |       data       |      free                 |
	+---------------------------------------------------------+
	*/
	if (a_iobuf->head >= a_iobuf->tail)
		return (SER2NET$SZ_IOBUF - a_iobuf->head);

	/*
	 *        head               tail
		   v                  v
	+---------------------------------------------------------+
	|   data   |       free       |      data                 |
	+---------------------------------------------------------+
	*/
	return	a_iobuf->tail - a_iobuf->head;
}



/*
 *   DESCRIPTION: Returns a pointer to free *continual* area to accept portion of data.
 *	Don't forget correct <head> after placing data in the area!
 *
 *
 *   INPUTS:
 *	a_iobuf:	A buffer to check
 *
 *   OUTPUTS:
 *	a_ptr:		An address of free space
 *
 *   RETURNS;
 *	0:		No free space
 *	0 >:		A size of the free space
 */
static inline void s_buf_adjust_tail (
		SER2NET$IOBUF	*a_iobuf,
			ssize_t	a_len
		)
{
	a_iobuf->tail = (a_iobuf->tail + a_len) % SER2NET$SZ_IOBUF;		/* Adjust <tail> according has been processed length of data */
}

static inline size_t s_buf_data4get (
		SER2NET$IOBUF	*a_iobuf,
			char	**a_ptr
		)
{
	*a_ptr = a_iobuf->buf + a_iobuf->tail;					/* Precompute address of unread data */

	/*
	 *        tail               head
		   v                  v
	+---------------------------------------------------------+
	|  free    |       data       |      free                 |
	+---------------------------------------------------------+
	*/
	if (a_iobuf->head >= a_iobuf->tail)
		return	a_iobuf->head - a_iobuf->tail;


	/*
	 *        head               tail
		   v                  v
	+---------------------------------------------------------+
	|   data   |       free       |      data                 |
	+---------------------------------------------------------+
	*/
	return (SER2NET$SZ_IOBUF - a_iobuf->head);
}







enum {
	SER2NET$M_SER_OPEN = (1 << 0),
	SER2NET$M_NET_OPEN = (1 << 1),
};


typedef struct ser2net_ctx_t {
	int	flags;

	char	devname[SER2NET$SZ_DEVNAME + 1];	/* Device name like "ser2netXX" */


	SER2NET$IOBUF	ser2net,			/* I/O buffer for serial-to-network direction */
			net2ser;			/*  --//--        network-to-serial --//--    */

	struct wait_queue_head readq,			/* Incoming data in the <net2ser> buffer is ready */
			writeq;				/*       --//--         <ser2net> --//-- */

	struct sockaddr_in sk;				/* Socket for remote peer */

	struct task_struct *net_task;			/* A task\thread to performs network I/O */

		dev_t	devt;
	struct cdev	scdev;

	struct tty_struct tty;
} SER2NET$CTX;


#endif	/* __SER2NET_KERNEL */


enum {
	SER2NET$K_CTL_GETINFO = 0,			/* Get generaL about of the module */
	SER2NET$K_CTL_TRACE,				/* Set TRACE=ON|OFF */
	SER2NET$K_CTL_SETDVI,				/* Set Device Information vector for specified <ser2netXX>*/
	SER2NET$K_CTL_GETDVI,				/* Get --//-- */

	SER2NET$K_CTL_EOL,
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

#define $IPv4_ADDR33(a,b,c,d) ( \
    ((uint32_t)(a) << 24) | \
    ((uint32_t)(b) << 16) | \
    ((uint32_t)(c) <<  8) | \
    ((uint32_t)(d)      ) \
)

#endif



#endif	/* __SERIAL2NET_DEFS__ */
