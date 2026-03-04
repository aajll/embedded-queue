/*
 * test_queue_count_mt.c
 *
 * Regression test: name##_count() must never return a value > capacity.
 *
 * This is an SPSC scenario (fail-on-full mode). count() is not a stable
 * snapshot under concurrency, but it should always remain within [0, capacity]
 * when head and tail are read exactly once.
 */

#define QUEUE_OVERWRITE_ON_FULL 0

#include "queue.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define MT_COUNT 200000

QUEUE_DEFINE(mt_queue, int, 1024)

static atomic_bool producer_done = false;
static atomic_bool stop_monitor = false;

static void *
producer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        for (int i = 0; i < MT_COUNT; i++) {
                while (mt_queue_enqueue(q, &i) == QUEUE_STATUS_FULL) {
                        /* spin */
                }
        }
        atomic_store(&producer_done, true);
        return NULL;
}

static void *
consumer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        int expected = 0;
        while (expected < MT_COUNT) {
                int val;
                if (mt_queue_dequeue(q, &val) == QUEUE_STATUS_OK) {
                        assert(val == expected);
                        expected++;
                }
        }
        atomic_store(&stop_monitor, true);
        return NULL;
}

static void *
monitor(void *arg)
{
        const mt_queue_t *q = (const mt_queue_t *)arg;
        const size_t cap = mt_queue_capacity();
        while (!atomic_load(&stop_monitor)) {
                const size_t c = mt_queue_count(q);
                assert(c <= cap);
        }
        return NULL;
}

int
main(void)
{
        mt_queue_t q;
        mt_queue_init(&q);

        pthread_t prod_thread;
        pthread_t cons_thread;
        pthread_t mon_thread;

        pthread_create(&prod_thread, NULL, producer, &q);
        pthread_create(&cons_thread, NULL, consumer, &q);
        pthread_create(&mon_thread, NULL, monitor, &q);

        pthread_join(prod_thread, NULL);
        pthread_join(cons_thread, NULL);
        pthread_join(mon_thread, NULL);

        assert(atomic_load(&producer_done));
        assert(mt_queue_is_empty(&q));

        printf("Multi-threaded count() bounds test passed.\n");
        return 0;
}
