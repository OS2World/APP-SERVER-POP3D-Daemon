POP3D - a simple POP3 daemon for OS/2
-------------------------------------

This is a very simple and basic POP3 daemon, which will accept
connections on the usual POP3 port (110) and distribute mail.  It is not
sophisticated, but it is small and fast. 

All companion programs are available from the same place:

     http://www.tavi.co.uk/os2pages/mail.html

User authorisation
------------------

This version of POP3D normally performs user authorisation using the
User Profile Management services in OS/2.  These will be installed if
you have LAN Requester or Peer Networking installed; check the System
Setup folder for the UPM Services folder.  User names are limited by
UPM, to the characters A-Z 0-9 # @ $ only.  Lower case is forced to
upper case, so you can use either.  Usernames and passwords are limited
to 8 characters in length. 

If you don't have UPM (or don't want to use it), you can use the dummy
authorisation module included in this package.  This permits all
user/password combinations, performing no checking. 

Installation
------------

Copy the POP3D.EXE file to any suitable directory which is named in the
system PATH. Copy the NETLIB.DLL file to any directory on the LIBPATH.

Next, choose an authorisation module.  If you are using UPM
authorisation, use AUTH_UPM.DLL.  If you want to use the dummy
authorisation module (accepting all username and password combinations),
use AUTH_ALT.DLL.  Copy your chosen DLL to any directory on the LIBPATH,
but rename it (in either case) to POP3AUTH.DLL. 

Configuration
-------------

First, ensure that you have a line in CONFIG.SYS of the form:

     SET TZ=....

This defines your time zone setting, names, and daylight saving rules. 
If you don't have one, you need to add it; the actual value can be quite
complex.  For the United Kingdom, the line is:

     SET TZ=GMT0BST,3,-1,0,3600,10,-1,0,7200,3600

For other areas, download the TZCALC utility.  This will work out the
correct setting for you.  It will be necessary to reboot in order to
pick up this setting, but wait until you have completed the rest of
these instructions. 

Now edit CONFIG.SYS, adding a new line of the form:

     SET POP3=directoryname

where 'directoryname' is the name of a directory (which must exist)
where incoming mail is stored.  You will also need to create a
subdirectory for each user for whom mail is to be collected.  The name
of the directory should be the same as the username to be used for
collection.  If you are using a FAT partition, then the first 11
characters of the username are used, forced to upper case (if there are
illegal characters in the username then this fails).  All in all,
though, HPFS or JFS are better solutions.

It is assumed that something else (probably the companion POP3 program)
will have stored mail messages into each user's 'post office' directory. 
The choice of username is of course up to you. 

It will be necessary to reboot in order to pick up the POP3 setting, but
wait until you have completed the rest of these instructions. 

Locate the directory described by the ETC environment variable.  If you
are not sure, type the command:

     SET ETC

at a command prompt.  Note that this is where the logfile (see below) is
stored. 

Lastly, edit the file INETD.LST, also found in the ETC directory.  Add a
line like this:

     pop3 tcp pop3d

You can do this via the TCP/IP configuration notebook if you prefer.  If
INETD is not already running, edit the file TCPSTART.CMD (normally found
in \TCPIP\BIN) to un-comment the line that starts INETD; again, use the
TCP/IP configuration notebook if you prefer.  Reboot to activate INETD,
and also to pick up the POP3 environment setting (and the TZ one if you
added it).  INETD will now accept incoming POP3 calls and start POP3D as
necessary. 

Logfile
-------

POP3D maintains a logfile called POP3D.LOG in the ETC directory.  This
will grow without bound if not pruned regularly! But it is occasionally
useful.... 

Feedback
--------

POP3D was written by me, Bob Eager. I can be contacted at rde@tavi.co.uk.

Please let me know if you actually use this program.  Suggestions for
improvements are welcomed. 

History
-------

1.0	Initial version.
1.1	Implemented TOP command (needed by Post Road Mailer preview).
	Fixed problem with deleted messages not being detected as
	deleted by RETR.
1.2	Use new thread-safe logging module.
	Server will now continue if log file cannot be opened.
	Use OS/2 type definitions.
	New, simplified network interface module.
	Converted to use 32-bit UPM library.
	Grouped initialisation code together.
1.3	Moved 'addsockettolist' call to after sock_init().
2.0	Added BLDLEVEL string.
	Diagnostics for occasional logfile open failures.
3.0	Recompiled using 32-bit TCP/IP toolkit, in 16-bit mode.
	Added support for using long filenames on JFS.
	Removed redundant 'addsockettolist' declaration (it is now
	properly defined and documented in the toolkit).
	Moved authorisation support to separate DLL; multiple
	authorisation DLLs are now possible; dummy one provided.

Bob Eager
rde@tavi.co.uk
June 2002

