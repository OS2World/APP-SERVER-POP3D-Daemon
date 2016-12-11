/*
 * File: cmds.h
 *
 * POP3 daemon for distributing mail on Tavi network; to be invoked only
 * by INETD.
 *
 * Command codes and table.
 *
 * Bob Eager   June 2002
 *
 */

/* Internal command codes */

#define	USER	1
#define	PASS	2
#define	STAT	3
#define	LIST	4
#define	RETR	5
#define	DELE	6
#define	NOOP	7
#define	RSET	8
#define	QUIT	9
#define	APOP	10
#define	TOP	11
#define	UIDL	12
#define	BAD	13

#define	CMDSIZE	4			/* Max size of a POP3 command */

static	struct	cmdtab {
	PUCHAR	cmdname;		/* Command name */
	INT	cmdcode;		/* Command code */
} cmdtab[] = {
	{ "USER", USER },
	{ "PASS", PASS },
	{ "STAT", STAT },
	{ "LIST", LIST },
	{ "RETR", RETR },
	{ "DELE", DELE },
	{ "NOOP", NOOP },
	{ "RSET", RSET },
	{ "QUIT", QUIT },
	{ "APOP", APOP },
	{ "TOP ", TOP  },		/* Note the space */
	{ "UIDL", UIDL },
	{ "",     BAD }			/* End of table marker */
};

/*
 * End of file: cmds.h
 *
 */

