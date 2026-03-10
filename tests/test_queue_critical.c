/* test_queue_critical.c */
#define QUEUE_OVERWRITE_ON_FULL 1
/* Redefine the critical-section macros to count usage */
#define QUEUE_ENTER_CRITICAL()  ++critical_enter_cnt
#define QUEUE_EXIT_CRITICAL()   ++critical_exit_cnt
#include <assert.h>
#include <stdio.h>
#include "queue_conf.h"
#include "queue.h"

static unsigned critical_enter_cnt = 0;
static unsigned critical_exit_cnt = 0;

typedef int elem_t;
QUEUE_DEFINE(crit_q, elem_t, 3)

int
main(void)
{
        crit_q_t q;
        crit_q_init(&q);

        elem_t v = 42;
        assert(crit_q_enqueue(&q, &v) == QUEUE_STATUS_OK);
        assert(crit_q_dequeue(&q, &v) == QUEUE_STATUS_OK);

        /* Should have entered/exited exactly once for each operation */
        assert(critical_enter_cnt == 2);
        assert(critical_exit_cnt == 2);

        printf("Critical‑section hook test passed (enter=%u, exit=%u).\n",
               critical_enter_cnt, critical_exit_cnt);
        return 0;
}
