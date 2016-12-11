/*
 * File: pop3d.c
 *
 * POP3D daemon for distributing mail on Tavi network; to be invoked only
 * by INETD.
 *
 * Main program
 *
 * Bob Eager   June 2002
 *
 */

/*
 * History:
 *	1.0	Initial version.
 *	1.1	Implemented TOP command (needed by Post Road Mailer preview).
 *		Fixed problem with deleted messages not being detected as
 *		deleted by RETR.
 *	1.2	Use new thread-safe logging module.
 *		Server will now continue if log file cannot be opened.
 *		Use OS/2 type definitions.
 *		New, simplified network interface module.
 *		Converted to use 32-bit UPM library.
 *		Grouped initialisation code together.
 *	1.3	Moved 'addsockettolist' call to after sock_init().
 *	2.0	Added BLDLEVEL string.
 *		Diagnostics for occasional logfile open failures.
 *	3.0	Recompiled using 32-bit TCP/IP toolkit, in 16-bit mode.
 *		Added support for using long filenames on JFS.
 *		Removed redundant 'addsockettolist' declaration (it is now
 *		properly defined and documented in the toolkit).
 *		Moved authorisation support to separate DLL; multiple
 *		authorisation DLLs are now possible.
 *
 */

#pragma	strings(readonly)

#pragma	alloc_text(a_init_seg, main)
#pragma	alloc_text(a_init_seg, error)
#pragma	alloc_text(a_init_seg, fix_domain)
#pragma	alloc_text(a_init_seg, log_connection)

#define	OS2
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <types.h>
#include <sys\socket.h>
#include <netdb.h>
#include <netinet\in.h>
#include <arpa\nameser.h>
#include <resolv.h>

#include "pop3d.h"
#include "log.h"

#define	LOGFILE		"POP3D.Log"	/* Name of log file */
#define	LOGENV		"ETC"		/* Environment variable for log dir */
#define	POP3DIR		"POP3"		/* Environment variable for spool dir */

/* Type definitions */

typedef	struct hostent		HOST, *PHOST;		/* Host structure */
typedef struct in_addr		INADDR, *PINADDR;	/* Internet address */
typedef	struct sockaddr		SOCKG, *PSOCKG;		/* Generic structure */
typedef	struct sockaddr_in	SOCK, *PSOCK;		/* Internet structure */

/* Forward references */

static	VOID	fix_domain(PUCHAR);
static	VOID	log_connection(VOID);

/* Local storage */

static	UCHAR 	hostname[MAXHOSTNAMELEN+1];
static	UCHAR 	myname[MAXHOSTNAMELEN+1];
static	PUCHAR	progname;


/*
 * Parse arguments and handle options.
 *
 */

INT main(INT argc, PUCHAR argv[])
{	INT sockno, namelen, rc;
	SOCK client;
	PHOST host;
	PUCHAR p;

	progname = strrchr(argv[0], '\\');
	if(progname != (PUCHAR) NULL)
		progname++;
	else
		progname = argv[0];
	p = strchr(progname, '.');
	if(p != (PUCHAR) NULL) *p = '\0';
	strlwr(progname);

	tzset();			/* Set time zone */
	res_init();			/* Initialise resolver */

	if(argc != 2) {
		error("usage: %s sockno", progname);
		exit(EXIT_FAILURE);
	}

	sockno = atoi(argv[1]);
	if(sockno <= 0) {
		error("bad arg from INETD");
		exit(EXIT_FAILURE);
	}

	rc = sock_init();		/* Initialise socket library */
	if(rc != 0) {
		error("INET.SYS not running");
		exit(EXIT_FAILURE);
	}

	addsockettolist(sockno);	/* Ensure socket belongs to us now */

	/* Get IP address of the client */

	client.sin_family = AF_INET;
	namelen = sizeof(SOCK);
	rc = getpeername(sockno, (PSOCKG) &client, &namelen);
	if(rc != 0) {
		error("cannot get peer name, errno = %d", sock_errno());
		exit(EXIT_FAILURE);
	}

	/* Get the host name of this server; if not possible, set it to the
	   dotted address. */

	rc = gethostname(myname, sizeof(myname));
	if(rc != 0) {
		INADDR myaddr;

		myaddr.s_addr = htonl(gethostid());
		sprintf(myname, "[%s]", inet_ntoa(myaddr));
	} else {
		fix_domain(myname);
	}


	/* Get the host name of the client; if not possible, set it to the
	   dotted address */

	host = gethostbyaddr((PUCHAR) &client.sin_addr,
			     sizeof(client.sin_addr), AF_INET);
	if(host == (PHOST) NULL) {
		if(h_errno == HOST_NOT_FOUND) {
			sprintf(hostname, "[%s]", inet_ntoa(client.sin_addr));
		} else {
			error("cannot get host name, errno = %d", h_errno);
			exit(EXIT_FAILURE);
		}
	} else {
		strcpy(hostname, host->h_name);
		fix_domain(hostname);
	}

	/* Start logging */

	rc = open_logfile(LOGENV, LOGFILE);
	if(rc != LOG_OK) {
		error("logging initialisation failed - %s",
		rc == LOG_NOENV ? "environment variable "LOGENV" not set" :
				  "file open failed");
	}

	log_connection();

	/* Run the server */

	rc = server(sockno, hostname, myname, POP3DIR);

	/* Shut down */

	(VOID) soclose(sockno);
	close_logfile();

	return(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}


/*
 * Print message on standard error in printf style,
 * accompanied by program name.
 *
 */

VOID error(PUCHAR mes, ...)
{	va_list ap;

	fprintf(stderr, "%s: ", progname);

	va_start(ap, mes);
	vfprintf(stderr, mes, ap);
	va_end(ap);

	fputc('\n', stderr);
}


/*
 * Check for a full domain name; if not present, add default domain name.
 *
 */

static VOID fix_domain(PUCHAR name)
{	if(strchr(name, '.') == (PUCHAR) NULL && _res.defdname[0] != '\0') {
		strcat(name, ".");
		strcat(name, _res.defdname);
	}
}


/*
 * Log details of the connection to standard output and to the logfile.
 *
 */

static VOID log_connection(VOID)
{	time_t tod;
	UCHAR timeinfo[35];
	UCHAR buf[100];

	(VOID) time(&tod);
	(VOID) strftime(timeinfo, sizeof(timeinfo),
		"on %a %d %b %Y at %X %Z", localtime(&tod));
	sprintf(buf, "%s: connection from %s, %s",
		progname, hostname, timeinfo);
	fprintf(stdout, "%s\n", buf);

	sprintf(buf, "connection from %s", hostname);
	dolog(buf);
}


/*
 * Allocate memory using 'malloc'; terminate with a message
 * if allocation failed.
 *
 */

PVOID xmalloc(size_t size)
{	PVOID res;

	res = malloc(size);

	if(res == (PVOID) NULL)
		error("cannot allocate memory");

	return(res);
}

/*
 * End of file: pop3d.c
 *
 */
