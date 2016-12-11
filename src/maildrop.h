/*
 * File: maildrop.h
 *
 * General maildrop handling routines; header file.
 *
 * Bob Eager   June 2002
 *
 */

/* Miscellaneous constants */

#define	FALSE			0
#define	TRUE			1

/* Error codes */

#define	MAILINIT_OK		0	/* Initialisation successful */
#define	MAILINIT_NOENV		1	/* Environment variable not set */
#define	MAILINIT_BADDIR		2	/* Cannot access directory */

#define	MAILUSER_OK		0	/* User selected OK */
#define	MAILUSER_BADDIR		1	/* Cannot access directory */
#define	MAILUSER_NOLOCK		2	/* Cannot lock maildrop */
#define	MAILUSER_FAIL		3	/* Other failure */

#define	MAILRETR_OK		0	/* Mail retrieval OK or no such msg */
#define	MAILRETR_FAIL		1	/* Mail retrieval error */
#define	MAILRETR_NOMES		2	/* No such message */

#define	MAILLINE_OK		0	/* Message line retrieved */
#define	MAILLINE_EOF		1	/* No more message lines */
#define	MAILLINE_FAIL		2	/* Error retrieving message line */

#define	MAILDELE_OK		0	/* Message marked for deletion OK */
#define	MAILDELE_FAIL		1	/* Failed to delete message */

/* External references */

extern	VOID	mail_close(VOID);
extern	VOID	mail_commit(INT, PINT, PINT);
extern	INT	mail_dele(INT);
extern	VOID	mail_info(INT, PINT);
extern	INT	mail_init(PUCHAR);
extern	INT	mail_line(PUCHAR, INT);
extern	VOID	mail_list(INT, PINT);
extern	INT	mail_retr(INT, PINT);
extern	VOID	mail_rset(VOID);
extern	VOID	mail_stat(PINT, PINT);
extern	INT	mail_user(PUCHAR);

/*
 * End of file: maildrop.h
 *
 */
