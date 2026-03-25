#	SER2NET - A yet another Linux driver to pass serial-like port over network

###	Abstract
*This is a trivial Linux driver implements serial-like device, pass all I/O as-is over TCP-connection to
a remote "server". Remote server is a something like StarLet MODBUS TCP-to-RTU gateway or StarLet NET-to-SERIAL gateway.*

###	Usage
```
$ insmod ./serial2net.ko [p1=<value] [p2=<value] ...
```

###	Parameters
| Option		|  Description
| ------		| ------------------------------------------------------------
| trace			| Enable extensible diagnostic output. Useful for for debug and troubleshouting purpose.
| logfile=\<fpsec\>	| Set a file name to accept logging output
| logsize=\<number\>	| Limit size of log file.
| settings=\<fspec\>	| Provide a rin-time configuration for network stuff and serial devices!
