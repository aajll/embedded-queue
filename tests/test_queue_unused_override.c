/*
 * SPDX-License-Identifier: MIT
 *
 * A consumer-supplied QUEUE__UNUSED must be respected without a macro
 * redefinition. This file is built with -Werror, so a redefinition warning
 * fails the build. Only some generated helpers are called, so the override
 * must also suppress unused-function diagnostics.
 */
#define QUEUE__UNUSED __attribute__((__unused__))
#include <assert.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

QUEUE_DEFINE(subset_q, int, 4)

int
main(void)
{
        subset_q_t q;
        int in = 7;
        int out = 0;

        subset_q_init(&q);
        assert(subset_q_enqueue(&q, &in) == QUEUE_STATUS_OK);
        assert(subset_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 7);

        printf("QUEUE__UNUSED override test passed.\n");
        return 0;
}
