#define QUEUE_OVERWRITE_ON_FULL 0
#include "queue_conf.h"
#include <assert.h>
#include <stdio.h>
#include "queue.h"

QUEUE_DEFINE(cycle_q, int, 3)

int
main(void)
{
        cycle_q_t q;
        cycle_q_init(&q);

        /* Cycle 1: fill and drain */
        for (int i = 0; i < 3; i++) {
                assert(cycle_q_enqueue(&q, &i) == QUEUE_STATUS_OK);
        }
        for (int i = 0; i < 3; i++) {
                int out;
                assert(cycle_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
                assert(out == i);
        }
        assert(cycle_q_is_empty(&q));

        /* Cycle 2: fill and drain */
        for (int i = 10; i < 13; i++) {
                assert(cycle_q_enqueue(&q, &i) == QUEUE_STATUS_OK);
        }
        for (int i = 10; i < 13; i++) {
                int out;
                assert(cycle_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
                assert(out == i);
        }
        assert(cycle_q_is_empty(&q));

        /* Cycle 3: partial operations */
        int out, val1 = 100, val2 = 200;
        assert(cycle_q_enqueue(&q, &val1) == QUEUE_STATUS_OK);
        assert(cycle_q_enqueue(&q, &val2) == QUEUE_STATUS_OK);
        assert(cycle_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 100);
        assert(cycle_q_count(&q) == 1);
        assert(cycle_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 200);
        assert(cycle_q_is_empty(&q));

        printf("Empty after cycles test passed.\n");
        return 0;
}
