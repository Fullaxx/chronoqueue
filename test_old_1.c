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

#include <stdio.h>
#include <stdlib.h>

#include "chronoqueue.h"
#include "package.h"

static void print_and_destroy(cqnode_t *p)
{
	char ts[64];
	dp_t *dp;

	if(!p) { return; }
	dp = (dp_t *)p->pkg;
	print_ts(p, &ts[0], sizeof(ts));
	printf("%s %s: (%ld) {%ld}\n", ts, dp->buf, pipeline_count(), pipeline_span());
	free(dp);
	destroy_orphan(p);
}

static dp_t* gen_dp(char *str, long len)
{
	dp_t *dp;

	/* Create a data package */
	dp = (dp_t *)malloc(sizeof(dp_t));
	dp->buf = (unsigned char *)str;
	dp->len = 2;

	return dp;
}

int main(int argc, char *argv[])
{
	cqtime_t *ts;
	cqnode_t *p;

	/* This should return NULL */
	p = pipeline_pop(CQ_OLDEST);
	if(p) { fprintf(stderr, "ERROR!\n"); exit(1); }

	ts = malloc(sizeof(cqtime_t));

	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 100;
	(void)pipeline_insert(gen_dp("D", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 200;
	(void)pipeline_insert(gen_dp("F", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 150;
	(void)pipeline_insert(gen_dp("E", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 300;
	(void)pipeline_insert(gen_dp("G", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 400;
	(void)pipeline_insert(gen_dp("H", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 1;
	(void)pipeline_insert(gen_dp("C", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1179; CQTSS(ts) = 1;
	(void)pipeline_insert(gen_dp("A", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1179; CQTSS(ts) = 1;
	(void)pipeline_insert(gen_dp("B", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	CQTSEC(ts) = 1180; CQTSS(ts) = 400;
	(void)pipeline_insert(gen_dp("I", 2), ts);
	printf("count: %ld {%ld}\n", pipeline_count(), pipeline_span());

	printf("\n");

	while( (p = pipeline_pop(CQ_OLDEST)) ) {
		print_and_destroy(p);
	}

	free(ts);
	return 0;	
}
