/* comsys.h */
/* $Id$ */

#include "commac.h"
#ifndef __COMSYS_H__
#define __COMSYS_H__

typedef struct chanentry CHANENT;
struct chanentry {
	char *channame;
	struct channel *chan;
};

#define CHAN_NAME_LEN 50
struct comuser
{
    dbref who;
    int on;
    char *title;
    struct comuser *on_next;
};

struct channel
{
    char name[CHAN_NAME_LEN];
    int type;
    int temp1;
    int temp2;
    int charge;
    int charge_who;
    int amount_col;
    int num_users;
    int max_users;
    int chan_obj;
    struct comuser **users;
    struct comuser *on_users;   /* Linked list of who is on */
    int num_messages;
};

int num_channels;
int max_channels;
struct channel **channels;

struct channel *select_channel();

struct comuser *select_user();

char * do_comdisconnectchannel();

char * do_setnewtitle();
char * do_comwho();
char * do_joinchannel();
char * do_leavechannel();
char * do_comsend();
char * do_chanobj();

char * get_channel_from_alias();

char * add_lastcom();

char * sort_channels();
char * sort_users();
char * check_channel();
char * add_spaces();
char * do_delcomchannel();

char * do_processcom();
char * send_csdebug();

int do_comsystem();
char * do_comconnectchannel();
char * do_chclose();
char * do_chloud();
char * do_chsquelch();
char * do_comdisconnectnotify();
char * do_comconnectnotify();
void do_chanlist();
#define CHANNEL_JOIN 0x1
#define CHANNEL_TRANSMIT 0x2
#define CHANNEL_RECIEVE 0x4

#define CHANNEL_PL_MULT 0x1
#define CHANNEL_OBJ_MULT 0x10
#define CHANNEL_LOUD 0x100
#define CHANNEL_PUBLIC 0x200

#define UNDEAD(x) (((!God(Owner(x))) || !(Going(x))) && \
	    ((Typeof(x) != TYPE_PLAYER) || (Connected(x))))

/* explanation of logic... If it's not owned by god, and it's either not a
player, or a connected player, it's good... If it is owned by god, then if
it's going, assume it's already gone, no matter what it is. :) */
#endif /* __COMSYS_H__ */
