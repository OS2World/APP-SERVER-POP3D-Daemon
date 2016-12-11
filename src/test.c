/*
 * File: test.c
 *
 * Test program for authorisation module
 *
 * Bob Eager   June 2002
 *
 */

#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>

#include "auth.h"
#include "log.h"

#define	LOGNAME		"z.log"


int main(int argc, char *argv[])
{	int rc;
	char user[100], pass[100];

	rc = open_logfile("TMP", LOGNAME);

	for(;;) {
		fprintf(stdout, "Username:");
		fflush(stdout);
		gets(user);
		if(user[0] == '\0') break;
		fprintf(stdout, "Password:");
		fflush(stdout);
		gets(pass);

		rc = authorise(user, pass);
		if(rc == AUTH_OK) {
			fprintf(stdout, "OK\n");
		} else {
			fprintf(stdout, "Failed\n");
		}
	}

	return(EXIT_SUCCESS);
}

/*
 * End of file: test.c
 *
 */

