/* walkdb.h - structures used in commands that walk the entire db */
/* $Id$ */

#include "copyright.h"

#ifndef __WALKDB_H
#define __WALKDB_H

/* Search structure, used by @search and search(). */

typedef struct search_type SEARCH;
struct search_type {
	int	s_wizard;
	dbref	s_owner;
	dbref	s_rst_owner;
	int	s_rst_type;
	FLAGSET	s_fset;
	POWERSET s_pset;
	dbref	s_parent;
	dbref   s_zone;
	char	*s_rst_name;
	char	*s_rst_eval;
	int	low_bound;
	int	high_bound;
};

/* Stats structure, used by @stats and stats(). */

typedef struct stats_type STATS;
struct stats_type {
	int	s_total;
	int	s_rooms;
	int	s_exits;
	int	s_things;
	int	s_players;
	int	s_going;
	int	s_garbage;
	int	s_unknown;
};

extern int	FDECL(search_setup, (dbref, char *, SEARCH *));
extern void	FDECL(search_perform, (dbref, dbref, SEARCH *));
extern int	FDECL(get_stats, (dbref, dbref, STATS *));

#endif /* __WALKDB_H */
