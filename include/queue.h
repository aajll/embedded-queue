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
 * - Atomic head and tail indices with acquire/release ordering
 *
 * Concurrency notes:
 * - This is a Single-Producer Single-Consumer (SPSC) queue.
 * - In "Fail-on-full" mode (QUEUE_OVERWRITE_ON_FULL=0):
 *   - Lock-free for SPSC with proper memory ordering.
 *   - The indices use the atomic backend selected in queue_conf.h.
 * - In "Overwrite-on-full" mode (QUEUE_OVERWRITE_ON_FULL=1):
 *   - NOT lock-free! The producer modifies 'tail', which is also modified by
 * the consumer.
 *   - Critical sections (QUEUE_ENTER_CRITICAL / QUEUE_EXIT_CRITICAL) ARE
 * REQUIRED if the producer and consumer can preempt each other (e.g., ISR and
 * Main).
 *
 * Safety considerations:
 * - Capacity must be > 0, <= QUEUE_MAX_CAPACITY and < SIZE_MAX
 *   (ring_size = cap + 1)
 * - Override QUEUE_MAX_CAPACITY to set a project-specific safe limit
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @name Version
 * @brief Release of this header, for diagnostics and @c #if checks.
 *
 * @details
 *    @c QUEUE_VERSION_NUMBER is @c MAJOR*10000 @c + @c MINOR*100
 *    @c + @c PATCH with type @c long, so it cannot overflow where @c int is
 *    16 bits. The Meson build fails to configure when these values differ
 *    from the project version.
 * @{
 */
/* cppcheck-suppress-begin misra-c2012-2.5 ; @deviation Public version macros
 * for consumer diagnostics and #if checks; the library does not use them. */
#define QUEUE_VERSION_MAJOR 1
#define QUEUE_VERSION_MINOR 3
#define QUEUE_VERSION_PATCH 0
#define QUEUE_VERSION       "1.3.0"
#define QUEUE_VERSION_NUMBER                                                   \
        ((QUEUE_VERSION_MAJOR * 10000L) + (QUEUE_VERSION_MINOR * 100L)         \
         + QUEUE_VERSION_PATCH)
/* cppcheck-suppress-end misra-c2012-2.5 ; @deviation As above. */
/** @} */

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

/*
 * In overwrite mode a full enqueue moves `tail`, which the consumer also
 * writes, so producer and consumer must exclude each other through the
 * critical-section hooks. Define QUEUE_SINGLE_CONTEXT to 1 when one execution
 * context performs every queue operation and no hooks are needed.
 */
#if (QUEUE_OVERWRITE_ON_FULL != 0) && defined(QUEUE__CRITICAL_DEFAULTED)       \
    && (QUEUE_SINGLE_CONTEXT == 0)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC warning                                                            \
    "QUEUE_OVERWRITE_ON_FULL needs QUEUE_ENTER_CRITICAL/QUEUE_EXIT_CRITICAL, or QUEUE_SINGLE_CONTEXT=1"
#elif defined(__TI_COMPILER_VERSION__)
#warning                                                                       \
    "QUEUE_OVERWRITE_ON_FULL needs QUEUE_ENTER_CRITICAL/QUEUE_EXIT_CRITICAL, or QUEUE_SINGLE_CONTEXT=1"
#endif
#endif

/*
 * QUEUE__UNUSED marks every static inline function in this header and every
 * wrapper QUEUE_DEFINE generates. A consumer usually calls only some of them,
 * and the marker keeps the rest from raising unused-function diagnostics:
 *
 *   - Clang warns about unused static inline functions defined in the file
 *     being compiled, which is where QUEUE_DEFINE expands.
 *   - TI compilers warn (#179-D) about any unused static function, inline or
 *     not. They do not define __GNUC__ but accept the GNU attribute.
 *   - GCC does not warn; the attribute is harmless there.
 *
 * Other compilers get an empty marker, which can cost warnings but never a
 * failed build. To supply a marker for another toolchain, define
 * QUEUE__UNUSED before including this header.
 */
#ifndef QUEUE__UNUSED
#if defined(__GNUC__) || defined(__clang__) || defined(__TI_COMPILER_VERSION__)
#define QUEUE__UNUSED __attribute__((unused))
#else
#define QUEUE__UNUSED
#endif
#endif

/*
 * Index access for the selected backend (see queue_conf.h). The producer
 * publishes a slot with a release store of `head` and the consumer frees one
 * with a release store of `tail`; each side reads the other's index with an
 * acquire load, so slot contents never race with index updates.
 */
#if defined(QUEUE_USE_GNU_ATOMICS)
#define QUEUE__ATOMIC
#define QUEUE__LOAD_RELAXED(ptr) __atomic_load_n((ptr), __ATOMIC_RELAXED)
#define QUEUE__LOAD_ACQUIRE(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
#define QUEUE__STORE_RELAXED(ptr, val)                                         \
        __atomic_store_n((ptr), (val), __ATOMIC_RELAXED)
#define QUEUE__STORE_RELEASE(ptr, val)                                         \
        __atomic_store_n((ptr), (val), __ATOMIC_RELEASE)
#define QUEUE__FENCE_AFTER_ACQUIRE()                                           \
        do {                                                                   \
        } while (0)
#define QUEUE__FENCE_BEFORE_RELEASE()                                          \
        do {                                                                   \
        } while (0)
#elif defined(QUEUE_USE_C11_ATOMICS)
#define QUEUE__ATOMIC _Atomic
#define QUEUE__LOAD_RELAXED(ptr)                                               \
        atomic_load_explicit((ptr), memory_order_relaxed)
#define QUEUE__LOAD_ACQUIRE(ptr)                                               \
        atomic_load_explicit((ptr), memory_order_acquire)
#define QUEUE__STORE_RELAXED(ptr, val)                                         \
        atomic_store_explicit((ptr), (val), memory_order_relaxed)
#define QUEUE__STORE_RELEASE(ptr, val)                                         \
        atomic_store_explicit((ptr), (val), memory_order_release)
#define QUEUE__FENCE_AFTER_ACQUIRE()                                           \
        do {                                                                   \
        } while (0)
#define QUEUE__FENCE_BEFORE_RELEASE()                                          \
        do {                                                                   \
        } while (0)
#else /* QUEUE_USE_NO_ATOMICS */
#define QUEUE__ATOMIC                  volatile
#define QUEUE__LOAD_RELAXED(ptr)       (*(ptr))
#define QUEUE__LOAD_ACQUIRE(ptr)       (*(ptr))
#define QUEUE__STORE_RELAXED(ptr, val) ((void)(*(ptr) = (val)))
#define QUEUE__STORE_RELEASE(ptr, val) ((void)(*(ptr) = (val)))
#define QUEUE__FENCE_AFTER_ACQUIRE()   QUEUE_FENCE_ACQUIRE()
#define QUEUE__FENCE_BEFORE_RELEASE()  QUEUE_FENCE_RELEASE()
#endif

/*
 * Item copy for the selected backend. The volatile backend copies through
 * volatile lvalues, because C orders volatile accesses against each other
 * but not against ordinary ones: the copy then stays between the index
 * accesses on every compiler, whatever the fences expand to.
 */
#if defined(QUEUE_USE_NO_ATOMICS)
static inline QUEUE__UNUSED void
queue__copy_item(void *dst, const void *src, size_t size)
{
        volatile unsigned char *const to = (volatile unsigned char *)dst;
        const volatile unsigned char *const from =
            (const volatile unsigned char *)src;
        for (size_t i = 0U; i < size; i++) {
                to[i] = from[i];
        }
}
#define QUEUE__COPY_ITEM(dst, src, size) queue__copy_item((dst), (src), (size))
#else
#define QUEUE__COPY_ITEM(dst, src, size) ((void)memcpy((dst), (src), (size)))
#endif

#if defined(QUEUE_USE_GNU_ATOMICS)
/* __atomic_always_lock_free is a compile-time constant but not a standard
 * integer constant expression, so -Wpedantic objects to it inside
 * _Static_assert; suppress only that diagnostic. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
_Static_assert(__atomic_always_lock_free(sizeof(size_t), 0),
               "queue: size_t indices are not lock-free on this target");
#pragma GCC diagnostic pop
#elif defined(QUEUE_USE_C11_ATOMICS)
/* atomic_is_lock_free is a runtime query, so match size_t to the
 * ATOMIC_*_LOCK_FREE macro of the same width and require "always" (2). */
_Static_assert(
    (sizeof(size_t) == sizeof(int) && ATOMIC_INT_LOCK_FREE == 2)
        || (sizeof(size_t) == sizeof(long) && ATOMIC_LONG_LOCK_FREE == 2)
        || (sizeof(size_t) == sizeof(long long) && ATOMIC_LLONG_LOCK_FREE == 2),
    "queue: size_t indices are not always lock-free on this target");
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
        QUEUE__ATOMIC size_t head; /* next slot the producer writes */
        QUEUE__ATOMIC size_t tail; /* next slot the consumer reads */
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
        QUEUE__STORE_RELAXED(&core->head, (size_t)0U);
        QUEUE__STORE_RELAXED(&core->tail, (size_t)0U);
}

static inline QUEUE__UNUSED bool
queue__is_empty(const void *queue)
{
        if (queue == NULL) {
                return true;
        }
        const queue_core_t *core = (const queue_core_t *)queue;
        const size_t head = QUEUE__LOAD_ACQUIRE(&core->head);
        const size_t tail = QUEUE__LOAD_ACQUIRE(&core->tail);
        QUEUE__FENCE_AFTER_ACQUIRE();
        return head == tail;
}

static inline QUEUE__UNUSED bool
queue__is_full(const void *queue, size_t ring_size)
{
        if (queue == NULL) {
                return false;
        }
        const queue_core_t *core = (const queue_core_t *)queue;
        const size_t head = QUEUE__LOAD_ACQUIRE(&core->head);
        const size_t tail = QUEUE__LOAD_ACQUIRE(&core->tail);
        QUEUE__FENCE_AFTER_ACQUIRE();
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
        const size_t head = QUEUE__LOAD_ACQUIRE(&core->head);
        const size_t tail = QUEUE__LOAD_ACQUIRE(&core->tail);
        QUEUE__FENCE_AFTER_ACQUIRE();
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
        const size_t head = QUEUE__LOAD_RELAXED(&core->head);
        const size_t tail = QUEUE__LOAD_ACQUIRE(&core->tail);
        QUEUE__FENCE_AFTER_ACQUIRE();
        const size_t next_head = queue__next_index(head, ring_size);
#if QUEUE_OVERWRITE_ON_FULL
        if (next_head == tail) {
                /* Drop the oldest item; the caller excludes the consumer. */
                QUEUE__STORE_RELAXED(&core->tail,
                                     queue__next_index(tail, ring_size));
                status = QUEUE_STATUS_OVERWROTE;
        }
        QUEUE__COPY_ITEM(&buffer[head * item_size], item, item_size);
        QUEUE__FENCE_BEFORE_RELEASE();
        QUEUE__STORE_RELEASE(&core->head, next_head);
#else
        if (next_head == tail) {
                status = QUEUE_STATUS_FULL;
        } else {
                QUEUE__COPY_ITEM(&buffer[head * item_size], item, item_size);
                QUEUE__FENCE_BEFORE_RELEASE();
                QUEUE__STORE_RELEASE(&core->head, next_head);
        }
#endif
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
        const size_t tail = QUEUE__LOAD_RELAXED(&core->tail);
        const size_t head = QUEUE__LOAD_ACQUIRE(&core->head);
        QUEUE__FENCE_AFTER_ACQUIRE();
        if (head == tail) {
                return QUEUE_STATUS_EMPTY;
        }
        QUEUE__COPY_ITEM(out, &buffer[tail * item_size], item_size);
        QUEUE__FENCE_BEFORE_RELEASE();
        QUEUE__STORE_RELEASE(&core->tail, queue__next_index(tail, ring_size));
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
 * - capacity must be > 0
 * - capacity must be <= QUEUE_MAX_CAPACITY
 * - ring_size = capacity + 1U must not overflow size_t
 *
 * The generated functions are thin typed wrappers over the core above. The
 * critical-section hooks expand here, at the point of instantiation, so they
 * may refer to anything declared before QUEUE_DEFINE.
 */
#define QUEUE_DEFINE(name, type, capacity)                                     \
        _Static_assert(((size_t)(capacity)) > 0U, "QUEUE_CAPACITY_ZERO");      \
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
