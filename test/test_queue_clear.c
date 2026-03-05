#include <assert.h>
#include <stdint.h>
#include "queue_conf.h"
#include "queue.h"

typedef struct {
        uint32_t id;
        uint8_t dlc;
        uint8_t data[8];
} can_msg_t;

QUEUE_DEFINE(can_msg_queue, can_msg_t, 4)

static can_msg_t
make_msg(uint32_t id, uint8_t seed)
{
        can_msg_t m = {.id = id,
                       .dlc = 8,
                       .data = {seed, seed + 1, seed + 2, seed + 3, seed + 4,
                                seed + 5, seed + 6, seed + 7}};
        return m;
}

int
main(void)
{
        can_msg_queue_t q;
        can_msg_queue_init(&q);
        // enqueue two items
        can_msg_t m1 = make_msg(1, 10);
        can_msg_t m2 = make_msg(2, 20);
        assert(can_msg_queue_enqueue(&q, &m1) == QUEUE_STATUS_OK);
        assert(can_msg_queue_enqueue(&q, &m2) == QUEUE_STATUS_OK);
        // clear using macro
        QUEUE_CLEAR(can_msg_queue, &q);
        assert(can_msg_queue_is_empty(&q));
        assert(can_msg_queue_count(&q) == 0);
        // enqueue again and clear using macro
        assert(can_msg_queue_enqueue(&q, &m1) == QUEUE_STATUS_OK);
        QUEUE_CLEAR(can_msg_queue, &q);
        assert(can_msg_queue_is_empty(&q));
        assert(can_msg_queue_count(&q) == 0);
        return 0;
}
