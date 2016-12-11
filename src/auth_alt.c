/*
 * File: auth_alt.c
 *
 * General user authorisation routines - alternate version
 *
 * Bob Eager   June 2002
 *
 */

#pragma	strings(readonly)

#include <os2.h>

#include "auth.h"


/*
 * Check user authorisation.
 *
 * Returns:
 *	AUTH_OK		user authorised OK
 *	AUTH_FAILURE	authorisation failure
 *
 */

INT authorise(PUCHAR username, PUCHAR password)
{
	return(AUTH_OK);
}

/*
 * End of file: auth_alt.c
 *
 */
