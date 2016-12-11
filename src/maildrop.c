/*
 * File: maildrop.c
 *
 * General maildrop handling routines.
 *
 * Bob Eager   June 2002
 *
 */

#pragma	strings(readonly)

#pragma	alloc_text(a_init_seg, mail_init)
#pragma	alloc_text(a_init_seg, fstype)

#define	INCL_DOSFILEMGR
#define	INCL_DOSERRORS
#define INCL_DOSPROCESS
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pop3d.h"
#include "maildrop.h"
#include "lock.h"
#include "netio.h"
#include "log.h"

#define	FSQBUFSIZE	100		/* Size of FS query buffer */
#define	SLOTQ		50		/* Message slot quantum */

#define	MAXMES		100		/* Maximum length of a return message */

#define	MAXLOCKTRIES	5		/* Maximum number of tries at maildrop lock */
#define	LOCKINTERVAL	10		/* Lock retry interval (seconds) */

/* Type definitions */

typedef	enum	{ FS_CDFS, FS_FAT, FS_HPFS, FS_NFS, FS_JFS }
	FSTYPE;

typedef struct _MSG {			/* Message table entry */
ULONG		size;
INT		delete;
PUCHAR		name;
} MSG, *PMSG;

/* Forward references */

static	BOOL	build_table(VOID);
static	FSTYPE	fstype(PUCHAR);
static	INT	sortfunc(const void *, const void *);

/* Local storage */

static	UCHAR	maildir[CCHMAXPATH+1];
static	INT	msgcount;
static	ULONG	total_octets;
static	UCHAR	userdir[CCHMAXPATH+1];
static	FSTYPE	mailfstype;
static	PMSG	msgtable;
static	FILE	*mailfp;


/*
 * Initialise storage, etc.
 *
 * Returns:
 *	MAILINIT_OK		Initialisation successful
 *	MAILINIT_NOENV		Mail spool directory env variable not set
 *	MAILINIT_BADDIR		Cannot access mail spool directory
 *
 */

INT mail_init(PUCHAR direnv)
{	APIRET rc;
	INT last;
	INT disk = 0;
	PUCHAR dir;

	dir = getenv(direnv);
	if(dir == (PUCHAR) NULL) return(MAILINIT_NOENV);

	if(isalpha(dir[0]) && dir[1] == ':') {
		disk = toupper(dir[0]) - 'A' + 1;

		rc = DosSetDefaultDisk(disk);
		if(rc != 0) return(MAILINIT_BADDIR);
	}

	strcpy(maildir, dir);
	last = strlen(maildir) - 1;
	if(maildir[last] == '\\' || maildir[last] == '/')
		maildir[last] = '\0';
	rc = DosSetCurrentDir(maildir);
	if(rc != 0) return(MAILINIT_BADDIR);

	mailfstype = fstype(maildir);
#ifdef	DEBUG
	trace("mail base directory = \"%s\", FS type = %s\n",
		maildir, mailfstype == FS_FAT  ? "FAT"  :
			 mailfstype == FS_HPFS ? "HPFS" :
			 mailfstype == FS_CDFS ? "CDFS" :
			 mailfstype == FS_NFS  ? "NFS"  :
			 mailfstype == FS_JFS  ? "JFS"  :
			 "????");
#endif

	mailfp = (FILE *) NULL;
	msgtable = (PMSG) NULL;

	return(MAILINIT_OK);
}


/*
 * Function to determine the type of file system on the drive specified
 * in 'path'.
 *
 * Returns:
 *	 FS_CDFS, FS_NFS, FS_FAT, FS_HPFS or FS_JFS
 *
 * Any error causes the result to default to FS_FAT.
 *
 */

static FSTYPE fstype(PUCHAR path)
{	ULONG drive, dummy;
	ULONG fsqbuflen = FSQBUFSIZE;
	UCHAR fsqbuf[FSQBUFSIZE];
	PUCHAR p;
	UCHAR drv[3];

	if(path[1] != ':') {		/* No drive in path - use default */
		(VOID) DosQueryCurrentDisk(&drive, &dummy);
		drv[0] = drive + 'a' - 1;
	} else {
		drv[0] = path[0];
	}
	drv[1] = ':';
	drv[2] = '\0';

	if(DosQueryFSAttach(
			drv,
			0L,
			FSAIL_QUERYNAME,
			(PFSQBUFFER2) fsqbuf,
			&fsqbuflen) != 0)
		return(FS_FAT);

	/* Set 'p' to point to the file system name */

	p = fsqbuf + sizeof(USHORT);	/* Point past device type */
	p += (USHORT) *p + 3*sizeof(USHORT) + 1;
					/* Point past drive name and FS name */
					/* and FSDA length */

	if(strcmp(p, "CDFS") == 0) return(FS_CDFS);
	if(strcmp(p, "HPFS") == 0) return(FS_HPFS);
	if(strcmp(p, "NFS") == 0) return(FS_NFS);
	if(strcmp(p, "JFS") == 0) return(FS_JFS);

	return(FS_FAT);
}


/*
 * Set up for a new username.
 *
 * Returns:
 *	MAILUSER_OK		new user selected and maildrop locked
 *	MAILUSER_BADDIR		no user maildrop directory
 *	MAILUSER_NOLOCK		could not lock maildrop
 *	MAILUSER_FAIL		other failure
 *
 */

INT mail_user(PUCHAR username)
{	INT rc;
	INT i;

	/* Select the maildrop directory */

	strcpy(userdir, maildir);
	strcat(userdir, "\\");

	if((mailfstype == FS_HPFS) || (mailfstype == FS_JFS)) {
		strcat(userdir, username);
	} else {
		INT len = strlen(username);
		PUCHAR temp = xmalloc(13);

		if(len > 11) len = 11;
		if(len > 8) {
			strncpy(temp, username, 8);
			temp[8] = '\0';
			strcat(temp, ".");
			strncat(temp, username+8, len-8);
		} else strcpy(temp, username);
		strcat(userdir, temp);
		free(temp);
	}

#ifdef	DEBUG
	trace("mail user directory = \"%s\"", userdir);
#endif

	rc = DosSetCurrentDir(userdir);
	if(rc != 0) return(MAILUSER_BADDIR);

	/* Lock the maildrop directory */

	for(i = 1; i <= MAXLOCKTRIES; i++) {
#ifdef	DEBUG
		trace("trying for maildrop lock...");
#endif
		rc = lock(userdir);
		if(rc == TRUE) break;
		if(i == MAXLOCKTRIES) return(MAILUSER_NOLOCK);

#ifdef	DEBUG
		trace("waiting for lock...");
#endif
		(VOID) DosSleep(LOCKINTERVAL*1000);
	}

#ifdef	DEBUG
	trace("locked the maildrop");
#endif
	/* Build the message table */

	if(build_table() == FALSE) return(MAILUSER_FAIL);

	return(MAILUSER_OK);
}


/*
 * Build the table of messages in the maildrop.
 *
 */

static BOOL build_table(VOID)
{	APIRET rc;
	HDIR hdir = HDIR_CREATE;
	ULONG count;
	INT len;
	FILEFINDBUF3 entry;
	PUCHAR mask = { (mailfstype == FS_HPFS) ||
			(mailfstype == FS_JFS) ? "*" : "*.*" };
	INT freeslots = 0;
	PMSG cur;

	msgcount = 0;
	total_octets = 0;
	count = 1;
	rc = DosFindFirst(
		mask,
		&hdir,
		FILE_NORMAL,
		&entry,
		sizeof(entry),
		&count,
		FIL_STANDARD);
	if(rc == ERROR_NO_MORE_FILES) return(TRUE);
	if(rc != NO_ERROR) {
		error("DosFindFirst failed, rc=%d", rc);
		return(FALSE);
	}

	while(count != 0) {
#ifdef	DEBUG
		trace("msgcount=%d, freeslots=%d", msgcount, freeslots);
#endif
		if(freeslots <= 0) {
#ifdef	DEBUG
			trace("creating slots");
#endif
			msgtable = realloc(msgtable,
				(msgcount+SLOTQ)*sizeof(MSG));
			if(msgtable == (PMSG) NULL) {
				error("out of storage");
				return(FALSE);
			}
			freeslots = SLOTQ;
		}

		cur = &msgtable[msgcount];
		len = strlen(entry.achName);
		cur->name = (PUCHAR) xmalloc(strlen(entry.achName)+1);
		if(cur->name == (PUCHAR) NULL) {
			error("out of storage");
			return(FALSE);
		}
		strcpy(cur->name, entry.achName);
		cur->size = entry.cbFile;
		cur->delete = FALSE;
		msgcount++;
		freeslots--;

		count = 1;
		rc = DosFindNext(
			hdir,
			&entry,
			sizeof(entry),
			&count);

		if(rc == ERROR_NO_MORE_FILES) break;
		if(rc != NO_ERROR) {
			error("DosFindNext failed, rc=%d", rc);
			return(FALSE);
		}
	}

	(VOID) DosFindClose(hdir);

#ifdef	DEBUG
	{	INT i;

		trace("     Before sort");
		trace("  msg   size   name");
		for(i = 0; i < msgcount; i++) {
			trace("%4d %6d  %s", i+1, msgtable[i].size,
				msgtable[i].name);
		}
		trace("--------------------------");
	}
#endif

	/* On a FAT volume, sort the files into alphabetical order,
	   which corresponds to likely arrival order. */

	if(mailfstype == FS_FAT) {
		qsort(&msgtable[0], msgcount, sizeof(MSG), sortfunc);
	}

#ifdef	DEBUG
	{	INT i;

		trace("     After sort");
		trace("  msg   size   name");
		for(i = 0; i < msgcount; i++) {
			trace("%4d %6d  %s", i+1, msgtable[i].size,
				msgtable[i].name);
		}
		trace("--------------------------");
	}
#endif

	return(TRUE);
}


/*
 * Function used for sorting the message table.
 * This is used by 'qsort', and must return:
 *	< 0	p less than q
 *	= 0	p equal to q
 *	> 0	p greater than q
 *
 */

static INT sortfunc(const void *p, const void *q)
{	return(strcmpi(((PMSG)p)->name, ((PMSG)q)->name));
}


/*
 * Return maildrop statistics.
 *
 * These consist of the number of messages, and the total number of octets.
 *
 */

VOID mail_stat(PINT nmsgs, PINT octets)
{	INT i;

	*nmsgs = msgcount;
	if(total_octets == 0) {
		for(i = 0; i < msgcount; i++)
			total_octets += msgtable[i].size;
	}
	*octets = total_octets;
#ifdef	DEBUG
	trace("stats: %d messages, %d octets", msgcount, total_octets);
#endif
}


/*
 * Return mail item statistics.
 *
 * This consists of the size of a message in octets, or -1 if the message
 * does not exist. Note that the message number is 1-based.
 *
 */

VOID mail_info(INT msg, PINT octets)
{	PMSG item = &msgtable[msg-1];

	if(item->delete == TRUE) {
		*octets = -1;
	} else {
		*octets = item->size;
	}
}


/*
 * Commence retrieval of a mail item. This opens the mail file ready for
 * access. Note that the message number is 1-based.
 *
 * Returns:
 *	MAILRETR_OK	mail ready for retrieval
 *	MAILRETR_FAIL	failed to access mail file
 *	MAILRETR_NOMES	no such message
 *
 */

INT mail_retr(INT msg, PINT octets)
{	PMSG item;
	UCHAR mes[MAXMES+1];

	if(msg <= 0 || msg > msgcount) return(MAILRETR_NOMES);

	item = &msgtable[msg-1];

	/* Get basic statistics */

	if(item->delete == TRUE) {
		return(MAILRETR_NOMES);
	} else {
		*octets = item->size;
	}

	/* Open the mail file */

	mailfp = fopen(item->name, "r");
	if(mailfp == (FILE *) NULL) {
		sprintf(mes, "cannot open mail file %s", item->name);
		error(mes);
		dolog(mes);
		return(MAILRETR_FAIL);
	}

	return(MAILRETR_OK);
}


/*
 * Retrieve the next line from the current mail file.
 * Performs dot stuffing as required, and generates the final dot line.
 *
 * Returns:
 *	MAILLINE_OK	Line retrieved OK
 *	MAILLINE_EOF	End of file, returning dot line
 *	MAILLINE_FAIL	Error retrieving mail line
 *
 */

INT mail_line(PUCHAR buf, INT max)
{	INT len;
	PUCHAR p;

	p = fgets(buf, max, mailfp);
	if(p == (PUCHAR) NULL) {	/* End of file */
		buf[0] = '.';
		buf[1] = '\n';
		buf[2] = '\0';

		return(MAILLINE_EOF);
	}

	len = strlen(buf);
	if((buf[len-1] != '\n') ||
	   ((buf[0] == '.') && (len == max))) {
		strcpy(buf, "line too long in mail file");
			error(buf);
			dolog(buf);
		return(MAILLINE_FAIL);
	}

	if(buf[0] == '.') {		/* Need to dot stuff */
		memmove(&buf[1], &buf[0], len+1);
		buf[0] = '.';
	}

#ifdef	DEBUG
	trace("mail_line: %s", buf);
#endif
	return(MAILLINE_OK);
}


/*
 * Close any open mail file.
 *
 */

VOID mail_close(VOID)
{	if(mailfp != (FILE *) NULL) {
		(VOID) fclose(mailfp);
		mailfp = (FILE *) NULL;
	}
}


/*
 * Perform a reset. Clear all delete indicators.
 *
 */

VOID mail_rset(VOID)
{	INT i;

	for(i = 0; i < msgcount; i++)
		msgtable[i].delete = FALSE;
}


/*
 * Delete a message. Strictly, the message is just marked for deletion.
 *
 * Returns:
 *	MAILDELE_OK	message marked for deletion
 *	MAILDELE_FAIL	no such message
 *
 */

INT mail_dele(INT msgno)
{	if(msgno < 0 ||
	   msgno > msgcount ||
	   msgtable[msgno-1].delete == TRUE)
		return(MAILDELE_FAIL);

	msgtable[msgno-1].delete = TRUE;
	return(MAILDELE_OK);
}


/*
 * Close down the maildrop; commit any deletions,
 * and unlock the maildrop if required.
 *
 */

VOID mail_commit(INT unlocking, PINT count, PINT octets)
{	INT i;
	PMSG cur;

	mail_close();			/* For safety */
	*count = 0;
	*octets = 0;

#ifdef	DEBUG
	trace("commit: msgcount = %d", msgcount);
#endif
	for(i = 0; i < msgcount; i++) {
		cur = &msgtable[i];
		if(cur->delete == FALSE) {
			*count += 1;
			*octets += cur->size;
			continue;
		}

#ifdef	DEBUG
		trace("committing deletion of message %d, file %s",
			i+1, cur->name);
#endif
		remove(cur->name);
	}

	if(unlocking == TRUE) unlock();
}

/*
 * End of file: maildrop.c
 *
 */
