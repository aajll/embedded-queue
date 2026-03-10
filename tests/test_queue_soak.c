#include <assert.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

QUEUE_DEFINE(soak_queue, int, 7)

int
main(void)
{
        soak_queue_t q;
        soak_queue_init(&q);

        const int iterations = 1000000;
        int counter = 0;

        printf("Starting soak test (%d iterations)...\n", iterations);

        for (int i = 0; i < iterations; i++) {
                int val = i;
                if (soak_queue_enqueue(&q, &val) == QUEUE_STATUS_OK) {
                        int out;
                        if (soak_queue_dequeue(&q, &out) == QUEUE_STATUS_OK) {
                                assert(out == counter);
                                counter++;
                        }
                }
        }

        printf("Soak test passed. Elements processed: %d\n", counter);
        return 0;
}
