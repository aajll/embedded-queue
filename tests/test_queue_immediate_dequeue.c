#define QUEUE_OVERWRITE_ON_FULL 0
#include "queue_conf.h"
#include <assert.h>
#include <stdio.h>
#include "queue.h"

QUEUE_DEFINE(immediate_q, int, 5)

int
main(void)
{
        immediate_q_t q;
        immediate_q_init(&q);

        /* Fill partially */
        for (int i = 0; i < 3; i++) {
                assert(immediate_q_enqueue(&q, &i) == QUEUE_STATUS_OK);
        }

        /* Dequeue all immediately - no wrap scenario */
        int out;
        for (int i = 0; i < 3; i++) {
                assert(immediate_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
                assert(out == i);
        }
        assert(immediate_q_is_empty(&q));

        /* Verify head and tail are at same position */
        assert(immediate_q_count(&q) == 0);

        printf("Immediate dequeue test passed.\n");
        return 0;
}
