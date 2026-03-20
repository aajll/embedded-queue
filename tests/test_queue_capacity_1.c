#define QUEUE_OVERWRITE_ON_FULL 1
#include "queue_conf.h"
#include <assert.h>
#include <stdio.h>
#include "queue.h"

QUEUE_DEFINE(cap1_q, int, 1)

int
main(void)
{
        cap1_q_t q;
        cap1_q_init(&q);

        int val1 = 100, val2 = 200;

        /* Enqueue fills the single slot */
        assert(cap1_q_enqueue(&q, &val1) == QUEUE_STATUS_OK);
        assert(cap1_q_is_full(&q));
        assert(cap1_q_count(&q) == 1);

        /* Enqueue again should overwrite */
        assert(cap1_q_enqueue(&q, &val2) == QUEUE_STATUS_OVERWROTE);
        assert(cap1_q_count(&q) == 1);

        /* Dequeue should return val2, not val1 */
        int out;
        assert(cap1_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == val2);

        printf("Capacity-1 queue test passed.\n");
        return 0;
}
