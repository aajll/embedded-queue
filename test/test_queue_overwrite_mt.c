#include <pthread.h>
static pthread_mutex_t mt_lock = PTHREAD_MUTEX_INITIALIZER;
#define QUEUE_ENTER_CRITICAL()  pthread_mutex_lock(&mt_lock)
#define QUEUE_EXIT_CRITICAL()   pthread_mutex_unlock(&mt_lock)
#define QUEUE_OVERWRITE_ON_FULL 1
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

#define MT_COUNT 1000000
QUEUE_DEFINE(mt_queue, int, 10)

static atomic_bool producer_done = false;

void *
producer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        for (int i = 0; i < MT_COUNT; i++) {
                (void)mt_queue_enqueue(q, &i);
        }
        atomic_store(&producer_done, true);
        return NULL;
}

void *
consumer(void *arg)
{
        mt_queue_t *q = (mt_queue_t *)arg;
        int last_val = -1;
        while (!atomic_load(&producer_done) || !mt_queue_is_empty(q)) {
                int val;
                if (mt_queue_dequeue(q, &val) == QUEUE_STATUS_OK) {
                        assert(val > last_val);
                        last_val = val;
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

        printf("Multi-threaded Overwrite (with locks) test passed.\n");
        return 0;
}
