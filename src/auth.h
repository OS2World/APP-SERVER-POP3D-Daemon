/*
 * File: auth.h
 *
 * General user authorisation routines; header file.
 *
 * Bob Eager   June 2002
 *
 */

#ifdef	DEBUG
#include "log.h"
#endif

/* Return codes */

#define	AUTH_OK		1
#define	AUTH_FAILURE	0

/* External references */

extern	INT	authorise(PUCHAR, PUCHAR);

/*
 * End of file: auth.h
 *
 */

