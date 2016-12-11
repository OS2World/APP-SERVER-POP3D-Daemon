/*
 * File: pop3d.h
 *
 * POP3 daemon for distributing mail on Tavi network; to be invoked only
 * by INETD.
 *
 * Header file
 *
 * Bob Eager   June 2002
 *
 */

#include <os2.h>

#define	VERSION			3	/* Major version number */
#define	EDIT			0	/* Edit number within major version */

#define	FALSE			0
#define	TRUE			1

/* External references */

extern	VOID	error(PUCHAR, ...);
extern	BOOL	server(INT, PUCHAR, PUCHAR, PUCHAR);
extern	PVOID	xmalloc(size_t);

/*
 * End of file: pop3d.h
 *
 */

