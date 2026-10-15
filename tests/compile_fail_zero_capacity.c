/* SPDX-License-Identifier: MIT */

/* A queue with a capacity of 0 must not compile. The build checks that this
 * file fails to compile. */
#define QUEUE_SINGLE_CONTEXT 1
#include "queue_conf.h"

#include "queue.h"

QUEUE_DEFINE(zero_q, int, 0)
