#define QUEUE_OVERWRITE_ON_FULL 0
#include <assert.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

typedef int elem_t;
QUEUE_DEFINE(fail_q, elem_t, 2)

int
main(void)
{
        fail_q_t q;
        fail_q_init(&q);

        elem_t a = 10, b = 20, c = 30;
        /* Fill the queue */
        assert(fail_q_enqueue(&q, &a) == QUEUE_STATUS_OK);
        assert(fail_q_enqueue(&q, &b) == QUEUE_STATUS_OK);
        assert(fail_q_is_full(&q));
        assert(fail_q_count(&q) == 2);

        /* Try to add a third element – should be rejected */
        assert(fail_q_enqueue(&q, &c) == QUEUE_STATUS_FULL);
        assert(fail_q_count(&q) == 2); /* unchanged */

        /* Dequeue and verify order */
        elem_t out;
        assert(fail_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == a);
        assert(fail_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == b);
        assert(fail_q_is_empty(&q));

        printf("Fail‑on‑full mode test passed.\n");
        return 0;
}
