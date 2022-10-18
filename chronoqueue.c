/*
	A chronologically ordered linked-list queue written in ANSI C
	Copyright (C) 2022 Brett Kuskie <fullaxx@gmail.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "chronoqueue.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define PIPELOCK	pthread_mutex_lock(&mutex)
#define PIPEUNLOCK 	pthread_mutex_unlock(&mutex)

/**
	g_newest is a pointer to the newest element in the pipeline
	g_oldest is a pointer to the oldest element in the pipeline
	g_count is an updated count of elements in the pipeline

	     g_newest                                    g_oldest
	|----------------|    |----------------|    |----------------|
	| 1181.000000500 | -> | 1181.000000100 | -> | 1180.000000900 |
	|----------------|    |----------------|    |----------------|
*/
cqnode_t *g_newest = NULL;
cqnode_t *g_oldest = NULL;
unsigned long g_count = 0;

/**
	Calculate the time difference in sub-seconds between a and b
	Positive if a is newer than b
	Negative if b is newer than a
*/
#define TIMEDIFF(a, b) (((CQTSEC(a)-CQTSEC(b))*SSPERSEC)+(CQTSS(a)-CQTSS(b)))

static inline void bail(char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(EXIT_FAILURE);
}

static inline void* new_cqnode(void)
{
	void *p = calloc(1, sizeof(cqnode_t));
	if(!p) { bail("calloc() failed!"); }
	return p;
}

/**
	Search our list for the chronologically correct insertion point
	If the new timestamp is newest, then return NULL
	Otherwise return the location that will precede our new timestamp
*/
static cqnode_t* find_predecessor(cqtime_t *ts)
{
	cqnode_t *cursor;

	if(TIMEDIFF(ts, g_newest->ts) >= 0) { return NULL; }

	cursor = g_newest;
	while(cursor->older) {
		if(TIMEDIFF(ts, cursor->older->ts) >= 0) { break; }
		cursor = cursor->older;
	}

	return cursor;
}

/**
	Insert a timestamped data package into the pipeline
	Push all older elements to the right
	asterisks are printed if we updated a global edge pointer
	ellipsis are printed otherwise
*/
int pipeline_insert(void *pkg, cqtime_t *ts)
{
	int n = 0;
	char log[128];
	cqnode_t *cursor = NULL;
	cqnode_t *pre = NULL;
	cqnode_t *post = NULL;

	/* This is unacceptable for timestamp differential checking */
	if(CQTSS(ts) > SSMAX) { return 1; }

	PIPELOCK;

	if(!g_newest) {
		/* Create the head element */
		cursor = g_newest = new_cqnode();
	} else {
		/* Find the chronologically correct insertion point */
		pre = find_predecessor(ts);
		cursor = new_cqnode();
		if(!pre) {
			/* cursor is newer than g_newest */
			post = g_newest;
			g_newest = cursor;
		} else {
			/* cursor is older than pre */
			n += snprintf(&log[n], 128-n, "... ");
			n += snprintf(&log[n], 128-n, CQTSFMT, CQTSEC(pre->ts), CQTSS(pre->ts));
			n += snprintf(&log[n], 128-n, " -> ");
			post = pre->older;
			pre->older = cursor;
			cursor->newer = pre;
		}
	}

	if(!pre) { n += snprintf(&log[n], 128-n, "*** "); }
	n += snprintf(&log[n], 128-n, CQTSFMT, CQTSEC(ts), CQTSS(ts));

	if(post) {
		cursor->older = post;
		post->newer = cursor;
		n += snprintf(&log[n], 128-n, " -> ");
		n += snprintf(&log[n], 128-n, CQTSFMT, CQTSEC(post->ts), CQTSS(post->ts));
		n += snprintf(&log[n], 128-n, " ...");
	} else {
		/* cursor is oldest */
		g_oldest = cursor;
		n += snprintf(&log[n], 128-n, " ***");
	}

	/* Fill in all the details */
	cursor->ts = malloc(sizeof(cqtime_t));
	CQTSEC(cursor->ts) = CQTSEC(ts);
	CQTSS(cursor->ts) = CQTSS(ts);
	cursor->pkg = pkg;
	g_count++;

	PIPEUNLOCK;

#ifdef DEBUG_INSERT
	fprintf(stderr, "INSERT: %s\n", log);
#endif
	return 0;
}

/* How many elements in the pipeline? */
long pipeline_count(void)
{
	return g_count;
}

/* What is the timespan of the pipeline? */
long pipeline_span(void)
{
	long diff;

	if(!g_newest || !g_oldest) { return 0; }

	PIPELOCK;
	diff = TIMEDIFF(g_newest->ts, g_oldest->ts);
	PIPEUNLOCK;

	return diff;
}

/**
	Pop an edge element without pipeline traversal
	NEWEST: Return the first element in the pipeline, if any
	NEWEST: Reset g_newest to second element in the pipeline, if any
	OLDEST: Return the last element in the pipeline, if any
	OLDEST: Reset g_oldest to the second-to-last element in the pipeline, if any
*/
cqnode_t* pipeline_pop(int which)
{
	cqnode_t *cursor = NULL;

	if((which != CQ_OLDEST) && (which != CQ_NEWEST)) { return NULL; }

	PIPELOCK;
	if(which == CQ_NEWEST) {
		if(g_newest) {
			cursor = g_newest;
			g_newest = g_newest->older;					/* Update g_newest ptr */
			if(g_newest) { g_newest->newer = NULL; }	/* Now there is nothing newer */
			else { g_oldest = NULL; }					/* If there is nothing older, update g_oldest */
			g_count--;
		}
	}
	if(which == CQ_OLDEST) {
		if(g_oldest) {
			cursor = g_oldest;
			g_oldest = g_oldest->newer;					/* Update g_oldest ptr */
			if(g_oldest) { g_oldest->older = NULL; }	/* Now there is nothing older */
			else { g_newest = NULL; }					/* If there is nothing newer, update g_newest */
			g_count--;
		}
	}
	PIPEUNLOCK;

	if(cursor) {
		/* cursor is now an orphan */
		cursor->newer = NULL;
		cursor->older = NULL;
	}
	return cursor;
}

int print_ts(cqnode_t *p, char *buf, int len)
{
	return snprintf(buf, len, CQTSFMT, CQTSEC(p->ts), CQTSS(p->ts));
}

/**
	Any element that gets pipeline_pop()'d and handled
	should get passed to destroy_orphan() after usage for memory reclamation
*/
void destroy_orphan(cqnode_t *p)
{
	if(p) { free(p->ts); free(p); }
}
