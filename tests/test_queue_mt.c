#define QUEUE_OVERWRITE_ON_FULL 0
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

#define MT_COUNT 1000000
QUEUE_DEFINE(mt_queue, int, 1024)

void *
producer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        for (int i = 0; i < MT_COUNT; i++) {
                while (mt_queue_enqueue(q, &i) == QUEUE_STATUS_FULL) {
                        // spin
                }
        }
        return NULL;
}

void *
consumer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        int expected = 0;
        while (expected < MT_COUNT) {
                int val;
                if (mt_queue_dequeue(q, &val) == QUEUE_STATUS_OK) {
                        if (val != expected) {
                                fprintf(stderr, "Expected %d, got %d\n",
                                        expected, val);
                                assert(val == expected);
                        }
                        expected++;
                }
        }
        return NULL;
}

int
main(void)
{
        mt_queue_t q;
        mt_queue_init(&q);

        pthread_t prod_thread, cons_thread;
        pthread_create(&prod_thread, NULL, producer, &q);
        pthread_create(&cons_thread, NULL, consumer, &q);

        pthread_join(prod_thread, NULL);
        pthread_join(cons_thread, NULL);

        printf("Multi-threaded SPSC test passed.\n");
        return 0;
}
