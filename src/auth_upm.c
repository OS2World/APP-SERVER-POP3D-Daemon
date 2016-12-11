/*
 * File: auth_upm.c
 *
 * General user authorisation routines - UPM version
 *
 * Bob Eager   June 2002
 *
 */

#pragma	strings(readonly)

#include <os2.h>

#include <string.h>
#define	PURE_32
#include <upm.h>

#include "auth.h"

/* The character set for usernames and passwords is limited to:
	A-Z 0-9 # @ $
   Lower case is forced to upper case.
   Username and password lengths are limited as shown below.
*/

#define	MAXUSER		8		/* Maximum length of username */
#define	MAXPASS		8		/* Maximum length of password */

/* Forward references */

static	BOOL	xisalnum(INT);
static	INT	xtoupper(INT);


/*
 * Check user authorisation.
 *
 * Returns:
 *	AUTH_OK		user authorised OK
 *	AUTH_FAILURE	authorisation failure
 *
 */

INT authorise(PUCHAR username, PUCHAR password)
{	INT i, j;
	INT ch;
	INT rc;
	UCHAR user[MAXUSER+1];
	UCHAR pass[MAXPASS+1];

	i = 0;
	j = 0;
	while(j < MAXUSER) {
		ch = username[i++];
		if(xisalnum(ch) ||
		   ch == '#' ||
		   ch == '@' ||
		   ch == '$' ||
		   ch == '\0')
			user[j++] = xtoupper(ch);
		if(ch == '\0') break;
	}
	user[MAXUSER] = '\0';

	i = 0;
	j = 0;
	while(j < MAXPASS) {
		ch = password[i++];
		if(xisalnum(ch) ||
		   ch == '#' ||
		   ch == '@' ||
		   ch == '$' ||
		   ch == '\0')
			pass[j++] = xtoupper(ch);
		if(ch == '\0') break;
	}
	pass[MAXPASS] = '\0';

	rc = u32elgn(user, pass, "", UPM_LOCAL, UPM_USER);
	if(rc != UPM_OK) return(AUTH_FAILURE);

	return(AUTH_OK);
}


/*
 * Check whether a character is alphanumeric.
 *
 */

static BOOL xisalnum(INT c)
{	if((c >= 'A') && (c <= 'Z')) return(TRUE);
	if((c >= 'a') && (c <= 'z')) return(TRUE);
	if((c >= '0') && (c <= '9')) return(TRUE);

	return(FALSE);
}


/*
 * Convert a character to upper case.
 *
 */

static INT xtoupper(INT c)
{	if(('a' <= c) && (c <= 'z')) c = c - 'a' + 'A';
	return(c);
}

/*
 * End of file: auth_upm.c
 *
 */
