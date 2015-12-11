# TFTP Client for use on VMWare ESXi

This code is customization of the tftp client found at https://github.com/lanrat/tftp. For the rest of the code, that is needed for this client to compile, head over to [lanrat](https://github.com/lanrat/tftp).

tftpclient.c has been rewritten to allow reading hostnames from the command line. It also now does not wipe files on local disk when they are not found on the tftp server. For my use this has been an necessary addition of functionality.

The old code used a constant defined in tftp.h. The constant is still there, but gets overridden if -h address is specified as a argument to the program. This code is only tested with the use if IP-Addressesi, and not DNS names. This code is compiled in Linux on Centos 6.7 to be usable with ESXi.

To build this code to an executable, install build-essential for the tools you need:
yum install build-essential gcc make

It works fine on ESXi when it is compiled as statitically linked:
make CFLAGS="-static" EXEEXT="-static"

Just make sure the firewall on the ESXi is disabled:
esxcli network firewall set --enabled no

*- Atle Holm - December 2015*