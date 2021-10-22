/*
	A chronologically ordered linked-list queue written in C
	Copyright (C) 2021 Brett Kuskie <fullaxx@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __CHRONOQUEUE_H__
#define __CHRONOQUEUE_H__

#include <time.h>

/**
Define a CQ specific time struct and directives to support using microsecond or nanosecond resolution
SSPERSEC is the number of sub-seconds in one second.
SSMAX    is the max value allowed for the sub-seconds.
CQTSEC   is the accessor for the seconds field.
CQTSS    is the accessor for the sub-second field.
CQTSFMT  is the correct printf format.
*/

#ifdef CHRONOQUEUE_MICROSECOND

typedef struct timeval cqtime_t;
#define SSPERSEC (1000000L)
#define CQTSS(tsp) (tsp->tv_usec)
#define CQTSFMT "%ld.%06ld"

#else

typedef struct timespec cqtime_t;
#define SSPERSEC (1000000000L)
#define CQTSS(tsp) (tsp->tv_nsec)
#define CQTSFMT "%ld.%09ld"

#endif

#define SSMAX (SSPERSEC-1)
#define CQTSEC(tsp) (tsp->tv_sec)

typedef struct chronoqueue_node {
	void *pkg;
	cqtime_t *ts;
	struct chronoqueue_node *older;
	struct chronoqueue_node *newer;
} cqnode_t;

int pipeline_insert(void *, cqtime_t *);
long pipeline_count(void);
long pipeline_span(void);
cqnode_t *pipeline_pop(int);
int print_ts(cqnode_t *, char *, int);
void destroy_orphan(cqnode_t *);

#define CQ_OLDEST (0)
#define CQ_NEWEST (1)

#endif
