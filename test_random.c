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
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "chronoqueue.h"
#include "package.h"

int g_shutdown = 0;

static void alarm_handler(int signum)
{
/*
	(void) alarm(1);
*/
}

static void sig_handler(int signum)
{
	switch(signum) {
		case SIGHUP:
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			g_shutdown = 1;
			break;
	}
}

static void print_and_destroy(cqnode_t *p)
{
	char ts[64];

	if(!p) { return; }
	print_ts(p, &ts[0], sizeof(ts));
	printf("%s (%ld) {%ld}\n", ts, pipeline_count(), pipeline_span());
	free(p->pkg);
	destroy_orphan(p);
}

static void add_random(long limit)
{
	struct timespec ts;
	cqtime_t *cqts;
	dp_t *dp;

	/* Create a data package */
	dp = (dp_t *)malloc(sizeof(dp_t));
	dp->buf = (unsigned char *)"?";
	dp->len = 2;

	/* Generate a random timestamp */
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#ifdef CHRONOQUEUE_MICROSECOND
	ts.tv_nsec /= 1000;
	ts.tv_nsec += (rand()%500);
#else
	ts.tv_nsec += (rand()%500000);
#endif

	while(ts.tv_nsec > SSMAX) {
		ts.tv_sec += 1;
		ts.tv_nsec -= SSPERSEC;
	}

	/* Convert to cqtime_t in case of CHRONOQUEUE_MICROSECOND */
	cqts = malloc(sizeof(cqtime_t));
	CQTSEC(cqts) = ts.tv_sec;
	CQTSS(cqts) = ts.tv_nsec;

	/* Insert data package into the pipeline */
	(void)pipeline_insert(dp, cqts);
	free(cqts);
	if(pipeline_count() > limit) { g_shutdown = 1; }
}

int main(int argc, char *argv[])
{
	cqnode_t *p;

	if(argc != 2) { fprintf(stderr, "%s: <limit>\n", argv[0]); return 1; }

	srand(time(NULL));

	signal(SIGINT,	sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT,	sig_handler);
	signal(SIGHUP,	sig_handler);
	signal(SIGALRM, alarm_handler);
	(void) alarm(1);

	while(!g_shutdown) { add_random(atol(argv[1])); }

	printf("\n");
	while( (p = pipeline_pop(CQ_OLDEST)) ) {
		print_and_destroy(p);
	}

	return 0;
}
