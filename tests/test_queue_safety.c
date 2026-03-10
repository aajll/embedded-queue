/* test_queue_safety.c - Tests for the fixed safety issues */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

/* Test 1: Normal capacity should work */
QUEUE_DEFINE(test_normal_q, int, 10)

/* Test 2: Large capacity should work (within limits) */
QUEUE_DEFINE(test_large_q, int, 1000)

/* Test 3: Zero capacity - this should fail at compile time with _Static_assert
 */
/* QUEUE_DEFINE(test_zero_q, int, 0) */

int
main(void)
{
        printf("Testing safety fixes...\n");

        /* Test 1: Normal queue works */
        test_normal_q_t q1;
        test_normal_q_init(&q1);
        assert(test_normal_q_capacity() == 10);
        assert(test_normal_q_is_empty(&q1));

        /* Test enqueue and dequeue */
        int v = 42;
        assert(test_normal_q_enqueue(&q1, &v) == QUEUE_STATUS_OK);
        assert(!test_normal_q_is_empty(&q1));
        assert(test_normal_q_count(&q1) == 1);

        int out;
        assert(test_normal_q_dequeue(&q1, &out) == QUEUE_STATUS_OK);
        assert(out == 42);
        assert(test_normal_q_is_empty(&q1));

        /* Test 2: Large capacity queue */
        test_large_q_t q2;
        test_large_q_init(&q2);
        assert(test_large_q_capacity() == 1000);

        /* Test 3: Verify count() returns correct values */
        for (int i = 0; i < 10; i++) {
                test_normal_q_enqueue(&q1, &i);
        }
        assert(test_normal_q_count(&q1) == 10);

        /* Test wraparound and count consistency */
        int x;
        for (int i = 0; i < 5; i++) {
                test_normal_q_dequeue(&q1, &x);
        }
        assert(test_normal_q_count(&q1) == 5);

        for (int i = 0; i < 5; i++) {
                test_normal_q_enqueue(&q1, &i);
        }
        assert(test_normal_q_count(&q1) == 10);

        /* Test 4: NULL pointer handling */
        assert(test_normal_q_count(NULL) == 0);
        assert(test_normal_q_is_empty(NULL));
        assert(test_normal_q_is_full(NULL) == false);
        assert(test_normal_q_enqueue(NULL, &v) == QUEUE_STATUS_BAD_ARG);
        assert(test_normal_q_dequeue(NULL, &v) == QUEUE_STATUS_BAD_ARG);

        printf("All safety tests passed!\n");
        return 0;
}