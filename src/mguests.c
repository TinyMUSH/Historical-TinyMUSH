/* mguests.c - multiguest code originally ported from DarkZone */
/* $Id$ */

#include "autoconf.h"
#include "copyright.h"
#include "config.h"

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mail.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by interface */
#include "interface.h"	/* required by code */

#include "mguests.h"	/* required by code */
#include "attrs.h"	/* required by code */
#include "powers.h"	/* required by code */

extern void FDECL(destroy_player, (dbref));
extern void FDECL(do_lock, (dbref, dbref, int, char *, char *));
typedef int object_flag_type;

dbref create_guest(name, password)
char *name;
char *password;
{
	dbref player;
	char *buff;

	if (!Wizard(mudconf.guest_nuker) || !Good_obj(mudconf.guest_nuker))
		mudconf.guest_nuker = 1;

	buff = alloc_lbuf("create_guest");

	/*
	 * Make the player. 
	 */

	player = create_player(name, password, mudconf.guest_nuker, 0, 1);

	if (player == NOTHING) {
		log_text("GUEST: failed in create_player\n");
		return NOTHING;
	}
	/*
	 * Turn the player into a guest. 
	 */

	s_Guest(player);
	if (Good_obj(mudconf.guest_start_room))
	    move_object(player, mudconf.guest_start_room);
	else
	    move_object(player, mudconf.start_room);
	s_Flags(player, (Flags(mudconf.guest_char) & ~TYPE_MASK &
			 ~mudconf.stripped_flags.word1) | TYPE_PLAYER);
	s_Flags2(player, Flags2(mudconf.guest_char) &
		 ~mudconf.stripped_flags.word2);
	s_Flags3(player, Flags3(mudconf.guest_char) &
		 ~mudconf.stripped_flags.word3);
	s_Pennies(player, Pennies(mudconf.guest_char));
	s_Zone(player, Zone(mudconf.guest_char));
	s_Parent(player, Parent(mudconf.guest_char));

	/*
	 * Make sure the guest is locked. 
	 */
	do_lock(player, player, A_LOCK, tprintf("#%d", player), "me");
	do_lock(player, player, A_LENTER, tprintf("#%d", player), "me");

	/*
	 * Copy all attributes. 
	 */
	atr_cpy(GOD, player, mudconf.guest_char);
	free_lbuf(buff);
	return player;
}

void destroy_guest(guest)
dbref guest;
{
	if (!Wizard(mudconf.guest_nuker) || !Good_obj(mudconf.guest_nuker))
		mudconf.guest_nuker = 1;

	if (!Guest(guest))
		return;

	atr_add_raw(guest, A_DESTROYER, tprintf("%d", mudconf.guest_nuker));
	destroy_player(guest);
	destroy_obj(mudconf.guest_nuker, guest);
}

char *make_guest(d)
DESC *d;
{
	int i;
	dbref player, p2;
	static char name[50];

	/*
	 * Nuke extra guests as new guests connect. 
	 */

	for (i = 0; i < mudconf.number_guests; i++) {
		sprintf(name, "%s%d", mudconf.guest_prefix, i + 1);
		p2 = lookup_player(GOD, name, 0);
		if (p2 != NOTHING && !Connected(p2))
			destroy_guest(p2);
	}

	/*
	 * Locate a free guest ID, and eat it. 
	 */

	for (i = 0; i < mudconf.number_guests; i++) {
		sprintf(name, "%s%d", mudconf.guest_prefix, i + 1);
		player = lookup_player(GOD, name, 0);
		if (player == NOTHING)
			break;
	}

	if (i == mudconf.number_guests) {
		queue_string(d, "GAME: All guest ID's are busy, please try again later.\n");
		return NULL;
	}
	sprintf(name, "%s%d", mudconf.guest_prefix, i + 1);
	player = create_guest(name, mudconf.guest_prefix);

	if (player == NOTHING) {
		queue_string(d, "GAME: Error creating guest ID, please try again later.\n");
		log_text(tprintf("GUEST: Error creating guest ID. '%s' already exists.\n",
				 name));
		return NULL;
	}
	return Name(player);
}
