#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

typedef struct {
        uint32_t id;
        uint8_t dlc;
        uint8_t data[8];
} can_msg_t;

QUEUE_DEFINE(can_msg_queue, can_msg_t, 3)

static can_msg_t
make_can_msg(uint32_t id, uint8_t seed)
{
        can_msg_t m = {
            .id = id,
            .dlc = 8,
            .data = {seed, seed + 1U, seed + 2U, seed + 3U, seed + 4U,
                     seed + 5U, seed + 6U, seed + 7U},
        };
        return m;
}

int
main(void)
{
        can_msg_queue_t q;
        can_msg_queue_init(&q);

        const can_msg_t m1 = make_can_msg(1U, 10U);
        const can_msg_t m2 = make_can_msg(2U, 20U);
        const can_msg_t m3 = make_can_msg(3U, 30U);
        const can_msg_t m4 = make_can_msg(4U, 40U);

        assert(can_msg_queue_enqueue(&q, &m1) == QUEUE_STATUS_OK);
        assert(can_msg_queue_enqueue(&q, &m2) == QUEUE_STATUS_OK);
        assert(can_msg_queue_enqueue(&q, &m3) == QUEUE_STATUS_OK);
        assert(can_msg_queue_is_full(&q));

        /* Fail-on-full mode: new element is rejected, queue unchanged. */
        assert(can_msg_queue_enqueue(&q, &m4) == QUEUE_STATUS_FULL);
        assert(can_msg_queue_count(&q) == 3U);

        can_msg_t out = {0};
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out.id == m1.id);
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out.id == m2.id);
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert(out.id == m3.id);
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_EMPTY);

        printf("All fail-on-full tests passed.\n");
        return 0;
}
