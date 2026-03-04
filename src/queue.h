/*
 * queue.h
 *
 * MISRA-oriented, single-producer/single-consumer (SPSC) typed ring-buffer
 * queue.
 *
 * - No dynamic allocation: storage is embedded in the queue instance.
 * - Designed for embedded MCUs.
 * - Default behavior on full: overwrite oldest.
 *
 * Features:
 * - Compile-time capacity validation prevents integer overflow
 * - Consistent count() calculations using snapshots
 * - Proper memory barriers using C11 atomics for weakly-ordered architectures
 *
 * Concurrency notes:
 * - This is a Single-Producer Single-Consumer (SPSC) queue.
 * - In "Fail-on-full" mode (QUEUE_OVERWRITE_ON_FULL=0):
 *   - Lock-free for SPSC with proper memory ordering.
 *   - Uses atomic_thread_fence() for C11 platforms, compiler barrier otherwise.
 * - In "Overwrite-on-full" mode (QUEUE_OVERWRITE_ON_FULL=1):
 *   - NOT lock-free! The producer modifies 'tail', which is also modified by
 * the consumer.
 *   - Critical sections (QUEUE_ENTER_CRITICAL / QUEUE_EXIT_CRITICAL) ARE
 * REQUIRED if the producer and consumer can preempt each other (e.g., ISR and
 * Main).
 *
 * Safety considerations (IEC-61508 / MISRA):
 * - Capacity must be <= QUEUE_MAX_CAPACITY and < SIZE_MAX (ring_size = cap + 1)
 * - Override QUEUE_MAX_CAPACITY to set a project-specific safe limit
 * - Memory barriers use C11 atomics for portability
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "queue_version.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SIZE_MAX must be defined before using it in assertions */
#if !defined(SIZE_MAX)
#error "SIZE_MAX not defined - requires <stdint.h> or <limits.h>"
#endif

#if defined(__GNUC__) || defined(__clang__)
#define QUEUE__UNUSED __attribute__((unused))
#else
#define QUEUE__UNUSED
#endif

#ifndef QUEUE_OVERWRITE_ON_FULL
#define QUEUE_OVERWRITE_ON_FULL 1
#endif

/* Maximum valid capacity to prevent overflow in ring_size calculation */
#ifndef QUEUE_MAX_CAPACITY
#define QUEUE_MAX_CAPACITY (SIZE_MAX - 1U)
#endif

/*
 * Memory ordering fences.
 *
 * Users may override by defining QUEUE_FENCE_ACQUIRE/RELEASE/SEQ_CST.
 */
#ifndef QUEUE_FENCE_ACQUIRE
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_FENCE_ACQUIRE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)               \
    && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define QUEUE_FENCE_ACQUIRE() atomic_thread_fence(memory_order_acquire)
#else
#define QUEUE_FENCE_ACQUIRE()                                                  \
        do {                                                                   \
        } while (0)
#endif
#endif

#ifndef QUEUE_FENCE_RELEASE
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_FENCE_RELEASE() __atomic_thread_fence(__ATOMIC_RELEASE)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)               \
    && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define QUEUE_FENCE_RELEASE() atomic_thread_fence(memory_order_release)
#else
#define QUEUE_FENCE_RELEASE()                                                  \
        do {                                                                   \
        } while (0)
#endif
#endif

#ifndef QUEUE_FENCE_SEQ_CST
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_FENCE_SEQ_CST() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)               \
    && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define QUEUE_FENCE_SEQ_CST() atomic_thread_fence(memory_order_seq_cst)
#else
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_FENCE_SEQ_CST() __asm__ volatile("" : : : "memory")
#else
#define QUEUE_FENCE_SEQ_CST()                                                  \
        do {                                                                   \
        } while (0)
#endif
#endif
#endif

/* Backwards-compatible barrier name. */
#ifndef QUEUE_BARRIER
#define QUEUE_BARRIER() QUEUE_FENCE_SEQ_CST()
#endif

#ifndef QUEUE_ENTER_CRITICAL
#define QUEUE_ENTER_CRITICAL()                                                 \
        do {                                                                   \
        } while (0)
#endif

#ifndef QUEUE_EXIT_CRITICAL
#define QUEUE_EXIT_CRITICAL()                                                  \
        do {                                                                   \
        } while (0)
#endif

typedef enum {
        QUEUE_STATUS_OK = 0,   /* operation successful */
        QUEUE_STATUS_EMPTY,    /* dequeue attempted on empty queue */
        QUEUE_STATUS_FULL,     /* enqueue attempted on full queue (non‑overwrite
                                  mode) */
        QUEUE_STATUS_BAD_ARG,  /* NULL pointer passed to API */
        QUEUE_STATUS_OVERWROTE /* item enqueued by overwriting oldest element */
} queue_status_t;

static inline size_t
queue__next_index(size_t index, size_t ring_size)
{
        index += 1U;
        if (index >= ring_size) {
                index = 0;
        }
        return index;
}

#if QUEUE_OVERWRITE_ON_FULL
#define QUEUE__HANDLE_FULL(q, ring_size, status_var)                           \
        do {                                                                   \
                (q)->tail = queue__next_index((q)->tail, (ring_size));         \
                (status_var) = QUEUE_STATUS_OVERWROTE;                         \
        } while (0)
#else
#define QUEUE__HANDLE_FULL(q, ring_size, status_var)                           \
        do {                                                                   \
                (void)(q);                                                     \
                (void)(ring_size);                                             \
                (status_var) = QUEUE_STATUS_FULL;                              \
        } while (0)
#endif

/*
 * QUEUE_DEFINE(name, type, capacity)
 *
 * Defines:
 * - type: `name##_t`
 * - functions: `name##_init`, `name##_enqueue`, `name##_dequeue`, ...
 *
 * `capacity` is the usable element capacity.
 *
 * Compile-time assertions:
 * - capacity must be <= QUEUE_MAX_CAPACITY
 * - ring_size = capacity + 1U must not overflow size_t
 */
#define QUEUE_DEFINE(name, type, capacity)                                     \
        _Static_assert(((size_t)(capacity)) <= ((size_t)QUEUE_MAX_CAPACITY),   \
                       "QUEUE_CAPACITY_EXCEEDS_MAXIMUM");                      \
        _Static_assert(((size_t)(capacity)) < SIZE_MAX,                        \
                       "QUEUE_CAPACITY_OVERFLOWS_RING");                       \
                                                                               \
        typedef struct {                                                       \
                type buffer[((size_t)(capacity)) + 1U];                        \
                volatile size_t head;                                          \
                volatile size_t tail;                                          \
        } name##_t;                                                            \
                                                                               \
        static inline QUEUE__UNUSED void name##_init(name##_t *q)              \
        {                                                                      \
                if (!q) {                                                      \
                        return;                                                \
                }                                                              \
                q->head = 0U;                                                  \
                q->tail = 0U;                                                  \
        }                                                                      \
        static inline QUEUE__UNUSED void name##_clear(name##_t *q)             \
        {                                                                      \
                if (!q) {                                                      \
                        return;                                                \
                }                                                              \
                q->head = 0U;                                                  \
                q->tail = 0U;                                                  \
        }                                                                      \
        static inline QUEUE__UNUSED bool name##_is_empty(const name##_t *q)    \
        {                                                                      \
                if (!q) {                                                      \
                        return true;                                           \
                }                                                              \
                const size_t head = q->head;                                   \
                const size_t tail = q->tail;                                   \
                return head == tail;                                           \
        }                                                                      \
        static inline QUEUE__UNUSED bool name##_is_full(const name##_t *q)     \
        {                                                                      \
                if (!q) {                                                      \
                        return false;                                          \
                }                                                              \
                const size_t head = q->head;                                   \
                const size_t tail = q->tail;                                   \
                const size_t ring_size = ((size_t)(capacity)) + 1U;            \
                return queue__next_index(head, ring_size) == tail;             \
        }                                                                      \
        static inline QUEUE__UNUSED size_t name##_capacity(void)               \
        {                                                                      \
                return ((size_t)(capacity));                                   \
        }                                                                      \
        static inline QUEUE__UNUSED size_t name##_count(const name##_t *q)     \
        {                                                                      \
                if (!q) {                                                      \
                        return 0U;                                             \
                }                                                              \
                /* Read head and tail once to get a consistent snapshot */     \
                const size_t head = q->head;                                   \
                const size_t tail = q->tail;                                   \
                const size_t ring_size = ((size_t)(capacity)) + 1U;            \
                if (head >= tail) {                                            \
                        return head - tail;                                    \
                }                                                              \
                return ring_size - (tail - head);                              \
        }                                                                      \
        static inline QUEUE__UNUSED queue_status_t name##_enqueue(             \
            name##_t *q, const type *item)                                     \
        {                                                                      \
                if (!q || !item) {                                             \
                        return QUEUE_STATUS_BAD_ARG;                           \
                }                                                              \
                const size_t ring_size = ((size_t)(capacity)) + 1U;            \
                queue_status_t status = QUEUE_STATUS_OK;                       \
                QUEUE_ENTER_CRITICAL();                                        \
                const size_t head = q->head;                                   \
                const size_t tail = q->tail;                                   \
                const size_t next_head = queue__next_index(head, ring_size);   \
                if (next_head == tail) {                                       \
                        QUEUE__HANDLE_FULL(q, ring_size, status);              \
                }                                                              \
                if (status != QUEUE_STATUS_FULL) {                             \
                        q->buffer[head] = *item;                               \
                        QUEUE_FENCE_RELEASE();                                 \
                        q->head = next_head;                                   \
                }                                                              \
                QUEUE_EXIT_CRITICAL();                                         \
                return status;                                                 \
        }                                                                      \
        static inline QUEUE__UNUSED queue_status_t name##_dequeue(name##_t *q, \
                                                                  type *out)   \
        {                                                                      \
                if (!q || !out) {                                              \
                        return QUEUE_STATUS_BAD_ARG;                           \
                }                                                              \
                const size_t ring_size = ((size_t)(capacity)) + 1U;            \
                QUEUE_ENTER_CRITICAL();                                        \
                const size_t head = q->head;                                   \
                const size_t tail = q->tail;                                   \
                if (head == tail) {                                            \
                        QUEUE_EXIT_CRITICAL();                                 \
                        return QUEUE_STATUS_EMPTY;                             \
                }                                                              \
                QUEUE_FENCE_ACQUIRE();                                         \
                *out = q->buffer[tail];                                        \
                QUEUE_FENCE_RELEASE();                                         \
                q->tail = queue__next_index(tail, ring_size);                  \
                QUEUE_EXIT_CRITICAL();                                         \
                return QUEUE_STATUS_OK;                                        \
        }

/* Generic clear macro for queues generated by QUEUE_DEFINE */
#define QUEUE_CLEAR(name, q)                                                   \
        do {                                                                   \
                if ((void *)(q) != NULL) {                                     \
                        (q)->head = 0U;                                        \
                        (q)->tail = 0U;                                        \
                }                                                              \
        } while (0)

#endif /* QUEUE_H */
