/* comsys.h */
/* $Id$ */

#ifndef __COMSYS_H__
#define __COMSYS_H__

#define NUM_COMSYS 500

struct comsys *comsys_table[NUM_COMSYS];


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

struct comsys
{
    dbref who;

    int numchannels;
    int maxchannels;
    char *alias;
    char **channels;

    struct comsys *next;
};

#define NUM_COMSYS 500

struct comsys *comsys_table[NUM_COMSYS];

void load_comsystem ();
void save_comsystem ();
void purge_comsystem();

void sort_com_aliases();
struct comsys *get_comsys ();
struct comsys *create_new_comsys ();
void destroy_comsys ();
void add_comsys ();
void del_comsys ();
void save_comsys ();
void load_comsys ();
void purge_comsys();
void sort_com_aliases();
struct comsys *get_comsys ();
struct comsys *create_new_comsys ();
void destroy_comsys ();
void add_comsys ();
void del_comsys ();
void save_comsys ();
void load_comsys ();
void save_channels ();
void load_channels ();
void load_old_channels ();

int num_channels;
int max_channels;
struct channel **channels;

struct channel *select_channel();

struct comuser *select_user();

void do_comdisconnectchannel();

void do_setnewtitle();
void do_comwho();
void do_joinchannel();
void do_leavechannel();
void do_comsend();
void do_chanobj();

char *get_channel_from_alias();

void add_lastcom();

void sort_channels();
void sort_users();
void check_channel();
void add_spaces();
void do_delcomchannel();

void do_processcom();
void send_csdebug();

int do_comsystem();
void do_comconnectchannel();
void do_chclose();
void do_chloud();
void do_chsquelch();
void do_comdisconnectnotify();
void do_comconnectnotify();
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
