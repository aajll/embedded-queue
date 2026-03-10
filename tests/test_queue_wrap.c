#include <assert.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

typedef int elem_t;
QUEUE_DEFINE(wrap_q, elem_t, 4)

int
main(void)
{
        wrap_q_t q;
        wrap_q_init(&q);
        assert(wrap_q_is_empty(&q));

        /* Fill the buffer */
        for (int i = 0; i < 4; ++i) {
                elem_t v = i;
                assert(wrap_q_enqueue(&q, &v) == QUEUE_STATUS_OK);
        }
        assert(wrap_q_is_full(&q));
        assert(wrap_q_count(&q) == 4);

        /* Dequeue one element (value 0) */
        elem_t out = -1;
        assert(wrap_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 0);
        assert(wrap_q_count(&q) == 3);
        assert(!wrap_q_is_full(&q));

        /* Enqueue a new element – this forces the ring to wrap */
        elem_t v = 99;
        assert(wrap_q_enqueue(&q, &v) == QUEUE_STATUS_OK);
        assert(wrap_q_is_full(&q));
        assert(wrap_q_count(&q) == 4);

        /* Dequeue the rest – they must appear in FIFO order */
        int expected[] = {1, 2, 3, 99};
        for (size_t i = 0; i < 4; ++i) {
                assert(wrap_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
                assert(out == expected[i]);
        }
        assert(wrap_q_is_empty(&q));

        printf("Wrap‑around test passed.\n");
        return 0;
}
