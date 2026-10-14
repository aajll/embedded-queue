/**
 * SPDX-License-Identifier: MIT
 *
 * @file: queue.h
 *
 * @brief
 *    MISRA-oriented, single-producer/single-consumer (SPSC) typed ring-buffer
 *    queue.
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
 * Safety considerations:
 * - Capacity must be <= QUEUE_MAX_CAPACITY and < SIZE_MAX (ring_size = cap + 1)
 * - Override QUEUE_MAX_CAPACITY to set a project-specific safe limit
 * - Memory barriers use C11 atomics for portability
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* SIZE_MAX must be defined before using it in assertions */
#if !defined(SIZE_MAX)
#error "SIZE_MAX not defined - requires <stdint.h> or <limits.h>"
#endif

/* Verify configuration macros are defined */
#ifndef QUEUE_OVERWRITE_ON_FULL
#error                                                                         \
    "QUEUE_OVERWRITE_ON_FULL not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_MAX_CAPACITY
#error                                                                         \
    "QUEUE_MAX_CAPACITY not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_FENCE_ACQUIRE
#error                                                                         \
    "QUEUE_FENCE_ACQUIRE not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_FENCE_RELEASE
#error                                                                         \
    "QUEUE_FENCE_RELEASE not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_FENCE_SEQ_CST
#error                                                                         \
    "QUEUE_FENCE_SEQ_CST not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_ENTER_CRITICAL
#error                                                                         \
    "QUEUE_ENTER_CRITICAL not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_EXIT_CRITICAL
#error                                                                         \
    "QUEUE_EXIT_CRITICAL not defined - include <queue_conf.h> before <queue.h>"
#endif

#ifndef QUEUE_BARRIER
#error "QUEUE_BARRIER not defined - include <queue_conf.h> before <queue.h>"
#endif

#if defined(__GNUC__) || defined(__clang__)
#define QUEUE__UNUSED __attribute__((unused))
#else
#define QUEUE__UNUSED
#endif

typedef enum {
        QUEUE_STATUS_OK = 0,   /* operation successful */
        QUEUE_STATUS_EMPTY,    /* dequeue attempted on empty queue */
        QUEUE_STATUS_FULL,     /* enqueue attempted on full queue (non‑overwrite
                                  mode) */
        QUEUE_STATUS_BAD_ARG,  /* NULL pointer passed to API */
        QUEUE_STATUS_OVERWROTE /* item enqueued by overwriting oldest element */
} queue_status_t;

/*
 * Index pair shared by every queue instance. QUEUE_DEFINE places it as the
 * first member of the generated type, so a pointer to a queue is also a
 * pointer to its core and the type-erased functions below can operate on any
 * instantiation.
 */
typedef struct {
        volatile size_t head; /* next slot the producer writes */
        volatile size_t tail; /* next slot the consumer reads */
} queue_core_t;

static inline QUEUE__UNUSED size_t
queue__next_index(size_t index, size_t ring_size)
{
        index += 1U;
        if (index >= ring_size) {
                index = 0;
        }
        return index;
}

#if QUEUE_OVERWRITE_ON_FULL
#define QUEUE__HANDLE_FULL(core, ring_size, status_var)                        \
        do {                                                                   \
                (core)->tail = queue__next_index((core)->tail, (ring_size));   \
                (status_var) = QUEUE_STATUS_OVERWROTE;                         \
        } while (0)
#else
#define QUEUE__HANDLE_FULL(core, ring_size, status_var)                        \
        do {                                                                   \
                (void)(core);                                                  \
                (void)(ring_size);                                             \
                (status_var) = QUEUE_STATUS_FULL;                              \
        } while (0)
#endif

/*
 * Type-erased core. `queue` points to a QUEUE_DEFINE instance, `buffer_offset`
 * is the offset of its element array, `item_size` the element size and
 * `ring_size` the array length (capacity + 1). The typed wrappers generated by
 * QUEUE_DEFINE supply these and hold the critical section around each call.
 */
static inline QUEUE__UNUSED void
queue__init(void *queue)
{
        if (queue == NULL) {
                return;
        }
        queue_core_t *core = (queue_core_t *)queue;
        core->head = 0U;
        core->tail = 0U;
}

static inline QUEUE__UNUSED bool
queue__is_empty(const void *queue)
{
        if (queue == NULL) {
                return true;
        }
        const queue_core_t *core = (const queue_core_t *)queue;
        const size_t head = core->head;
        const size_t tail = core->tail;
        return head == tail;
}

static inline QUEUE__UNUSED bool
queue__is_full(const void *queue, size_t ring_size)
{
        if (queue == NULL) {
                return false;
        }
        const queue_core_t *core = (const queue_core_t *)queue;
        const size_t head = core->head;
        const size_t tail = core->tail;
        return queue__next_index(head, ring_size) == tail;
}

static inline QUEUE__UNUSED size_t
queue__count(const void *queue, size_t ring_size)
{
        if (queue == NULL) {
                return 0U;
        }
        const queue_core_t *core = (const queue_core_t *)queue;
        /* Read head and tail once to get a consistent snapshot */
        const size_t head = core->head;
        const size_t tail = core->tail;
        if (head >= tail) {
                return head - tail;
        }
        return ring_size - (tail - head);
}

static inline QUEUE__UNUSED queue_status_t
queue__enqueue(void *queue, size_t buffer_offset, size_t item_size,
               size_t ring_size, const void *item)
{
        if ((queue == NULL) || (item == NULL)) {
                return QUEUE_STATUS_BAD_ARG;
        }
        queue_core_t *core = (queue_core_t *)queue;
        unsigned char *buffer = (unsigned char *)queue + buffer_offset;
        queue_status_t status = QUEUE_STATUS_OK;
        const size_t head = core->head;
        const size_t tail = core->tail;
        const size_t next_head = queue__next_index(head, ring_size);
        if (next_head == tail) {
                QUEUE__HANDLE_FULL(core, ring_size, status);
        }
        if (status != QUEUE_STATUS_FULL) {
                (void)memcpy(&buffer[head * item_size], item, item_size);
                QUEUE_FENCE_RELEASE();
                core->head = next_head;
        }
        return status;
}

static inline QUEUE__UNUSED queue_status_t
queue__dequeue(void *queue, size_t buffer_offset, size_t item_size,
               size_t ring_size, void *out)
{
        if ((queue == NULL) || (out == NULL)) {
                return QUEUE_STATUS_BAD_ARG;
        }
        queue_core_t *core = (queue_core_t *)queue;
        const unsigned char *buffer =
            (const unsigned char *)queue + buffer_offset;
        const size_t head = core->head;
        const size_t tail = core->tail;
        if (head == tail) {
                return QUEUE_STATUS_EMPTY;
        }
        QUEUE_FENCE_ACQUIRE();
        (void)memcpy(out, &buffer[tail * item_size], item_size);
        QUEUE_FENCE_RELEASE();
        core->tail = queue__next_index(tail, ring_size);
        return QUEUE_STATUS_OK;
}

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
 *
 * The generated functions are thin typed wrappers over the core above. The
 * critical-section hooks expand here, at the point of instantiation, so they
 * may refer to anything declared before QUEUE_DEFINE.
 */
#define QUEUE_DEFINE(name, type, capacity)                                     \
        _Static_assert(((size_t)(capacity)) <= ((size_t)QUEUE_MAX_CAPACITY),   \
                       "QUEUE_CAPACITY_EXCEEDS_MAXIMUM");                      \
        _Static_assert(((size_t)(capacity)) < SIZE_MAX,                        \
                       "QUEUE_CAPACITY_OVERFLOWS_RING");                       \
                                                                               \
        typedef struct {                                                       \
                queue_core_t core;                                             \
                type buffer[((size_t)(capacity)) + 1U];                        \
        } name##_t;                                                            \
                                                                               \
        static inline QUEUE__UNUSED void name##_init(name##_t *q)              \
        {                                                                      \
                queue__init(q);                                                \
        }                                                                      \
        static inline QUEUE__UNUSED void name##_clear(name##_t *q)             \
        {                                                                      \
                queue__init(q);                                                \
        }                                                                      \
        static inline QUEUE__UNUSED bool name##_is_empty(const name##_t *q)    \
        {                                                                      \
                return queue__is_empty(q);                                     \
        }                                                                      \
        static inline QUEUE__UNUSED bool name##_is_full(const name##_t *q)     \
        {                                                                      \
                return queue__is_full(q, ((size_t)(capacity)) + 1U);           \
        }                                                                      \
        static inline QUEUE__UNUSED size_t name##_capacity(void)               \
        {                                                                      \
                return ((size_t)(capacity));                                   \
        }                                                                      \
        static inline QUEUE__UNUSED size_t name##_count(const name##_t *q)     \
        {                                                                      \
                return queue__count(q, ((size_t)(capacity)) + 1U);             \
        }                                                                      \
        static inline QUEUE__UNUSED queue_status_t name##_enqueue(             \
            name##_t *q, const type *item)                                     \
        {                                                                      \
                queue_status_t status;                                         \
                QUEUE_ENTER_CRITICAL();                                        \
                status = queue__enqueue(q, offsetof(name##_t, buffer),         \
                                        sizeof(type),                          \
                                        ((size_t)(capacity)) + 1U, item);      \
                QUEUE_EXIT_CRITICAL();                                         \
                return status;                                                 \
        }                                                                      \
        static inline QUEUE__UNUSED queue_status_t name##_dequeue(name##_t *q, \
                                                                  type *out)   \
        {                                                                      \
                queue_status_t status;                                         \
                QUEUE_ENTER_CRITICAL();                                        \
                status = queue__dequeue(q, offsetof(name##_t, buffer),         \
                                        sizeof(type),                          \
                                        ((size_t)(capacity)) + 1U, out);       \
                QUEUE_EXIT_CRITICAL();                                         \
                return status;                                                 \
        }

/* Generic clear macro for queues generated by QUEUE_DEFINE */
#define QUEUE_CLEAR(name, q)                                                   \
        do {                                                                   \
                name##_clear(q);                                               \
        } while (0)

#endif /* QUEUE_H */
