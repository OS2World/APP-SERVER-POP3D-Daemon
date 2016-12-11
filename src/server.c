/*
 * File: server.c
 *
 * POP3 daemon for distributing mail on Tavi network; to be invoked only
 * by INETD.
 *
 * Protocol handler for server
 *
 * Bob Eager   June 2002
 *
 */

#pragma	strings(readonly)

#pragma	alloc_text(a_init_seg, server)
#pragma	alloc_text(a_init_seg, greeting)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "pop3d.h"
#include "cmds.h"
#include "maildrop.h"
#include "auth.h"
#include "netio.h"
#include "log.h"

#define	MAXCMD		514		/* Maximum length of a command */
#define	MAXLINE		1002		/* Maximum length of line */
#define	MAXREPLY	514		/* Maximum length of reply message */
#define	CMD_TIMEOUT	10*60		/* Command timeout (secs) */
#define	DATA_TIMEOUT	10*60		/* Data timeout (secs) */
#define	MSG_TIMEOUT	5		/* Fatal message write timeout (secs) */

/* Type definitions */

typedef	enum	{ ST_CONNECT, ST_AUTH1, ST_AUTH2, ST_TRANSACTION, ST_UPDATE }
	STATE;

/* Forward references */

static	BOOL	do_dele(INT, PUCHAR, INT);
static	BOOL	do_list(INT, PUCHAR, INT);
static	BOOL	do_login(INT);
static	BOOL	do_pass(INT, PUCHAR, INT);
static	VOID	do_quit(INT, PUCHAR);
static	BOOL	do_retr(INT, PUCHAR, INT);
static	VOID	do_rset(INT);
static	VOID	do_stat(INT);
static	BOOL	do_top(INT, PUCHAR, INT);
static	BOOL	do_user(INT, PUCHAR, INT);
static	VOID	expect_auth(INT);
static	INT	getcmd(PUCHAR);
static	VOID	greeting(INT, PUCHAR);
static	VOID	net_read_error(INT, PUCHAR);
static	VOID	net_read_timeout(INT, PUCHAR);
static	VOID	process_commands(INT, PUCHAR, PUCHAR);

/* Local storage */

static	UCHAR	logmsg[MAXREPLY];	/* Logging buffer */
static	STATE	state;			/* Internal state */
static	PUCHAR	password;		/* Pointer to stored password */
static	PUCHAR	username;		/* Pointer to stored user name */


/*
 * Do the conversation between the server and the client.
 *
 * Returns:
 *	TRUE		server ran and terminated
 *	FALSE		server failed to start
 *
 */


BOOL server(INT sockno, PUCHAR clientname, PUCHAR servername, PUCHAR direnv)
{	INT rc;

	username = (PUCHAR) NULL;
	password = (PUCHAR) NULL;

	state = ST_CONNECT;

	rc = mail_init(direnv);
	switch(rc) {
		case MAILINIT_OK:
			break;

		case MAILINIT_NOENV:
			error("environment variable %s not set", direnv);
			return(FALSE);

		case MAILINIT_BADDIR:
			error("cannot access mail storage directory");
			return(FALSE);

		default:
			error("mail storage initialisation failed");
			return(FALSE);
	}

	if(netio_init() == FALSE) {
		error("network initialisation failure");
		return(FALSE);
	}

	greeting(sockno, servername);
	state = ST_AUTH1;

	process_commands(sockno, clientname, servername);

	return(TRUE);
}


/*
 * Process POP3 commands.
 *
 */

static VOID process_commands(INT sockno, PUCHAR clientname, PUCHAR servername)
{	INT cmd, len, i;
	INT cmdlen;
	UCHAR cmdbuf[MAXCMD+1];

	for(;;) {
		len = sock_gets(cmdbuf, sizeof(cmdbuf), sockno, CMD_TIMEOUT);
		if(len == SOCKIO_ERR) {
			net_read_error(sockno, servername);
			return;
		}
		if(len == SOCKIO_TIMEOUT) {
			net_read_timeout(sockno, servername);
			return;
		}
		if(len == SOCKIO_TOOLONG) {
			sock_puts("-ERR Line too long\n", sockno, CMD_TIMEOUT);
			continue;
		}

		cmd = getcmd(cmdbuf);
		cmdlen = strlen(cmdtab[cmd-1].cmdname);
		for(i = 0; i < cmdlen; i++)
			cmdbuf[i] = toupper(cmdbuf[i]);
#ifdef	DEBUG
		trace(cmdbuf);
#endif

		switch(cmd) {
			case USER:
				if(state == ST_AUTH1) {
					if(do_user(sockno, cmdbuf, cmdlen) == TRUE) {
						state = ST_AUTH2;
						sock_puts(
							"+OK enter password\n",
							sockno, CMD_TIMEOUT);
					}
				} else {
					sock_puts(
						"-ERR USER not acceptable now\n",
						sockno, CMD_TIMEOUT);
				}
				break;

			case PASS:
				if(state == ST_AUTH2) {
					if(do_pass(sockno, cmdbuf, cmdlen) == TRUE) {
						if(do_login(sockno) == TRUE)
							state = ST_TRANSACTION;
						else
							state = ST_AUTH1;
					} else {
						state = ST_AUTH1;
					}
				} else {
					sock_puts(
						"-ERR PASS not acceptable now\n",
						sockno, CMD_TIMEOUT);
				}
				break;

			case STAT:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(cmdbuf[cmdlen] != '\n') {
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
					} else
						do_stat(sockno);
				}
				break;

			case LIST:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(do_list(sockno, cmdbuf, cmdlen) == FALSE) {
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
					}
				}
				break;

			case RETR:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(do_retr(sockno, cmdbuf, cmdlen) == FALSE)
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
				}
				break;

			case DELE:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(do_dele(sockno, cmdbuf, cmdlen) == FALSE)
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
				}
				break;

			case NOOP:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(cmdbuf[cmdlen] != '\n') {
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
					} else
						sock_puts("+OK\n", sockno,
							CMD_TIMEOUT);
				}
				break;

			case RSET:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(cmdbuf[cmdlen] != '\n') {
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
					} else
						do_rset(sockno);
				}
				break;

			case QUIT:
				if(cmdbuf[cmdlen] != '\n') {
					sock_puts("-ERR Syntax error\n",
						sockno, CMD_TIMEOUT);
				} else {
					do_quit(sockno, servername);
					return;
				}
				break;

			case TOP:
				if(state != ST_TRANSACTION) {
					expect_auth(sockno);
				} else {
					if(do_top(sockno, cmdbuf, cmdlen) == FALSE)
						sock_puts("-ERR Syntax error\n",
							sockno, CMD_TIMEOUT);
				}
				break;

			case UIDL:
			case APOP:
				sock_puts("-ERR Command not implemented\n",
					sockno, CMD_TIMEOUT);
				break;

			case BAD:
				sock_puts("-ERR Command not recognised\n",
					sockno, CMD_TIMEOUT);
				break;

			default:
				error("bad case in command switch %d", cmd);
				sprintf(cmdbuf, "bad case in command switch %d\n", cmd);
				dolog(cmdbuf);
				exit(EXIT_FAILURE);
		}
	}
}


/*
 * Parse a line for a valid command.
 *
 */

static INT getcmd(PUCHAR buf)
{	INT i;

	if(strlen(buf) < CMDSIZE) return(BAD);	/* Works even for TOP */
	for(i = 0; ; i++) {
		if(cmdtab[i].cmdcode == BAD) return(BAD);
		if(strnicmp(buf, cmdtab[i].cmdname, CMDSIZE) == 0) break;
	}

	return(cmdtab[i].cmdcode);
}


/*
 * Action when a command is received (other than USER, PASS or QUIT)
 * when no user is logged in.
 *
 */

static VOID expect_auth(INT sockno)
{	sock_puts("-ERR No user authorised\n", sockno, CMD_TIMEOUT);
}


/*
 * Send a message indicating a network read error.
 *
 */

static VOID net_read_error(INT sockno, PUCHAR servername)
{	UCHAR mes[MAXREPLY+1];

	sprintf(mes,
		"-ERR %s network read error - closing transmission channel\n",
		servername);
	sock_puts(mes, sockno, MSG_TIMEOUT); 
	dolog("network read error\n");
}


/*
 * Send a message indicating a network read timeout.
 *
 */

static VOID net_read_timeout(INT sockno, PUCHAR servername)
{	UCHAR mes[MAXREPLY+1];

	sprintf(mes,
		"-ERR %s network read timeout - closing transmission channel\n",
		servername);
	sock_puts(mes, sockno, MSG_TIMEOUT); 
	dolog("network read timeout\n");
}


/*
 * Output a greeting on initial connection.
 *
 */

static VOID greeting(INT sockno, PUCHAR servername)
{	UCHAR mes[MAXREPLY+1];

	sprintf(mes, "+OK %s POP3 server version %d.%d ready\n",
		servername, VERSION, EDIT);
	sock_puts(mes, sockno, CMD_TIMEOUT);
}


/*
 * Handle login after reading username and password.
 *
 * Returns:
 *	TRUE	if logged in OK
 *	FALSE	if login failed
 *
 * In all cases, appropriate messages are issued.
 *
 */

static BOOL do_login(INT sockno)
{	INT rc;
	INT msgcount, octets;
	UCHAR buf[MAXREPLY+1];

	/* Validate username/password combination */

	if(authorise(username, password) == AUTH_FAILURE) {
		sock_puts("-ERR Authorisation failure\n", sockno, CMD_TIMEOUT);
		return(FALSE);
	}

	rc = mail_user(username);
	switch(rc) {
		case MAILUSER_NOLOCK:
			sock_puts("-ERR Failed to lock maildrop\n",
				sockno, CMD_TIMEOUT);
			dolog("failed to lock maildrop");
			return(FALSE);

		case MAILUSER_BADDIR:
			sock_puts("-ERR Failed to find maildrop\n",
				sockno, CMD_TIMEOUT);
			dolog("failed to find maildrop");
			return(FALSE);

		case MAILUSER_FAIL:
			sock_puts("-ERR Failed to initialise maildrop\n",
				sockno, CMD_TIMEOUT);
			dolog("failed to initialise maildrop");
			return(FALSE);

		case MAILUSER_OK:
			break;
	}

	mail_stat(&msgcount, &octets);
	sprintf(
		buf,
		"+OK maildrop contains %d message%s (%d octets)\n",
		msgcount,
		msgcount == 1 ? "" : "s",
		octets);
	sock_puts(buf, sockno, CMD_TIMEOUT);
	sprintf(
		buf,
		"User %s logged in, maildrop contains %d message%s (%d octets)",
		username,
		msgcount,
		msgcount == 1 ? "" : "s",
		octets);
	dolog(buf);

	return(TRUE);
}


/*
 * Handle a USER command.
 *
 * Returns:
 *	TRUE	if command OK
 *	FALSE	if syntax error
 *
 */

static BOOL do_user(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	PUCHAR p = &cmdbuf[cmdlen];

	while(*p == ' ') p++;
	if(*p == '\n') {
		sock_puts(
			"-ERR User name not specified\n",
			sockno,
			CMD_TIMEOUT);
		return(FALSE);
	}

	if(username != (PUCHAR) NULL) free(username);
	username = xmalloc(strlen(p)+1);
	strcpy(username, p);
	username[strlen(username)-1] = '\0';	/* Strip '\n' */

	return(TRUE);
}


/*
 * Handle a PASS command.
 *
 * Returns:
 *	TRUE	if command OK
 *	FALSE	if syntax error
 *
 */

static BOOL do_pass(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	PUCHAR p = &cmdbuf[cmdlen];

	while(*p == ' ') p++;
	if(*p == '\n') {
		sock_puts("-ERR Password not specified\n", sockno, CMD_TIMEOUT);
		return(FALSE);
	}

	if(password != (PUCHAR) NULL) free(password);
	password = xmalloc(strlen(p)+1);
	strcpy(password, p);
	password[strlen(password)-1] = '\0';	/* Strip '\n' */

	return(TRUE);
}


/*
 * Handle the STAT command.
 *
 */

static VOID do_stat(INT sockno)
{	INT msgcount, octets;
	UCHAR buf[MAXREPLY+1];

	mail_stat(&msgcount, &octets);
	sprintf(buf, "+OK %d %d\n", msgcount, octets);
	sock_puts(buf, sockno, CMD_TIMEOUT);
}


/*
 * Handle the LIST command.
 *
 * Returns:
 *	TRUE	if command syntactically OK but possibly failed
 *	FALSE	if syntax error
 *
 */

static BOOL do_list(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	PUCHAR p = &cmdbuf[cmdlen];
	INT i, size;
	INT msgcount, octets;
	UCHAR buf[MAXREPLY+1];

	mail_stat(&msgcount, &octets);

	while(*p == ' ') p++;
	if(*p == '\n') {	/* Scan listing required */
		sock_puts("+OK scan listing follows\n", sockno, CMD_TIMEOUT);

		for(i = 1; i <= msgcount; i++) {
			mail_info(i, &size);
			if(size < 0) continue;	/* Deleted message */
			sprintf(buf, "%d %d\n", i, size);
			sock_puts(buf, sockno, CMD_TIMEOUT);
		}

		sock_puts(".\n", sockno, CMD_TIMEOUT);

		return(TRUE);
	}

	i = atoi(p);
	if(i <= 0) return(FALSE);	/* Syntax error */
	if(i > msgcount) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	mail_info(i, &size);
	if(size < 0) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	sprintf(buf, "+OK %d %d\n", i, size);
	sock_puts(buf, sockno, CMD_TIMEOUT);

	return(TRUE);
}


/*
 * Handle the DELE command.
 *
 * Returns:
 *	TRUE	if command syntactically OK but possibly failed
 *	FALSE	if syntax error
 *
 */

static BOOL do_dele(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	INT i, msgcount, octets;
	PUCHAR p = &cmdbuf[cmdlen];
	UCHAR buf[MAXREPLY+1];

	while(*p == ' ') p++;
	if(*p == '\n')		/* No message specified */
		return(FALSE);

	i = atoi(p);
	if(i <= 0) return(FALSE);	/* Syntax error */

	mail_stat(&msgcount, &octets);
#ifdef	DEBUG
	trace("do_dele: msg %d, msgcount=%d", i, msgcount);
#endif
	if(i > msgcount) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	if(mail_dele(i) == MAILDELE_FAIL) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	sprintf(buf, "+OK message %d deleted\n", i);
	sock_puts(buf, sockno, CMD_TIMEOUT);

	return(TRUE);
}


/*
 * Handle the RSET command.
 *
 */

static VOID do_rset(INT sockno)
{	mail_rset();

	sock_puts("+OK\n", sockno, CMD_TIMEOUT);
}


/*
 * Handle the RETR command.
 *
 * Returns:
 *	TRUE	if command syntactically OK but possibly failed
 *	FALSE	if syntax error
 *
 */

static BOOL do_retr(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	INT i, size;
	INT rc;
	PUCHAR p = &cmdbuf[cmdlen];
	UCHAR buf[MAXLINE+1];

	while(*p == ' ') p++;
	if(*p == '\n')		/* No message specified */
		return(FALSE);

	i = atoi(p);
	if(i <= 0) return(FALSE);	/* Syntax error */

	rc = mail_retr(i, &size);
	if(rc == MAILRETR_NOMES) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	if(rc == MAILRETR_FAIL) {
		sock_puts("-ERR Message retrieval failure\n",
			sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	sprintf(buf, "+OK message %d has %d octets\n", i, size);
	sock_puts(buf, sockno, CMD_TIMEOUT);

	for(;;) {
		rc = mail_line(buf, MAXLINE);

		if(rc == MAILLINE_FAIL) {
			sprintf(buf, "error retrieving message %d\n", i);
			error(buf);
			dolog(buf);
			sock_puts(".\n", sockno, DATA_TIMEOUT);
			mail_close();
			return(TRUE);
		}

		sock_puts(buf, sockno, DATA_TIMEOUT);
		if(rc == MAILLINE_EOF) break;
	}
	sprintf(buf, "retrieved message %d (%d octets)", i, size);
	dolog(buf);

	mail_close();

	return(TRUE);
}


/*
 * Handle the TOP command.
 *
 * Returns:
 *	TRUE	if command syntactically OK but possibly failed
 *	FALSE	if syntax error
 *
 */

static BOOL do_top(INT sockno, PUCHAR cmdbuf, INT cmdlen)
{	INT i, size;
	INT nlines;
	INT in_body = FALSE;
	INT rc;
	PUCHAR p = &cmdbuf[cmdlen];
	UCHAR buf[MAXLINE+1];

	while(*p == ' ') p++;
	if(*p == '\n')		/* No message specified */
		return(FALSE);

	p = strtok(p, " ");
	if(p == (PUCHAR) NULL) return(FALSE);
	i = atoi(p);			/* Message number */
	if(i <= 0) return(FALSE);	/* Syntax error */
	p = strtok(NULL, " ");
	if(p == (PUCHAR) NULL) return(FALSE);
	nlines = atoi(p);		/* Number of lines required */
	if(nlines < 0) return(FALSE);	/* Syntax error */

#ifdef	DEBUG
	trace("top: message %d, requested %d lines", i, nlines);
#endif

	rc = mail_retr(i, &size);
	if(rc == MAILRETR_NOMES) {
		sock_puts("-ERR No such message\n", sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	if(rc == MAILRETR_FAIL) {
		sock_puts("-ERR Message retrieval failure\n",
			sockno, CMD_TIMEOUT);
		return(TRUE);
	}
	strcpy(buf, "+OK top of message follows\n");
	sock_puts(buf, sockno, CMD_TIMEOUT);

	for(;;) {
		rc = mail_line(buf, MAXLINE);

		if(rc == MAILLINE_FAIL) {
			sprintf(buf, "error retrieving message %d\n", i);
			error(buf);
			dolog(buf);
			sock_puts(".\n", sockno, DATA_TIMEOUT);
			mail_close();
			return(TRUE);
		}

		if(in_body == TRUE) {
			if(nlines <= 0) {
				sock_puts(".\n", sockno, DATA_TIMEOUT);
				break;	/* Sent all requested lines */
			}
			nlines--;
		} else {
			if(buf[0] == '\n') in_body = TRUE;
		}

		sock_puts(buf, sockno, DATA_TIMEOUT);
		if(rc == MAILLINE_EOF) break;
	}

	mail_close();

	return(TRUE);
}


/*
 * Handle the QUIT command.
 *
 */

static VOID do_quit(INT sockno, PUCHAR servername)
{	INT msgcount, size;
	UCHAR mes[MAXREPLY+1];
	INT unlocking = (state == ST_TRANSACTION ? TRUE : FALSE);

	state = ST_UPDATE;

	/* Action deletions, and unlock maildrop if required */

	mail_commit(unlocking, &msgcount, &size);

	sprintf(mes,
		"+OK %s POP3 service closing, user has %d message%s (%d octets)\n",
		servername, msgcount, msgcount == 1 ? "" : "s", size);
	sock_puts(mes, sockno, CMD_TIMEOUT);
	sprintf(mes,
		"User logged off %s, %d message%s (%d octets) remaining\n",
		servername, msgcount, msgcount == 1 ? "" : "s", size);
	dolog(mes);

	/* Tidy storage */

	free(username);
	free(password);
}

/*
 * End of file: server.c
 *
 */
