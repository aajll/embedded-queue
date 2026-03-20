#define QUEUE_OVERWRITE_ON_FULL 1
#include "queue_conf.h"
#include <assert.h>
#include <stdio.h>
#include "queue.h"

QUEUE_DEFINE(null_q, int, 10)

int
main(void)
{
        int val = 42;
        null_q_t q;
        null_q_init(&q);

        /* Test all functions with NULL queue pointer */
        (void)null_q_init(NULL);  /* Should be safe no-op */
        (void)null_q_clear(NULL); /* Should be safe no-op */
        assert(null_q_is_empty(NULL) == true);
        assert(null_q_is_full(NULL) == false);
        assert(null_q_count(NULL) == 0);

        /* Test enqueue with NULL queue */
        assert(null_q_enqueue(NULL, &val) == QUEUE_STATUS_BAD_ARG);
        assert(null_q_enqueue(&q, NULL) == QUEUE_STATUS_BAD_ARG);

        /* Test dequeue with NULL queue */
        assert(null_q_dequeue(NULL, &val) == QUEUE_STATUS_BAD_ARG);
        assert(null_q_dequeue(&q, NULL) == QUEUE_STATUS_BAD_ARG);

        /* Add an element to test dequeue with valid arguments */
        assert(null_q_enqueue(&q, &val) == QUEUE_STATUS_OK);

        /* Test dequeue with NULL output buffer */
        int out;
        assert(null_q_dequeue(&q, NULL) == QUEUE_STATUS_BAD_ARG);

        /* Dequeue the element */
        assert(null_q_dequeue(&q, &out) == QUEUE_STATUS_OK);

        /* Test dequeue on empty queue returns EMPTY, not BAD_ARG */
        assert(null_q_dequeue(&q, &out) == QUEUE_STATUS_EMPTY);

        printf("NULL validation test passed.\n");
        return 0;
}
