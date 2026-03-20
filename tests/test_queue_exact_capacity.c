#define QUEUE_OVERWRITE_ON_FULL 1
#include "queue_conf.h"
#include <assert.h>
#include <stdio.h>
#include "queue.h"

QUEUE_DEFINE(exact_q, int, 4)

int
main(void)
{
        exact_q_t q;
        exact_q_init(&q);

        /* Fill to exact capacity */
        for (int i = 0; i < 4; i++) {
                assert(exact_q_enqueue(&q, &i) == QUEUE_STATUS_OK);
        }
        assert(exact_q_is_full(&q));
        assert(exact_q_count(&q) == 4);

        /* Dequeue one, enqueue one - should overwrite oldest */
        int out, val = 100;
        assert(exact_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 0);

        /* Queue is now 3/4 full, enqueue should succeed (not overwrite) */
        assert(exact_q_enqueue(&q, &val) == QUEUE_STATUS_OK);
        assert(exact_q_count(&q) == 4);

        /* Verify order: 1, 2, 3, 100 */
        for (int i = 1; i <= 3; i++) {
                assert(exact_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
                assert(out == i);
        }
        assert(exact_q_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out == 100);

        printf("Exact capacity test passed.\n");
        return 0;
}
