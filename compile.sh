#!/bin/bash

CFLAGS="-Wall"
OPTCFLAGS="${CFLAGS} -O2"
DBGCFLAGS="${CFLAGS} -ggdb3 -DDEBUG"

rm -f *.dbg

gcc ${DBGCFLAGS} -DDEBUG_INSERT test_new_1.c chronoqueue.c -o test_new_1.ns.dbg
gcc ${DBGCFLAGS} -DDEBUG_INSERT test_old_1.c chronoqueue.c -o test_old_1.ns.dbg
gcc ${DBGCFLAGS} -DDEBUG_INSERT test_random.c chronoqueue.c -o test_random.ns.dbg

gcc ${DBGCFLAGS} -DSPAN_BOUNDED sliding_window.c chronoqueue.c -lpthread -o sliding_window_span.ns.dbg
gcc ${DBGCFLAGS} -DCOUNT_BOUNDED sliding_window.c chronoqueue.c -lpthread -o sliding_window_count.ns.dbg

DBGCFLAGS+=" -DCHRONOQUEUE_MICROSECOND"
gcc ${DBGCFLAGS} -DDEBUG_INSERT test_new_1.c chronoqueue.c -o test_new_1.us.dbg
gcc ${DBGCFLAGS} -DDEBUG_INSERT test_old_1.c chronoqueue.c -o test_old_1.us.dbg
gcc ${DBGCFLAGS} -DDEBUG_INSERT test_random.c chronoqueue.c -o test_random.us.dbg
gcc ${DBGCFLAGS} -DSPAN_BOUNDED sliding_window.c chronoqueue.c -lpthread -o sliding_window_span.us.dbg
gcc ${DBGCFLAGS} -DCOUNT_BOUNDED sliding_window.c chronoqueue.c -lpthread -o sliding_window_count.us.dbg
