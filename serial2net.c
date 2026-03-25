#define	__MODULE__	"SER2NET"
#define	__IDENT__	"X.00-03"
#define	__REV__		"00.00.03"

/*
**++
**
**  FACILITY: A yet another Linux driver to pass serial-like port over network
**
**  ENVIRONMENT: Linux
**
**  ABSTRACT: This is a trivial Linux driver implement serial-like device, pass all I/O as-is over TCP-connection to
**	a remote "server". Remote server is a something like StarLet MODBUS TCP-to-RTU gateway or StarLet NET-to-SERIAL gateway.
**
**  DESCRIPTION: Main routine, read and process connfiguration, start workers.
**
**  AUTHORS: Ruslan R. (The BadAss Sysman) Laishev
**
**  CREATION DATE:  13-JAN-2026
**
**
**  USAGE:
**
**	$insmod ./serial2net.ko [p1=<value] [p2=<value] ...
**
**		p_ser2net_max=<number>	-	A number of serial devices, p_ser2net_max=4 will create /dev/ser2net00 ... /dev/ser2net03
**		p_ser2net_trace -		Enable extensible diagnostic output for debug and troubleshooting purpose
**		p_ser2net_peer -		A coma-separated list of host:port pairs, should be corresponding to number of <ser2netXX> devices:
**						"host1:port1, host2:por2, host3:port3"
**							/dev/ser2net00 ---- host1:port1
**								...
**							/dev/ser2net02 ---- host3:port3
**
**		p_ser2net_hub -			Switch driver to Hub\Server mode, in this case <p_ser2net_peer> defines  local end-points (TCP-listeners) to
**						accept remote connections.
**
**  EXAMPLE: (Note that the driver is compiled with *debug* option)
**
	$ insmod ./serial2net.ko p_ser2net_max=2 p_ser2net_peer="127.0.0.1:8135,172.16.1.1:502"
	$ dmesg
	...
	[13969.951537] [ 0x1b95 SER2NET\s_module_init:979] SER2NET Rev: X.00-01 Ruslan R. Laishev (AKA: BadAss Sysman, SysManOne), zator@yandex.ru
	[13969.953000] [ 0x1b95 SER2NET\s_module_init:981] SER2NET : p_ser2net_trace = 1
	[13969.953892] [ 0x1b95 SER2NET\s_module_init:982] SER2NET : p_ser2net_max = 2
	[13969.954728] [ 0x1b95 SER2NET\s_module_init:983] SER2NET : p_ser2net_peer_nr = 2
	[13969.955610] [ 0x1b95 SER2NET\s_module_init:987] SER2NET : p_ser2net_peer[i] = '127.0.0.1:8135'
	[13969.956654] [ 0x1b95 SER2NET\s_module_init:1017] [00]: Peer: <127.0.0.1:8135>
	[13969.957404] [ 0x1b95 SER2NET\s_module_init:987] SER2NET : p_ser2net_peer[i] = '172.16.1.1:502'
	[13969.958320] [ 0x1b95 SER2NET\s_module_init:1017] [01]: Peer: <172.16.1.1:502>
	[13969.959264] [ 0x1b95 SER2NET\s_ser2net_ctxs_init:964] Created ser2net context for <ser2net00>, ser2net: 00000000605eec2b, task: 00000000535a117e
	[13969.960179] [ 0x1b96 SER2NET\s_network_proc:559] <ser2net00>: kernel_connect(127.0.0.1:8135)->-111, retry after 3 seconds ...
	[13969.960594] [ 0x1b95 SER2NET\s_ser2net_ctxs_init:964] Created ser2net context for <ser2net01>, ser2net: 000000009bdfbb18, task: 0000000009ba6068
	[13969.964263] [ 0x1b95 SER2NET\s_module_init:1034] 2 contexts is initialized
	[13973.162262] [ 0x1b96 SER2NET\s_network_proc:559] <ser2net00>: kernel_connect(127.0.0.1:8135)->-111, retry after 3 seconds ...


	$ root@devuan4-sysman:/sysman/Works/DOK/serial2net# tail -f /dev/ser2net
	[33524.000452] [0x4124 #00000001 SER2NET\s_ser2net$ctl_init:474] Control device is initialized
	[33524.000452] [0x4125 #00000002 SER2NET\s_network_proc:523] <ser2net00>: Remote link: 127.0.0.1:8135 --- worker thread is started
	[33524.000452] [0x4125 #00000003 SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
	[33524.000455] [0x4126 #00000004 SER2NET\s_network_proc:523] <ser2net01>: Remote link: 172.16.1.1:502 --- worker thread is started
	[33524.000456] [0x4126 #00000005 SER2NET\s_network_proc:554] <ser2net01>: Connecting to: 172.16.1.1:502 ...
	[33527.000647] [0x4125 #00000006 SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
	[33530.000713] [0x4125 #00000007 SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
	[33533.000781] [0x4125 #00000008 SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
	[33536.000856] [0x4125 #00000009 SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
	[33539.000924] [0x4125 #0000000a SER2NET\s_network_proc:554] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
**
**
**  MODIFICATION HISTORY:
**
**	27-JAN-2026	RRL	X.00-03: Added Hub\Server mode support (p_ser2net_hub)
**
**--
*/
#include	<linux/init.h>
#include	<linux/module.h>
#include	<linux/fs.h>
#include	<linux/cdev.h>
#include	<linux/uaccess.h>
#include	<linux/mutex.h>
#include	<linux/wait.h>
#include	<linux/ioctl.h>
#include	<linux/errno.h>
#include	<linux/list.h>
#include	<linux/miscdevice.h>
#include	<linux/net.h>
#include	<linux/in.h>
#include	<linux/socket.h>
#include	<linux/slab.h>
#include	<linux/mempool.h>
#include	<linux/list.h>
#include	<linux/kref.h>
#include	<linux/spinlock.h>
#include	<linux/tcp.h>
#include	<linux/sockptr.h>
#include	<net/tcp.h>
#include	<linux/in.h>
#include	<linux/inet.h>
#include	<linux/termios.h>
#include	<linux/tty.h>

#define		__SER2NET_KERNEL
#include	"serial2net.h"



MODULE_LICENSE("GPL");
static const char s_author_desc [] =  "Ruslan R. Laishev (AKA: BadAss Sysman, SysManOne), zator@yandex.ru";
MODULE_AUTHOR("Ruslan R. Laishev (AKA: BadAss Sysman, SysManOne), zator@yandex.ru");
MODULE_DESCRIPTION("A yet anoteher driver to pass serial I/O over network");


static int p_ser2net_trace = 1;
module_param( p_ser2net_trace, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(p_serial2net_trace, "Extensible diagnostic output ([0] = Disabled, 1 = Enabled)");

static int p_ser2net_max = 1;
module_param( p_ser2net_max, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(p_serial2net_max, "A number of preallocated serial devices (1-32)");


static int p_ser2net_peer_nr = 0;							/* A number of array's elements */
static char *p_ser2net_peer[SER2NET$K_MAXDEV];
module_param_array(p_ser2net_peer, charp, &p_ser2net_peer_nr,  S_IRUGO | S_IWUSR);	/* Comma separated list of IP:port pairs */
MODULE_PARM_DESC(p_serial2net_peer, "A comma-separated list of remote peers (eg: 127.0.0.1:8135,172.16.1.1:502)");


static int p_ser2net_hub = 0;
module_param( p_ser2net_hub, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(p_serial2net_hub, "Switch drive in Hub\\Server mode");



#ifndef	_DEBUG
	#define	_DEBUG	1
#endif

#ifdef	_DEBUG				/* A macros to performs extensive debug output */
	#pragma	message		"$TRACE macro is defined!"

#define $TRACE(fmt, ...)	do { if (p_ser2net_trace) printk(KERN_DEBUG  "[ %#x " __MODULE__ "\\%s:%d] " fmt "\n", current->tgid, __FUNCTION__,  __LINE__ , ## __VA_ARGS__); } while (0)
#define $LOG(lvl, fmt, ...)	do { printk(lvl  "[ %#x " __MODULE__ "\\%s:%d] " fmt "\n", current->tgid, __FUNCTION__,  __LINE__ , ## __VA_ARGS__); } while (0)
#define $PUTMSG(fmt, ...)	s_ser2net$ctl_putmsg(fmt "\n", __FUNCTION__,  __LINE__ , ## __VA_ARGS__)
#else
	#define	$TRACE(fmt, ...)	{}
	#define $LOG(lvl, fmt, ...)	{ printk(lvl  "[ %#x " __MODULE__ "] " fmt "\n", current->tgid, ## __VA_ARGS__); }
	#define $PUTMSG(fmt, ...)	{}
#endif

#define $SHOW_PARM(name, value, format) $TRACE(__MODULE__ " : " #name " = " format, (value))
#define $SHOW_PTR(var)                  $SHOW_PARM(var, var, "%#x")
#define $SHOW_XL(var)			$SHOW_PARM(var, var, "%#x")
#define $SHOW_STR(var)                  $SHOW_PARM(var, (var ? var : "UNDEF(NULL)"), "'%s'")
#define $SHOW_INT(var)                  $SHOW_PARM(var, ((int) var), "%d")
#define $SHOW_UINT(var)                 $SHOW_PARM(var, ((unsigned) var), "%u")
#define $SHOW_ULL(var)                  $SHOW_PARM(var, ((unsigned long long) var), "%llu")
#define $SHOW_UNSIGNED(var)             $SHOW_PARM(var, var, "0x%08x")
#define $SHOW_BOOL(var)                 $SHOW_PARM(var, (var ? "ENABLED(TRUE)" : "DISABLED(FALSE)"), "%s");



static SER2NET$CTX	s_ser2net_ctxs [ SER2NET$K_MAXDEV ] = {0};			/* Global table for SERIAL2NET contexts */
static struct class	*s_class;
/*
 * The ioctl() implementation to performs control functions.
 *	example on user level:
 *	struct {
 *		char	len,
 *		char	key[32];
 *		} lkey;
 *
 *
 *	fd = open("dev/ser2net");
 *	ioctl(fd, SER2NET$K_GETSTAT, &lkey);
 *	close(fd);
 *
 */
#define	SER2NET$K_CTLDEV	"ser2net"						/* Pseudo-device to accept IOCTL control requests */
static struct miscdevice s_ser2net$ctl;							/* Pseudo-device to accept IOCTL control requests */


/* /dev/ser2net$ctl
 * ------------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define		LOGENT$SZ_MSG		512
#define		LOGENT$SZ_DEFENT_NR	132
static LIST_HEAD(s_logent_list);							/* A list of log messages */
static atomic64_t	s_logent_list_nr;						/* A number of entries in the list */
DEFINE_SPINLOCK(s_logent_list_lock);							/* A spinlock to coordinate access to the <s_logent_list> */


typedef struct	logent_t {
	struct list_head	link;
	struct timespec64	ts;
		char	__fill1[64];
		uint64_t	seq;
		uint16_t	len;

		char	__fill2[64];
		char		text[LOGENT$SZ_MSG + 1];
} LOGENT;

struct kmem_cache	*s_logent_cache;
static mempool_t	*s_logent_pool;


static int	s_cre_logent_slab	(
				int	a_defent_nr,
				size_t	a_entsz
					)
{
	if ( !(s_logent_cache = kmem_cache_create( "LOG Msgs", a_entsz, 0, 0, NULL)) )
		{
		$LOG(KERN_ERR, "kmem_cache_create() failed");
		return	-ENOMEM;
		}

	if ( !(s_logent_pool =  mempool_create_slab_pool( a_defent_nr, s_logent_cache)) )
		{
		$LOG(KERN_ERR, "mempool_init_slab_pool() failed");
		return	-ENOMEM;
		}

	return	0;
}

static void	s_del_logent_slab	(void)
{
LOGENT	*l_logent, *l_tmp;

	list_for_each_entry_safe(l_logent, l_tmp, &s_logent_list, link)
		{
		list_del(&l_logent->link);
		mempool_free(l_logent, s_logent_pool);
		}


	mempool_destroy(s_logent_pool);
	kmem_cache_destroy(s_logent_cache);
}


static inline int	s_get_logent (
			LOGENT		**a_logent
			)
{
	/* Get from memory pool */
	if ( !(*a_logent = mempool_alloc(s_logent_pool, GFP_NOIO)) )
		{
		$LOG(KERN_ERR, "Error allocation memory for IOB ARGs");
		return	-ENOMEM;
		}

	/* Zeroing new block, adjust statistic counter, initialize a reference count	*/
	memset(*a_logent, 0, sizeof(LOGENT));

	INIT_LIST_HEAD(&(*a_logent)->link);
	return	0;
}

static inline void	s_free_logent (
		LOGENT		*a_logent
			)
{
	memset(a_logent, 0, sizeof(LOGENT));
	mempool_free(a_logent, s_logent_pool);
}

/*
 *  DESCRIPTION: An utility routine to put formated message string into the single linked list.
 *	Allocate SLAB chunk from pool, format message into the chunk's text area, add the
 *	entry at the tail of list.
 *
 *	An example of message (has been got with cat /dev/bmf0 ) :
 *	1594114400.424 [__bmf_create_filter:574] [0x4309d804ac70]Start filter for 52 f6 5c f0 ff c7 1b d4-da 86 b2 e2 67 eb e2 62
 *
 *  IMPLICITE INPUTS:
 *
 *
 *  IMPLICITE OUTPUTS:
 *	g_bmf0_mlist_nr
 *	g_bmf0_mlist
 *
 *  RETURNS:
 *	VMK API status code
 */
/* [13969.959264] [ 0x1b95 SER2NET\s_ser2net_ctxs_init:964] Created ser2net context for <ser2net00>, ser2net: 00000000605eec2b, task: 00000000535a117e */
/* printk(KERN_DEBUG  "[ %#x " __MODULE__ "\\%s:%d] " fmt "\n", current->tgid, __FUNCTION__,  __LINE__ , ## __VA_ARGS__); } while (0) */

static inline void	s_ser2net$ctl_putmsg(
		const char	*a_fmt,
		const char	*a_func,
			int	a_line,
			...
			)
{
int	l_rc;
va_list a_arglist;
LOGENT	*l_logent;
const char	l_lfmt [] = {"[%05lld.%06ld] [%#4x #%08llx " __MODULE__ "\\%s:%d] "};
static	atomic64_t	l_seq;

	if ( (l_rc = s_get_logent(&l_logent)) )					/* Get free entry for log message */
		return;

	ktime_get_ts64(&l_logent->ts);
	atomic64_add(1, &l_seq);


	/*
	 * To ignore follow unix shit!!!
	 * /sysman/Works/DOK/serial2net/serial2net.c:252: warning: format ‘%llx’ expects argument of type ‘long long unsigned int’, but argument 7 has type ‘atomic64_t’ [-Wformat=]
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
	l_logent->len  = snprintf(l_logent->text, LOGENT$SZ_MSG, l_lfmt, l_logent->ts.tv_sec, l_logent->ts.tv_nsec/(1024*1024),
				  current->tgid, l_seq, a_func, a_line);
#pragma GCC diagnostic pop


	l_logent->len = min(l_logent->len, LOGENT$SZ_MSG);

	/* Format a variable part of the message */
	va_start (a_arglist, a_line);
	l_logent->len  += vsnprintf(l_logent->text + l_logent->len, LOGENT$SZ_MSG - l_logent->len, a_fmt, a_arglist);
	va_end (a_arglist);

	l_logent->len = min(l_logent->len, LOGENT$SZ_MSG);


	spin_lock(&s_logent_list_lock);
	atomic64_add(1, &s_logent_list_nr);
	list_add_tail(&l_logent->link, &s_logent_list);
	spin_unlock(&s_logent_list_lock);

	return;
}

static	int s_ser2net$ctl_open(struct inode *a_inode, struct file *a_filp)
{
	return	0;
}

static	int s_ser2net$ctl_release(struct inode *a_inode, struct file *a_filp)
{
	return	0;
}


static	ssize_t s_ser2net$ctl_read(struct file *a_filp, char __user *a_outbuf, size_t a_outsz, loff_t *a_loff)
{
LOGENT	*l_logent;
int	l_rc;

	if ( a_outsz  < LOGENT$SZ_MSG )
		return	-ENOMEM;

	if ( (a_outsz - *a_loff) < LOGENT$SZ_MSG )
		return	*a_loff = 0;						/* End-of-file */

	if ( list_empty(&s_logent_list) )
		return	0;							/* Noging to do */


	l_rc = 0;

	spin_lock(&s_logent_list_lock);

	l_logent = list_first_entry(&s_logent_list,  LOGENT, link);

	if ( (a_outsz - *a_loff) >= l_logent->len )
		{
		l_rc  = l_logent->len;
		atomic64_sub(1, &s_logent_list_nr);
		list_del(&l_logent->link);
		}

	spin_unlock(&s_logent_list_lock);


	if ( l_rc > 0 )
		{
		memcpy(a_outbuf, l_logent->text,  l_rc);
		*a_loff += l_rc;
		s_free_logent(l_logent);
		}

	return	l_rc;
}


/*
 *   DESCRIPTION: Handling IOCTL request to the /dev/bmf0 character device;
 *	performs controlling functions by handling of the I/O CTL Request.
 *
 *   INPUTS:
 *	{tbs}
 *
 *   OUTPUTS:
 *	{tbs}
 *
 *   RETURNS:
 *	VMK API status code
 */
static long	s_ser2net$ctl_ioctl(
		struct file *	a_fp,
		unsigned int	a_cmd,
		unsigned long	a_arg
			)
{
#if 0
unsigned char *l_argp, *l_cp;
unsigned l_rc, l_opcode = _IOC_NR(a_cmd);

	$TRACE("fp = %p, cmd = %#x, arg = %#lx, opcode = %#x", a_fp, a_cmd, a_arg, l_opcode);

	switch (l_opcode)
		{
		case	SER2NET$K_CTL_TRACE:
			p_ser2net_trace = (a_arg == 0);
			$LOG(KERN_NOTICE, "arg = %lu, set TRACE mode to = %s", a_arg, p_ser2net_trace ? "ON" : "OFF");
			return	0;

		case	SER2NET$K_CTL_GETINFO:
			return	0;

		}

	$LOG(KERN_NOTICE, "Illegal or illformed IOCTL request = %#x, arg = %#x", a_cmd, (unsigned int) a_arg);
#endif
	return	-EOPNOTSUPP;
}



static	ssize_t s_ser2net$ctl_write(struct file *a_filp, const char __user *a__user, size_t a_size, loff_t *a_loff)
{
	return	-ENOTSUPP;
}

static	__poll_t s_ser2net$ctl_poll(struct file *a_filp, struct poll_table_struct *a_ptbl)
{
	if ( !list_empty(&s_logent_list) )
		return	POLLIN;

	return	0;
}









/*
 * Device Function Table for the character-like device (/dev/bmf0) is supposed to be used to
 * performs monitoring and control actions by using IOCTL calls and normal I/O like:
 *	$ cat /dev/bmf0 <ENTER>
 *
 */
struct file_operations	s_ser2net$ctl_ops =
{
	.unlocked_ioctl = s_ser2net$ctl_ioctl,
		.open	= s_ser2net$ctl_open,
		.release= s_ser2net$ctl_release,
		.poll	= s_ser2net$ctl_poll,
		.read	= s_ser2net$ctl_read,
		.write	= s_ser2net$ctl_write
};



static  int s_ser2net$ctl_init(void)
{
int	l_rc;

	if ( (l_rc = s_cre_logent_slab(LOGENT$SZ_DEFENT_NR, sizeof(LOGENT))))
		return	l_rc;


	/* Initialize a stuff to handling control function */
	s_ser2net$ctl.minor = MISC_DYNAMIC_MINOR;
	s_ser2net$ctl.name = SER2NET$K_CTLDEV;
	s_ser2net$ctl.fops = &s_ser2net$ctl_ops;

	if ( (l_rc  = misc_register(&s_ser2net$ctl)) )
		{
		$LOG(KERN_ERR, "Registerind 'misc' device failed (name=%s, minor=%d)->%d",
		       s_ser2net$ctl.name, s_ser2net$ctl.minor, l_rc);

		return	l_rc;
		}

	$PUTMSG("Control device is initialized");


	return	0;
}

static  int s_ser2net$ctl_shut(void)
{

	misc_deregister(&s_ser2net$ctl);
	s_del_logent_slab();

	return	0;
}




/*
 *   DESCRIPTION: A main thread to performs transmiting data from <ser2net> buffer to remote peer, and receiving data from remote peer
 *	to <net2ser> buffer
 *
 *   INPUTS:
 *	a_ser2net:	A context for serial device --- remote peer pair
 *
 *   OUTPUTS:
 *	NONE
 *
 *   RETURNS:
 *	NONE
 */
static void s_network$sk_data_ready (struct sock *a_sk)
{
SER2NET$CTX	*l_ctx = a_sk->sk_user_data;

	$PUTMSG("<%s>: Wake-up! a_sk: %p", l_ctx->devname, a_sk);
	wake_up_interruptible(&l_ctx->writeq);
}

static int s_network_proc(void *a_data)
{
SER2NET$CTX	*l_ctx = (SER2NET$CTX *) a_data;
struct socket *l_sock;
struct kvec l_iov;
struct msghdr l_msg;
int l_rc, l_len, l_do_loop_flag;
SER2NET$IOBUF	*l_iobuf;
char	*l_ptr;

	$PUTMSG("<%s>: Remote link: %pI4:%d --- worker thread is started", l_ctx->devname, &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port));


	for ( l_do_loop_flag = 0;  !kthread_should_stop(); l_do_loop_flag = 0 )
		{
		/*
		 * Is there data for remote host is configured ?
		 */
		if ( (!l_ctx->sk.sin_addr.s_addr) || (!l_ctx->sk.sin_port) )
			{
			/*
			 * Nothing to do, sleep for some time ...
			 */
			ssleep(1);
			continue;
			}

		/*
		 * Do we need to open a network connection ?
		 */
		if ( !(l_ctx->flags & SER2NET$M_NET_OPEN) )
			{
			if ( 0 > (l_rc = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &l_sock)) )
				{
				$LOG(KERN_ERR, "<%s>: sock_create_kern(%pI4:%d)->%d", l_ctx->devname,
				     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc );

				ssleep(3);
				continue;
				}

			$PUTMSG("<%s>: Connecting to: %pI4:%d ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port));

			if (0 > (l_rc = kernel_connect(l_sock, (struct sockaddr *)&l_ctx->sk, sizeof(struct sockaddr), O_RDWR) ) )
				{
				/*
				 * Error ? Ok, log this situation, sleep, and will try again ...
				 */
				$LOG(KERN_ERR, "<%s>: kernel_connect(%pI4:%d)->%d, retry after %d seconds ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc, 3);

				sock_release(l_sock);

				ssleep(3);
				continue;
				}
			else	{
				tcp_sock_set_nodelay(l_sock->sk);


				$PUTMSG("<%s>: Connected to: %pI4:%d, sock:%p", l_ctx->devname,
					&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock);

				l_ctx->flags |= SER2NET$M_NET_OPEN;
				l_sock->sk->sk_user_data = l_ctx;
				l_sock->sk->sk_data_ready = s_network$sk_data_ready;
				}
			}


		/*
		 * Do main I/O from serial's buffer to socket and back
		 */
		l_iobuf = &l_ctx->ser2net;
		//$PUTMSG("<%s>: l_iobuf[%p, head: %d, tail: %d]", l_ctx->devname, l_iobuf, l_iobuf->head, l_iobuf->tail);
		if ( (l_len = s_buf_data4get(l_iobuf, &l_ptr)) )		/* Is there something to send over network ? */
			{
			l_iov.iov_base = l_ptr;
			l_iov.iov_len  = l_len;

			memset(&l_msg, 0, sizeof(l_msg));

			if ( 0 > (l_rc = kernel_sendmsg(l_sock, &l_msg, &l_iov, 1, l_len)) )
				{
				$LOG(KERN_ERR, "<%s>: Sending to: %pI4:%d, sock: %p --- failed, errno: %d", l_ctx->devname,
						&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock, l_rc);
				}
			else	{
				s_buf_adjust_tail(l_iobuf, l_rc);
				l_do_loop_flag = 1;

				$PUTMSG("<%s>: Sent %d (from %d) octets", l_ctx->devname, l_rc, l_len);
				}
			}


		l_iobuf = &l_ctx->net2ser;
		//$PUTMSG("<%s>: l_iobuf[%p, head: %d, tail: %d]", l_ctx->devname, l_iobuf, l_iobuf->head, l_iobuf->tail);
		if ( (l_len = s_buf_free2put(l_iobuf, &l_ptr)) )		/* Is there something to send over network ? */
			{
			l_iov.iov_base = l_ptr;
			l_iov.iov_len  = l_len;

			memset(&l_msg, 0, sizeof(l_msg));

			if ( 0 < (l_rc = kernel_recvmsg(l_sock, &l_msg, &l_iov, 1, l_len, MSG_PEEK | MSG_DONTWAIT)) )
				{
				$PUTMSG("<%s>: l_rc: %d", l_ctx->devname, l_rc);

				if ( 0 > (l_rc = kernel_recvmsg(l_sock, &l_msg, &l_iov, 1, l_len, 0)) )
					{
					$LOG(KERN_ERR, "<%s>: Receiving from: %pI4:%d, sock: %p --- failed, errno: %d", l_ctx->devname,
							&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock, l_rc);
					}
				else	{
					s_buf_adjust_head(l_iobuf, l_rc);
					l_do_loop_flag = 1;
					wake_up_interruptible(&l_ctx->readq);

					$PUTMSG("<%s>: Rcvd %d octets", l_ctx->devname, l_rc);
					}
				}
			else	{
				if ( (l_rc == -EAGAIN) || (l_rc == -EWOULDBLOCK) )
					{
					/* It's OK, Just no data to read */
					}
				else	{
					if ( l_rc == 0 )
						{
						$PUTMSG("<%s>: Remote peer: %pI4:%d, sock: %p --- close connection", l_ctx->devname,
								&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock);
						}
					else	{
						$LOG(KERN_ERR, "<%s>: kernel_recvmsg(%pI4:%d, MSG_PEEK | MSG_DONTWAIT)->%d", l_ctx->devname,
							&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc);
						}

					sock_release(l_sock);
					l_ctx->flags &= (~SER2NET$M_NET_OPEN);
					}
				}
			}


		if ( l_do_loop_flag )
			continue;

		l_rc = wait_event_interruptible_timeout(l_ctx->writeq,
							(s_buf_len(&l_ctx->net2ser) > 0)
							, msecs_to_jiffies(5*1024));
		}

	if ( l_ctx->flags & SER2NET$M_NET_OPEN )
		sock_release(l_sock);

	$PUTMSG("<%s>: task: %p --- terminated", l_ctx->devname, l_ctx->net_task);

	return 0;
}






/*
 *   DESCRIPTION: A server thread. Start listener on specified <IP>:<port, wait and ccept network connection request,
 *	performs bidirectional data send\receive.
 *
 *   INPUTS:
 *	a_ser2net:	A context for serial device --- <IP>:<port> pair to bind socket
 *
 *   OUTPUTS:
 *	NONE
 *
 *   RETURNS:
 *	NONE
 */
static int s_network_hub_proc(void *a_data)
{
SER2NET$CTX	*l_ctx = (SER2NET$CTX *) a_data;
struct socket *l_sock, *l_sock_clnt;
struct kvec l_iov;
struct msghdr l_msg;
int l_rc, l_len, l_do_loop_flag;
SER2NET$IOBUF	*l_iobuf;
char	*l_ptr;

	$PUTMSG("<%s>: Listen on: %pI4:%d --- worker thread is started", l_ctx->devname, &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port));


	for ( l_do_loop_flag = 0;  !kthread_should_stop(); l_do_loop_flag = 0 )
		{
		/*
		 * Is there data for remote host is configured ?
		 */
		if ( (!l_ctx->sk.sin_addr.s_addr) || (!l_ctx->sk.sin_port) )
			{
			/*
			 * Nothing to do, sleep for some time ...
			 */
			ssleep(1);
			continue;
			}

		/*
		 * Do we need to open a network connection ?
		 */
		if ( !(l_ctx->flags & SER2NET$M_NET_OPEN) )
			{
			if ( 0 > (l_rc = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &l_sock)) )
				{
				$LOG(KERN_ERR, "<%s>: sock_create_kern(%pI4:%d)->%d", l_ctx->devname,
				     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc );

				ssleep(3);
				continue;
				}

		$PUTMSG("<%s>: Listening on: %pI4:%d ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port));


			if ( 0 > (l_rc = kernel_bind(l_sock, (struct sockaddr *)&l_ctx->sk, sizeof(struct sockaddr))) )
				{
				/*
				 * Error ? Ok, log this situation, sleep, and will try again ...
				 */
				$LOG(KERN_ERR, "<%s>: kernel_bind(%pI4:%d)->%d, retry after %d seconds ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc, 3);

				sock_release(l_sock);

				ssleep(3);
				continue;
				}

			if ( 0 > (l_rc = kernel_listen(l_sock, 1)) )
				{
				/*
				 * Error ? Ok, log this situation, sleep, and will try again ...
				 */
				$LOG(KERN_ERR, "<%s>: kernel_listen(%pI4:%d)->%d, retry after %d seconds ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc, 3);

				sock_release(l_sock);

				ssleep(3);
				continue;
				}

			if (0 > (l_rc = kernel_accept(l_sock, &l_sock_clnt, 0)) )
				{
				/*
				 * Error ? Ok, log this situation, sleep, and will try again ...
				 */
				$LOG(KERN_ERR, "<%s>: kernel_accept(%pI4:%d)->%d, retry after %d seconds ...", l_ctx->devname,
					     &l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc, 3);

				sock_release(l_sock);

				ssleep(3);
				continue;
				}
			else	{
				tcp_sock_set_nodelay(l_sock->sk);


				$PUTMSG("<%s>: Connected to: %pI4:%d, sock:%p", l_ctx->devname,
					&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock);

				l_ctx->flags |= SER2NET$M_NET_OPEN;
				l_sock->sk->sk_user_data = l_ctx;
				l_sock->sk->sk_data_ready = s_network$sk_data_ready;
				}
			}


		/*
		 * Do main I/O from serial's buffer to socket and back
		 */
		l_iobuf = &l_ctx->ser2net;
		//$PUTMSG("<%s>: l_iobuf[%p, head: %d, tail: %d]", l_ctx->devname, l_iobuf, l_iobuf->head, l_iobuf->tail);
		if ( (l_len = s_buf_data4get(l_iobuf, &l_ptr)) )		/* Is there something to send over network ? */
			{
			l_iov.iov_base = l_ptr;
			l_iov.iov_len  = l_len;

			memset(&l_msg, 0, sizeof(l_msg));

			if ( 0 > (l_rc = kernel_sendmsg(l_sock, &l_msg, &l_iov, 1, l_len)) )
				{
				$LOG(KERN_ERR, "<%s>: Sending to: %pI4:%d, sock: %p --- failed, errno: %d", l_ctx->devname,
						&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock, l_rc);
				}
			else	{
				s_buf_adjust_tail(l_iobuf, l_rc);
				l_do_loop_flag = 1;

				$PUTMSG("<%s>: Sent %d (from %d) octets", l_ctx->devname, l_rc, l_len);
				}
			}


		l_iobuf = &l_ctx->net2ser;
		//$PUTMSG("<%s>: l_iobuf[%p, head: %d, tail: %d]", l_ctx->devname, l_iobuf, l_iobuf->head, l_iobuf->tail);
		if ( (l_len = s_buf_free2put(l_iobuf, &l_ptr)) )		/* Is there something to send over network ? */
			{
			l_iov.iov_base = l_ptr;
			l_iov.iov_len  = l_len;

			memset(&l_msg, 0, sizeof(l_msg));

			if ( 0 < (l_rc = kernel_recvmsg(l_sock, &l_msg, &l_iov, 1, l_len, MSG_PEEK | MSG_DONTWAIT)) )
				{
				$PUTMSG("<%s>: l_rc: %d", l_ctx->devname, l_rc);

				if ( 0 > (l_rc = kernel_recvmsg(l_sock, &l_msg, &l_iov, 1, l_len, 0)) )
					{
					$LOG(KERN_ERR, "<%s>: Receiving from: %pI4:%d, sock: %p --- failed, errno: %d", l_ctx->devname,
							&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock, l_rc);
					}
				else	{
					s_buf_adjust_head(l_iobuf, l_rc);
					l_do_loop_flag = 1;
					wake_up_interruptible(&l_ctx->readq);

					$PUTMSG("<%s>: Rcvd %d octets", l_ctx->devname, l_rc);
					}
				}
			else	{
				if ( (l_rc == -EAGAIN) || (l_rc == -EWOULDBLOCK) )
					{
					/* It's OK, Just no data to read */
					}
				else	{
					if ( l_rc == 0 )
						{
						$PUTMSG("<%s>: Remote peer: %pI4:%d, sock: %p --- close connection", l_ctx->devname,
								&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_sock);
						}
					else	{
						$LOG(KERN_ERR, "<%s>: kernel_recvmsg(%pI4:%d, MSG_PEEK | MSG_DONTWAIT)->%d", l_ctx->devname,
							&l_ctx->sk.sin_addr, ntohs(l_ctx->sk.sin_port), l_rc);
						}

					sock_release(l_sock);
					l_ctx->flags &= (~SER2NET$M_NET_OPEN);
					}
				}
			}


		if ( l_do_loop_flag )
			continue;

		l_rc = wait_event_interruptible_timeout(l_ctx->writeq,
							(s_buf_len(&l_ctx->net2ser) > 0)
							, msecs_to_jiffies(5*1024));
		}

	if ( l_ctx->flags & SER2NET$M_NET_OPEN )
		sock_release(l_sock);

	$PUTMSG("<%s>: task: %p --- terminated", l_ctx->devname, l_ctx->net_task);

	return 0;
}



























/*
 *   DESCRIPTION: Allocate ser2net context for a caller.
 *	- lookup a <ser2net> context for a given <a_filep.path>
 *	- check status of the found context
 *	- put ser2net context into the a_filp.private_data
 *
 *   INPUTS:
 *	a_inode:
 *	a_filp:
 *
 *   OUTPUTS:
 *	a_filp:
 *
 *   RETURNS:
 *	<errno>
 */
static struct mutex s_open_close_mutex;
static int s_serial_open(struct inode *a_inode, struct file *a_filp)
{
int l_rc;
SER2NET$CTX	*l_ctx;

	//$PUTMSG("a_inode: %p, a_filep: %p, .path=<%s>, .private_data=%p", a_inode, a_filp,
	//       a_filp->f_path.dentry->d_name.name,
	//       a_filp->private_data);

	l_ctx = s_ser2net_ctxs;
	l_rc = -ENODEV;

	mutex_lock(&s_open_close_mutex);

	for (int i = 0; i < p_ser2net_max; i++, l_ctx++)
		{
		if ( !(memcmp(a_filp->f_path.dentry->d_name.name, l_ctx->devname, SER2NET$SZ_DEVNAME)) )
			{
			a_filp->private_data = l_ctx;

			if ( l_ctx->flags & SER2NET$M_SER_OPEN )
				{
				$PUTMSG("a_inode: %p, a_filep: %p, .path=<%s>, .private_data=%p --- already open by another process", a_inode, a_filp,
					       a_filp->f_path.dentry->d_name.name,
					       a_filp->private_data);

				l_rc = -EBUSY;
				break;
				}

			l_ctx->flags |= SER2NET$M_SER_OPEN;
			memset(&l_ctx->ser2net , 0, sizeof(l_ctx->ser2net));
			memset(&l_ctx->net2ser , 0, sizeof(l_ctx->net2ser));

			l_rc = 0;

			break;
			}
		}

	mutex_unlock(&s_open_close_mutex);

	if ( l_rc )
		$LOG(KERN_ERR, "SER2NET Device <%s> has not been found", a_filp->f_path.dentry->d_name.name);
	else	$PUTMSG( "Device <%s> is open, ser2net: %p", l_ctx->devname, l_ctx);

	return l_rc;
}

static int s_serial_release(struct inode *a_inode, struct file *a_filp)
{
SER2NET$CTX	*l_ctx;

	//$PUTMSG("a_inode: %p, a_filep: %p, .path=<%s>, .private_data=%p", a_inode, a_filp,
	//       a_filp->f_path.dentry->d_name.name,
	//       a_filp->private_data);

	mutex_lock(&s_open_close_mutex);

	if ( !(l_ctx = a_filp->private_data) )
		{
		mutex_unlock(&s_open_close_mutex);
		$LOG(KERN_ERR, "SER2NET Device <%s> has not been found", a_filp->f_path.dentry->d_name.name);
		return	-ENOLINK;
		}

	if ( (l_ctx->flags & SER2NET$M_SER_OPEN) )
		l_ctx->flags &=  (~SER2NET$M_SER_OPEN);

	mutex_unlock(&s_open_close_mutex);

	$PUTMSG( "Device <%s> is closed, ser2net: %p", l_ctx->devname, l_ctx);

	return 0;
}




/*
 *   DESCRIPTION: Read data has been received from network leg.
 *	Be advised that we use variant: single producer - cingle consumer, so we don't need lock anything
 *
 *   INPUTS:
 *	a_filp:
 *	a_buf:
 *	a_count
 *	a_f_pos:
 *
 *   OUTPUTS:
 *	NONE
 *
 *   RETURNS:
 *	>= 0:	A number of data has been returned into the <a_buf>
 *	< 0:	<errno>
 */
static ssize_t s_serial_read (
	struct file	*a_filp,
	char __user	*a_buf,
		size_t	a_count,
		loff_t	*a_f_pos
		)
{
SER2NET$CTX	*l_ctx;
SER2NET$IOBUF	*l_iobuf;
int	l_rc;
size_t	l_len;
char	*l_ptr;


	if ( !a_count )
		return 0;

	l_ctx = a_filp->private_data;
	l_iobuf = &l_ctx->net2ser;


	/* Wait until data is available */
	wait_event_interruptible(l_ctx->readq, (s_buf_data4get(l_iobuf, &l_ptr) > 0) );

	l_len = s_buf_data4get(l_iobuf, &l_ptr);
	l_len = min(l_len, a_count);

	if ( !l_len )
		return	0;

	if ( (l_rc = copy_to_user(a_buf, l_ptr, l_len)) )
		return	-EFAULT;

	s_buf_adjust_tail(l_iobuf, l_len);					/* Adjust <head> */


	$PUTMSG("<%s>: Read %ld octets", l_ctx->devname, l_len);

	return l_len;
}

static ssize_t s_serial_write (
		struct file	*a_filp,
	const char __user	*a_buf,
			size_t	a_count,
			loff_t	*a_f_pos
			)
{
SER2NET$CTX	*l_ctx;
SER2NET$IOBUF	*l_iobuf;
int	l_rc;
size_t	l_len;
char	*l_ptr;

	if ( !a_count )
		return 0;

	l_ctx = a_filp->private_data;
	l_iobuf = &l_ctx->ser2net;


	if ( !(l_len = s_buf_free2put(l_iobuf, &l_ptr)) )			/* Get pointer and size of free space */
		return	0;

	l_len = min(l_len, a_count);


	if ( (l_rc = copy_from_user(l_ptr, a_buf, l_len)) )			/* Get data into local buffer */
		return -EFAULT;

	s_buf_adjust_head(l_iobuf, l_len);					/* Adjust <head> */

	$PUTMSG("<%s>: Wrote %ld (from: %d) octets", l_ctx->devname, l_len, a_count);

	/*
	 * Send signal to weak-up a thread to send data over network
	 */
	wake_up_interruptible(&l_ctx->writeq);

	return l_len;
}



static __poll_t	s_serial_poll(struct file *a_filp, struct poll_table_struct *a_ptbl)
{
int	l_rc;
SER2NET$CTX	*l_ctx;
char	*l_ptr;

	l_ctx = s_ser2net_ctxs;

	if ( s_buf_free2put(&l_ctx->ser2net, &l_ptr) )			/* Check a free space in output buffer */
		l_rc = POLLOUT;

	if ( s_buf_data4get(&l_ctx->net2ser, &l_ptr) )			/* Check for data to be read from input buffer */
		l_rc = POLLIN;

	return	l_rc;
}



static const struct termios s_default_termios = {
    /* c_iflag – флаги ввода */
    .c_iflag = (IGNBRK |   /* игнорировать BREAK как сигнал */
		BRKINT |   /* генерировать SIGINT при BREAK */
		IGNPAR |   /* игнорировать паритетные ошибки */
		ICRNL  |   /* перевод CR в NL */
		IXON   |   /* включить XON/XOFF (output) */
		IXOFF),    /* включить XON/XOFF (input) */

    /* c_oflag – флаги вывода */
    .c_oflag = (OPOST |    /* включить постобработку вывода */
		ONLCR),    /* перевод NL в CR‑NL */

    /* c_cflag – флаги управления */
    .c_cflag = (CS8   |    /* 8‑битные данные */
		CREAD |    /* включить приём */
		HUPCL),    /* «hang up» при закрытии */

    /* c_lflag – локальные флаги */
    .c_lflag = (ICANON |  /* канонический ввод */
		ECHO   |  /* эхо вводимых символов */
		ECHOE  |  /* эхо стирания (ERASE) */
		ECHOK  |  /* эхо «kill line» */
		ECHONL |  /* эхо NL */
		ISIG),    /* обработка сигналов (INTR, QUIT, SUSP) */

    .c_line  = N_TTY,      /* стандартный line‑discipline */

    /* c_cc – управляющие символы (NCCS = 32) */
    .c_cc[VINTR]    = 0x03,   /* Ctrl‑C */
    .c_cc[VQUIT]    = 0x1C,   /* Ctrl‑\ */
    .c_cc[VERASE]   = 0x7F,   /* DEL */
    .c_cc[VKILL]    = 0x15,   /* Ctrl‑U */
    .c_cc[VEOF]     = 0x04,   /* Ctrl‑D */
    .c_cc[VTIME]    = 0,      /* таймаут (не используется в каноническом режиме) */
    .c_cc[VMIN]     = 1,      /* минимум 1 байт */
    .c_cc[VSWTC]    = 0,      /* (не используется) */
    .c_cc[VSTART]   = 0x11,   /* Ctrl‑Q */
    .c_cc[VSTOP]    = 0x13,   /* Ctrl‑S */
    .c_cc[VSUSP]    = 0x1A,   /* Ctrl‑Z */
    .c_cc[VEOL]     = 0,      /* (не используется) */
    .c_cc[VEOL2]    = 0,      /* (не используется) */
    .c_cc[VREPRINT] = 0x12,   /* Ctrl‑R */
    .c_cc[VDISCARD] = 0x0F,   /* Ctrl‑O */
    .c_cc[VWERASE]  = 0x17,   /* Ctrl‑W */
    .c_cc[VLNEXT]   = 0x16,   /* Ctrl‑V */
    .c_cc[VEOL]     = 0,      /* (повтор, оставлен для совместимости) */
};




static long	s_serial_ioctl (
		struct file *	a_filp,
		unsigned int	a_cmd,
		unsigned long	a_arg
			)
{
int	l_rc;
SER2NET$CTX	*l_ctx;
struct tty_struct *l_tty;

	l_ctx = a_filp->private_data;
	l_tty = &l_ctx->tty;

	switch (a_cmd)
		{
		case TCGETS:
			if ( copy_to_user((struct termios __user *)a_arg, &l_tty->termios, sizeof(struct termios)) )
				return -EFAULT;
			break;

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			if ( copy_from_user(&l_tty->termios, (void __user *)a_arg, sizeof(struct termios)) )
				return -EFAULT;

		       break;


		case TIOCMGET:
			l_rc = 0;

			if ( copy_to_user( (int __user *)a_arg,(l_rc = 0, &l_rc), sizeof(l_rc)))
				return -EFAULT;

			break;

		case TIOCGSERIAL:
		case TIOCSSERIAL:
		case FIONREAD:
			break;

		//default:
		//	return -ENOTTY;
		}

	return	0;
}

static const struct file_operations s_serial_fops = {
		.open	= s_serial_open,
		.release= s_serial_release,
		.read	= s_serial_read,
		.write	= s_serial_write,
		.poll	= s_serial_poll,
	.unlocked_ioctl	= s_serial_ioctl
};

static int s_ser2net_ctxs_init (void)
{
int l_rc;
SER2NET$CTX	*l_ctx;

	l_ctx = s_ser2net_ctxs;

	s_class = class_create(SER2NET$T_DEVNAME);

	if (IS_ERR(s_class))
		{
		l_rc = PTR_ERR(s_class);
		class_destroy(s_class);

		$LOG(KERN_ERR, "class_create(%s)->%d", SER2NET$T_DEVNAME, l_rc);
		return	l_rc;
		}


	for ( int i = 0; i < p_ser2net_max; i++, l_ctx++)
		{
		/*
		 * Generate device name like: ser2net00, ser2net01 ...
		 */
		l_rc = snprintf(l_ctx->devname, SER2NET$SZ_DEVNAME, SER2NET$T_DEVNAME "%02d", i);

		if ( (l_rc = alloc_chrdev_region(&l_ctx->devt, 0, 1, SER2NET$T_DEVNAME)) )
			break;

		cdev_init(&l_ctx->scdev, &s_serial_fops);
		l_ctx->scdev.owner = THIS_MODULE;

		if ( (l_rc = cdev_add(&l_ctx->scdev, l_ctx->devt, 1)) )
			break;


		if ( IS_ERR(device_create(s_class, NULL, l_ctx->devt, l_ctx, l_ctx->devname)) )
			{
			l_rc = -ENOMEM;
			cdev_del(&l_ctx->scdev);
			break;
			}


		init_waitqueue_head(&l_ctx->readq);
		init_waitqueue_head(&l_ctx->writeq);

#if 0
		{/* Just for debug purpose set parameters for ser2net00 link */
		if ( i == 0 )
			{
			l_ctx->sk.sin_addr.s_addr = $IPv4_ADDR(127, 0, 0, 1);
			l_ctx->sk.sin_family = AF_INET;
			l_ctx->sk.sin_port = htons(8135);
			}
#endif

		/*
		 * Create worker thread to hand network I/O operations
		 */
		l_ctx->net_task = kthread_run(s_network_proc, l_ctx, l_ctx->devname);
		if (IS_ERR(l_ctx->net_task))
			{
			l_rc = PTR_ERR(l_ctx->net_task);
			$LOG(KERN_ERR, "kthread_run(%p, %s)->%d", l_ctx, l_ctx->devname, l_rc);
			break;
			}

		$LOG(KERN_INFO, "Created ser2net context for <%s>, ser2net: %p, task: %p", l_ctx->devname, l_ctx, l_ctx->net_task);
		}


	return	l_rc;
}




static int __init s_module_init(void)
{
int	l_rc;
char	*l_cp;

	$LOG(KERN_INFO, __MODULE__ " Rev: " __IDENT__ " %s", s_author_desc);

	$SHOW_INT(p_ser2net_trace);
	$SHOW_INT(p_ser2net_max);
	$SHOW_INT(p_ser2net_peer_nr);

	for (int i = 0; i < p_ser2net_peer_nr; i++)
		{
		if ( !(l_cp = strnchr(p_ser2net_peer[i], 32, ':')) )		/* Expect <IP>:<port> */
			{
			$LOG(KERN_ERR, "Illformed/illegal end-point: <%s> --- expect: <IP>:<port> pair", p_ser2net_peer[i]);
			return	-EINVAL;
			}

		l_rc = l_cp - p_ser2net_peer[i];				/* Convert IP-address part into internal representative */
		*l_cp = '\0';

		if ( !(s_ser2net_ctxs[i].sk.sin_addr.s_addr = in_aton(p_ser2net_peer[i])) )
			{
			$LOG(KERN_ERR, "Illformed/illegal end-point: <%s> --- expect: <IP>:<port> pair", p_ser2net_peer[i]);
			return	-EINVAL;
			}

		l_cp += 1;							/* Convert TCP-port part to NBO */

		if ( (kstrtouint(l_cp, 10, &l_rc)) )
			{
			$LOG(KERN_ERR, "Illformed/illegal end-point: <%s> --- expect: <IP>:<port> pair", p_ser2net_peer[i]);
			return	l_rc;
			}

		s_ser2net_ctxs[i].sk.sin_port =  htons(l_rc);
		s_ser2net_ctxs[i].sk.sin_family = AF_INET;
		s_ser2net_ctxs[i].flags |= (SER2NET$M_NET_HUB & p_ser2net_hub);
		}


	if ( (l_rc = s_ser2net$ctl_init()) )
		{
		$LOG(KERN_ERR, "Initialization of control device failed, errno: %d", l_rc);
		return	l_rc;
		}

	if ( (l_rc = s_ser2net_ctxs_init()) )
		{
		$LOG(KERN_ERR, "Initialization of context failed, errno: %d", l_rc);
		return	l_rc;
		}


	$LOG(KERN_INFO, "%d contexts is initialized", p_ser2net_max);

	return	0;
}


static void __exit s_module_exit(void)
{
SER2NET$CTX	*l_ctx;
int	l_rc;

	l_ctx = s_ser2net_ctxs;

	for ( int i = 0; i < p_ser2net_max; i++, l_ctx++)
		{
		device_destroy(s_class, l_ctx->devt);

		cdev_del(&l_ctx->scdev);
		unregister_chrdev_region(l_ctx->devt, 1);

		if ( l_ctx->net_task && (l_rc = kthread_stop(l_ctx->net_task)) )
			$LOG(KERN_ERR, "kthread_stop(<%s:%p)->%d", l_ctx->devname, l_ctx->net_task, l_rc);

		}

	class_destroy(s_class);
	s_ser2net$ctl_shut();

	$LOG(KERN_INFO, "Unloaded");
}

module_init(s_module_init);
module_exit(s_module_exit);
