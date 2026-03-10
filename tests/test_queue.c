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

static void
assert_can_msg_eq(const can_msg_t *a, const can_msg_t *b)
{
        assert(a->id == b->id);
        assert(a->dlc == b->dlc);
        for (size_t i = 0; i < 8U; i++) {
                assert(a->data[i] == b->data[i]);
        }
}

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
        can_msg_queue_t q = {0};
        can_msg_queue_init(&q);
        assert(can_msg_queue_capacity() == 3U);
        assert(can_msg_queue_is_empty(&q));
        assert(can_msg_queue_count(&q) == 0U);

        can_msg_t out = {0};
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_EMPTY);
        assert(can_msg_queue_dequeue(&q, NULL) == QUEUE_STATUS_BAD_ARG);
        assert(can_msg_queue_enqueue(NULL, &out) == QUEUE_STATUS_BAD_ARG);
        assert(can_msg_queue_enqueue(&q, NULL) == QUEUE_STATUS_BAD_ARG);

        const can_msg_t m1 = make_can_msg(1U, 10U);
        const can_msg_t m2 = make_can_msg(2U, 20U);
        const can_msg_t m3 = make_can_msg(3U, 30U);
        const can_msg_t m4 = make_can_msg(4U, 40U);

        assert(can_msg_queue_enqueue(&q, &m1) == QUEUE_STATUS_OK);
        assert(can_msg_queue_enqueue(&q, &m2) == QUEUE_STATUS_OK);
        assert(can_msg_queue_enqueue(&q, &m3) == QUEUE_STATUS_OK);
        assert(can_msg_queue_is_full(&q));
        assert(can_msg_queue_count(&q) == 3U);

        /* Overwrite-on-full (default): enqueuing drops oldest (m1). */
        assert(can_msg_queue_enqueue(&q, &m4) == QUEUE_STATUS_OVERWROTE);
        assert(can_msg_queue_count(&q) == 3U);

        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert_can_msg_eq(&out, &m2);
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert_can_msg_eq(&out, &m3);
        assert(can_msg_queue_dequeue(&q, &out) == QUEUE_STATUS_OK);
        assert_can_msg_eq(&out, &m4);
        assert(can_msg_queue_is_empty(&q));

        printf("All overwrite-mode tests passed.\n");
        return 0;
}
