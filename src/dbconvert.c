/* dbconvert.c - Convert databases to various TinyMUSH 3.0 formats */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#undef MEMORY_BASED

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

extern void NDECL(cf_init);
extern void FDECL(do_dbck, (dbref, dbref, int));
extern void NDECL(vattr_init);

/* ---------------------------------------------------------------------------
 * Cheap hack to avoid issues in compiling standalone.
 */

void raw_notify(player, msg)
    dbref player;
    const char *msg;
{
}

/* ---------------------------------------------------------------------------
 * info: display info about the file being read or written.
 */

void info(fmt, flags, ver)
int fmt, flags, ver;
{
	const char *cp;

	switch (fmt) {
        case F_TINYMUSH:
	        cp = "TinyMUSH-3";
		break;
	case F_MUX:
	        cp = "TinyMUX";
		break;
	case F_MUSH:
		cp = "TinyMUSH";
		break;
	case F_MUSE:
		cp = "TinyMUSE";
		break;
	case F_MUD:
		cp = "TinyMUD";
		break;
	case F_MUCK:
		cp = "TinyMUCK";
		break;
	default:
		cp = "*unknown*";
		break;
	}
	fprintf(mainlog_fp, "%s version %d:", cp, ver);
	if (flags & V_ZONE)
		fprintf(mainlog_fp, " Zone");
	if (flags & V_LINK)
		fprintf(mainlog_fp, " Link");
	if (flags & V_GDBM)
		fprintf(mainlog_fp, " GDBM");
	if (flags & V_ATRNAME)
		fprintf(mainlog_fp, " AtrName");
	if (flags & V_ATRKEY) {
		if ((fmt == F_MUSH) && (ver == 2))
			fprintf(mainlog_fp, " ExtLocks");
		else
			fprintf(mainlog_fp, " AtrKey");
	}
	if (flags & V_PARENT)
		fprintf(mainlog_fp, " Parent");
	if (flags & V_COMM)
		fprintf(mainlog_fp, " Comm");
	if (flags & V_ATRMONEY)
		fprintf(mainlog_fp, " AtrMoney");
	if (flags & V_XFLAGS)
		fprintf(mainlog_fp, " ExtFlags");
	if (flags & V_3FLAGS)
		fprintf(mainlog_fp, " MoreFlags");
	if (flags & V_POWERS)
	    	fprintf(mainlog_fp, " Powers");
	if (flags & V_QUOTED)
		fprintf(mainlog_fp, " QuotedStr");
	if (flags & V_TQUOTAS)
		fprintf(mainlog_fp, " TypedQuotas");
	if (flags & V_TIMESTAMPS)
		fprintf(mainlog_fp, " Timestamps");
	fprintf(mainlog_fp, "\n");
}

void usage(prog)
char *prog;
{
	fprintf(mainlog_fp, "Usage: %s gdbm-file [flags] [<in-file] [>out-file]\n", prog);
	fprintf(mainlog_fp, "   Available flags are:\n");
	fprintf(mainlog_fp, "      C - Perform consistency check\n");
	fprintf(mainlog_fp, "      G - Write in gdbm format        g - Write in flat file format\n");
	fprintf(mainlog_fp, "      K - Store key as an attribute   k - Store key in the header\n");
	fprintf(mainlog_fp, "      L - Include link information    l - Don't include link information\n");
	fprintf(mainlog_fp, "      M - Store attr map if GDBM      m - Don't store attr map if GDBM\n");
	fprintf(mainlog_fp, "      N - Store name as an attribute  n - Store name in the header\n");
	fprintf(mainlog_fp, "      P - Include parent information  p - Don't include parent information\n");
	fprintf(mainlog_fp, "      W - Write the output file  b    w - Don't write the output file.\n");
	fprintf(mainlog_fp, "      X - Create a default GDBM db    x - Create a default flat file db\n");
	fprintf(mainlog_fp, "      Z - Include zone information    z - Don't include zone information\n");
	fprintf(mainlog_fp, "      <number> - Set output version number\n");
}

int main(argc, argv)
int argc;
char *argv[];
{
	int setflags, clrflags, ver;
	int db_ver, db_format, db_flags, do_check, do_write;
	char *fp;

	logfile_init(NULL);

	if ((argc < 2) || (argc > 3)) {
		usage(argv[0]);
		exit(1);
	}
	cf_init();
#ifdef RADIX_COMPRESSION
	init_string_compress();
#endif
	/* Decide what conversions to do and how to format the output file */

	setflags = clrflags = ver = do_check = 0;
	do_write = 1;

	if (argc == 3) {
		for (fp = argv[2]; *fp; fp++) {
			switch (*fp) {
			case 'C':
				do_check = 1;
				break;
			case 'G':
				setflags |= V_GDBM;
				break;
			case 'g':
				clrflags |= V_GDBM;
				break;
			case 'Z':
				setflags |= V_ZONE;
				break;
			case 'z':
				clrflags |= V_ZONE;
				break;
			case 'L':
				setflags |= V_LINK;
				break;
			case 'l':
				clrflags |= V_LINK;
				break;
			case 'N':
				setflags |= V_ATRNAME;
				break;
			case 'n':
				clrflags |= V_ATRNAME;
				break;
			case 'K':
				setflags |= V_ATRKEY;
				break;
			case 'k':
				clrflags |= V_ATRKEY;
				break;
			case 'P':
				setflags |= V_PARENT;
				break;
			case 'p':
				clrflags |= V_PARENT;
				break;
			case 'W':
				do_write = 1;
				break;
			case 'w':
				do_write = 0;
				break;
			case 'X':
				clrflags = 0xffffffff;
				setflags = OUTPUT_FLAGS;
				ver = OUTPUT_VERSION;
				break;
			case 'x':
				clrflags = 0xffffffff;
				setflags = UNLOAD_OUTFLAGS;
				ver = UNLOAD_VERSION;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ver = ver * 10 + (*fp - '0');
				break;
			default:
				fprintf(mainlog_fp, "Unknown flag: '%c'\n", *fp);
				usage(argv[0]);
				exit(1);
			}
		}
	}
	/* Open the gdbm file */

	vattr_init();
	if (init_gdbm_db(argv[1]) < 0) {
		fprintf(mainlog_fp, "Can't open GDBM file\n");
		exit(1);
	}
	/* Go do it */

	db_read(stdin, &db_format, &db_ver, &db_flags);
	fprintf(mainlog_fp, "Input: ");
	info(db_format, db_flags, db_ver);

	if (do_check)
		do_dbck(NOTHING, NOTHING, DBCK_FULL);

	if (do_write) {
		db_flags = (db_flags & ~clrflags) | setflags;
		if (ver != 0)
			db_ver = ver;
		else
			db_ver = 3;
		fprintf(mainlog_fp, "Output: ");
		info(F_TINYMUSH, db_flags, db_ver);
		db_write(stdout, F_TINYMUSH, db_ver | db_flags);
	}
	CLOSE;
	exit(0);
}
