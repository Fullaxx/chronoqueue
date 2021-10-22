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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include "chronoqueue.h"
#include "package.h"

int g_shutdown = 0;
int g_thread_count = 4;
int g_delay = 100;

static void shutdown_message(char *what)
{
	g_shutdown = 1;
	usleep(10000);
	fprintf(stderr, "%s failure!\n", what);
	exit(1);
}

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

static void add_random(void)
{
	unsigned int r;
	struct timespec ts;
	cqtime_t *cqts;
	dp_t *dp;

	// Create a data package
	dp = (dp_t *)malloc(sizeof(dp_t));
	dp->buf = (unsigned char *)"?";
	dp->len = 2;

	// Generate a random timestamp
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	r = rand();

#ifdef CHRONOQUEUE_MICROSECOND
	ts.tv_nsec /= 1000;
	ts.tv_nsec += (r%500);
#else
	ts.tv_nsec += (r%500000);
#endif

	while(ts.tv_nsec > SSMAX) {
		ts.tv_sec += 1;
		ts.tv_nsec -= SSPERSEC;
	}

	// Convert to cqtime_t in case of CHRONOQUEUE_MICROSECOND
	cqts = malloc(sizeof(cqtime_t));
	CQTSEC(cqts) = ts.tv_sec;
	CQTSS(cqts) = ts.tv_nsec;

	// Insert data package into the pipeline
	(void)pipeline_insert(dp, cqts);
	free(cqts);
}

static void* gen_data(void *p)
{
	//

	while(g_shutdown == 0) {
		add_random();
		usleep(g_delay);
	}

	return NULL;
}

static int launch_threads(void)
{
	int err, c;
	pthread_attr_t attr;
	pthread_t thr_id;

	err = pthread_attr_init(&attr);
	if(err) { perror("pthread_attr_init()"); return -1; }

	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(err) { perror("pthread_attr_setdetachstate()"); return -2; }

	c = g_thread_count;
	while(c-- > 0) {
		err = pthread_create(&thr_id, &attr, &gen_data, NULL);
		if(err) { perror("pthread_create()"); return -3; }
		(void)pthread_setname_np(thr_id, "gen_data");
	}

	return 0;
}

#ifdef COUNT_BOUNDED
static void check_pipeline_count(long limit)
{
	cqnode_t *p;
	long total, c;

	total = pipeline_count();
	c = total - limit;
	if(c < 1) { printf("idle\n"); usleep(25); return; }
	while(c-- > 0) {
		p = pipeline_pop(CQ_OLDEST);
		print_and_destroy(p);
	}
}
#endif

#ifdef SPAN_BOUNDED
static void check_pipeline_span(long limit)
{
	cqnode_t *p;
	long span, diff, pps, c;

	pps = (1000000*g_thread_count)/g_delay;
	span = pipeline_span();
	if(span > limit) {
		diff = span - limit;
		printf("Status %ld - %ld = %ld\n", span, limit, diff);
		c = (diff * pps) / SSPERSEC;
		if(c > 0) {
			printf("Removing (%ld * %ld) / %ld = %ld\n", diff, pps, SSPERSEC, c);
			while(c-- > 0) {
				p = pipeline_pop(CQ_OLDEST);
				print_and_destroy(p);
			}
		} else { printf("idle\n"); usleep(25); }
	}
}
#endif

int main(int argc, char *argv[])
{
	int err;
	cqnode_t *p;

	srand(time(NULL));

#ifdef SPAN_BOUNDED
	if(argc != 4) { fprintf(stderr, "%s: <num threads> <usleep per thread> <span limit>\n", argv[0]); return 1; }
#endif
#ifdef COUNT_BOUNDED
	if(argc != 4) { fprintf(stderr, "%s: <num threads> <usleep per thread> <count limit>\n", argv[0]); return 1; }
#endif
	g_thread_count = atoi(argv[1]);
	g_delay = atoi(argv[2]);

	err = launch_threads();
	if(err) { shutdown_message("launch_threads()"); }

	signal(SIGINT,	sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT,	sig_handler);
	signal(SIGHUP,	sig_handler);
	signal(SIGALRM, alarm_handler);
	(void) alarm(1);

#ifdef SPAN_BOUNDED
	while(!g_shutdown) { check_pipeline_span(atol(argv[3])); }
#endif

#ifdef COUNT_BOUNDED
	while(!g_shutdown) { check_pipeline_count(atoi(argv[3])); }
#endif

	usleep(1000);
	printf("\n");
	printf("EMPTY THE PIPELINE\n");
	while( (p = pipeline_pop(CQ_OLDEST)) ) {
		print_and_destroy(p);
	}

	return 0;
}
