#	SER2NET - A yet another Linux driver to pass serial-like port over network

###	Abstract
*This is a trivial Linux driver implemented serial-like device. Pass all I/O as-is over TCP-connection to
a remote "server". Remote server is a something like StarLet MODBUS TCP-to-RTU gateway or StarLet NET-to-SERIAL gateway.*


###	NaPi World
This project is developed and maintained by the NaPi Lab team
and is primarily tested on NaPi industrial single-board computers based on Rockchip SoCs.

####	NaPi Boards
If you are looking for a reliable hardware platform to run MODBUS transport sowftware in production,
check out the NaPi board lineup:

Welcome to NaPi Wolrd (https://github.com/napilab/napi-boards) for more information!

Right now is available:

- NAPI2 — RK3568J, RS-485 onboard, Armbian
- NAPI-C — RK3308, compact, industrial grade



###	Usage
```
$ insmod ./serial2net.ko [p1=<value] [p2=<value] ...
```

###	Parameters
| Option			|  Description
| ------			| ------------------------------------------------------------
| p_ser2net_max=\<number\>	| A number of serial devices, p_ser2net_max=4 will create /dev/ser2net00 ... /dev/ser2net03
| p_ser2net_trace		| Enable extensible diagnostic output for debug and troubleshooting purpose
| p_ser2net_peer		| A coma-separated list of host:port pairs, should be corresponding to number of \<ser2netXX\> devices.
| p_ser2net_hub			| Switch driver to Hub\Server mode, in this case \<p_ser2net_peer\> defines  local end-points (TCP-listeners) to accept remote connections.

###	Example of usage
####	Load LKM to create 2 pseduo-serial devices:
```
insmod ./serial2net.ko p_ser2net_max=2 p_ser2net_peer="127.0.0.1:8135,172.16.1.1:502"
```

####	Check presence
```
root@devuan4-sysman:/sysman/Works/DOK/serial2net# ls -l  /dev/ser*
crw------- 1 root root  10, 262 Mar 25 13:18 /dev/ser2net
crw------- 1 root root 242,   0 Mar 25 13:18 /dev/ser2net00
crw------- 1 root root 241,   0 Mar 25 13:18 /dev/ser2net01
```


####	Check system's log file for related diagnostic
```
$ dmesg

[186795.162045] [ 0xa4f3 SER2NET\s_module_init:1342] SER2NET Rev: X.00-03 Ruslan R. Laishev (AKA: BadAss Sysman, SysManOne), zator@yandex.ru
[186795.163529] [ 0xa4f3 SER2NET\s_module_init:1344] SER2NET : p_ser2net_trace = 1
[186795.164417] [ 0xa4f3 SER2NET\s_module_init:1345] SER2NET : p_ser2net_max = 2
[186795.165320] [ 0xa4f3 SER2NET\s_module_init:1346] SER2NET : p_ser2net_peer_nr = 2
[186795.168464] [ 0xa4f3 SER2NET\s_ser2net_ctxs_init:1327] Created ser2net context for <ser2net00>, ser2net: 00000000fa6cb968, task: 00000000c4dd2831
[186795.168552] [ 0xa4f6 SER2NET\s_network_proc:585] <ser2net00>: kernel_connect(127.0.0.1:8135)->-111, retry after 3 seconds ...
[186795.170852] [ 0xa4f3 SER2NET\s_ser2net_ctxs_init:1327] Created ser2net context for <ser2net01>, ser2net: 00000000b459f9c4, task: 00000000c1257e13
[186795.173829] [ 0xa4f3 SER2NET\s_module_init:1392] 2 contexts is initialized
[186798.348028] [ 0xa4f6 SER2NET\s_network_proc:585] <ser2net00>: kernel_connect(127.0.0.1:8135)->-111, retry after 3 seconds ...
[186801.430749] [ 0xa4f6 SER2NET\s_network_proc:585] <ser2net00>: kernel_connect(127.0.0.1:8135)->-111, retry after 3 seconds ...
```

####	Check driver's internal tracing
```
root@devuan4-sysman:/sysman/Works/DOK/serial2net# tail -f /dev/ser2net
[186952.000839] [0xa4f6 #0000003e SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186955.000914] [0xa4f6 #0000003f SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186959.000028] [0xa4f6 #00000040 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186962.000094] [0xa4f6 #00000041 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186964.000630] [0xa4f7 #00000042 SER2NET\s_network_proc:577] <ser2net01>: Connecting to: 172.16.1.1:502 ...
[186965.000161] [0xa4f6 #00000043 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186968.000232] [0xa4f6 #00000044 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186971.000300] [0xa4f6 #00000045 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186974.000365] [0xa4f6 #00000046 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186977.000438] [0xa4f6 #00000047 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186980.000504] [0xa4f6 #00000048 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186983.000571] [0xa4f6 #00000049 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186986.000638] [0xa4f6 #0000004a SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186988.000694] [0xa4f7 #0000004b SER2NET\s_network_proc:577] <ser2net01>: Connecting to: 172.16.1.1:502 ...
[186989.000726] [0xa4f6 #0000004c SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186992.000775] [0xa4f6 #0000004d SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186995.000849] [0xa4f6 #0000004e SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[186998.000924] [0xa4f6 #0000004f SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187002.000031] [0xa4f6 #00000050 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187005.000095] [0xa4f6 #00000051 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187008.000169] [0xa4f6 #00000052 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187011.000233] [0xa4f6 #00000053 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187012.000748] [0xa4f7 #00000054 SER2NET\s_network_proc:577] <ser2net01>: Connecting to: 172.16.1.1:502 ...
[187014.000304] [0xa4f6 #00000055 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187017.000371] [0xa4f6 #00000056 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187020.000458] [0xa4f6 #00000057 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
[187023.000523] [0xa4f6 #00000058 SER2NET\s_network_proc:577] <ser2net00>: Connecting to: 127.0.0.1:8135 ...
```


###	Notes
- `/dev/ser2net` is a pseudo-device with the messages of the driver (`tail -f /dev/ser2net`). Since X.00-08ECO1 it keeps at most 4096 messages
  when nobody reads it, the next ones are dropped (before, the queue was unbounded and could eat all the memory of a small board).
- The remote end can be a RAW listener of the `mbusgw-t2r` gateway (`mode = "RAW"`): a program opens `/dev/ser2net00` (`mbpoll -m rtu`, `minicom`,
  `pyserial`) and works with the serial line of the gateway as with a local port. The speed and the parity are taken from the gateway.
- Tested on NAPI-C (kernel 7.3.0-rc7-edge): 1 MiB each way in blocks with CRC32 (no loss), four ports at once, 115200.


###	Coding style
The driver is written in the OpenVMS C style (the "VMS style"), a patch is expected to follow it:

- Whitesmiths braces (a brace on its own line, indented together with the body; the braces of a routine body are not indented), only tabs
  (8 columns), no spaces, lines up to 192 columns.
- Prefixes: `g_` global, `l_` local, `s_` static (routines too), `a_` routine arguments, `m_` bit masks, `v_` bit fields.
  `$` separates the words of macros and constants (`SER2NET$K_SEQ_CUR`); structures get the `_t` suffix.
- A file starts with `__MODULE__`, `__IDENT__` and `__REV__` (`X.00-08ECO1` is `00.08.01`), then a VMS-style header: the facility, the abstract,
  the usage and the modification history. Every revision is recorded there.
- It is kernel code, so the messages go through the macros of the module itself: `$LOG` (printk), `$TRACE` (printk, only if `p_ser2net_trace`)
  and `$PUTMSG` (the queue of `/dev/ser2net`), not through bare `printk` calls. A routine returns a status, the results go through the output
  parameters, and the error value (`errno`) is always logged.
