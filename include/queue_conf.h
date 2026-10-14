/**
 * SPDX-License-Identifier: MIT
 *
 * @file: queue_conf.h
 *
 * @brief
 *    User configuration header for the queue library.
 *    Include this file before queue.h to customize behavior.
 */

#ifndef QUEUE_CONF_H
#define QUEUE_CONF_H

/* Overwrite mode (default 1):
 * - 1: enqueue overwrites oldest item on full
 * - 0: enqueue fails on full
 */
#ifndef QUEUE_OVERWRITE_ON_FULL
#define QUEUE_OVERWRITE_ON_FULL 1
#endif

/* Maximum capacity to prevent integer overflow */
#ifndef QUEUE_MAX_CAPACITY
#define QUEUE_MAX_CAPACITY (SIZE_MAX - 1U)
#endif

/*
 * Atomic backend for the head and tail indices. Define exactly one of
 * QUEUE_USE_GNU_ATOMICS, QUEUE_USE_C11_ATOMICS or QUEUE_USE_NO_ATOMICS to
 * override automatic selection, which tries in order:
 *   GCC/Clang __atomic builtins -> C11 <stdatomic.h> -> volatile.
 *
 * QUEUE_USE_NO_ATOMICS stores the indices in volatile objects and orders
 * them with QUEUE_FENCE_ACQUIRE / QUEUE_FENCE_RELEASE. It is correct only
 * when a naturally aligned size_t load or store is indivisible and the
 * fences provide the ordering the target needs, for example a single-core
 * MCU whose toolchain has no atomics.
 */
#if !defined(QUEUE_USE_GNU_ATOMICS) && !defined(QUEUE_USE_C11_ATOMICS)         \
    && !defined(QUEUE_USE_NO_ATOMICS)
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_USE_GNU_ATOMICS 1
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)               \
    && !defined(__STDC_NO_ATOMICS__)
#define QUEUE_USE_C11_ATOMICS 1
#else
#define QUEUE_USE_NO_ATOMICS 1
#endif
#endif

#if (defined(QUEUE_USE_GNU_ATOMICS) + defined(QUEUE_USE_C11_ATOMICS)           \
     + defined(QUEUE_USE_NO_ATOMICS))                                          \
    != 1
#error "queue: define exactly one atomic backend"
#endif

#if defined(QUEUE_USE_C11_ATOMICS)
#include <stdatomic.h>
#endif

/*
 * Memory ordering fences. The atomic backends order the indices with
 * acquire loads and release stores; the core uses these fences only with
 * QUEUE_USE_NO_ATOMICS. Override to supply a target-specific barrier.
 */
#if defined(QUEUE_USE_GNU_ATOMICS)
#define QUEUE__DEFAULT_FENCE_ACQUIRE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define QUEUE__DEFAULT_FENCE_RELEASE() __atomic_thread_fence(__ATOMIC_RELEASE)
#define QUEUE__DEFAULT_FENCE_SEQ_CST() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#elif defined(QUEUE_USE_C11_ATOMICS)
#define QUEUE__DEFAULT_FENCE_ACQUIRE() atomic_thread_fence(memory_order_acquire)
#define QUEUE__DEFAULT_FENCE_RELEASE() atomic_thread_fence(memory_order_release)
#define QUEUE__DEFAULT_FENCE_SEQ_CST() atomic_thread_fence(memory_order_seq_cst)
#elif defined(__GNUC__) || defined(__clang__)
#define QUEUE__DEFAULT_FENCE_ACQUIRE() __asm__ volatile("" : : : "memory")
#define QUEUE__DEFAULT_FENCE_RELEASE() __asm__ volatile("" : : : "memory")
#define QUEUE__DEFAULT_FENCE_SEQ_CST() __asm__ volatile("" : : : "memory")
#else
#define QUEUE__DEFAULT_FENCE_ACQUIRE()                                         \
        do {                                                                   \
        } while (0)
#define QUEUE__DEFAULT_FENCE_RELEASE()                                         \
        do {                                                                   \
        } while (0)
#define QUEUE__DEFAULT_FENCE_SEQ_CST()                                         \
        do {                                                                   \
        } while (0)
#endif

#ifndef QUEUE_FENCE_ACQUIRE
#define QUEUE_FENCE_ACQUIRE() QUEUE__DEFAULT_FENCE_ACQUIRE()
#endif

#ifndef QUEUE_FENCE_RELEASE
#define QUEUE_FENCE_RELEASE() QUEUE__DEFAULT_FENCE_RELEASE()
#endif

#ifndef QUEUE_FENCE_SEQ_CST
#define QUEUE_FENCE_SEQ_CST() QUEUE__DEFAULT_FENCE_SEQ_CST()
#endif

/* Backwards-compatible barrier name */
#ifndef QUEUE_BARRIER
#define QUEUE_BARRIER() QUEUE_FENCE_SEQ_CST()
#endif

/* Critical section macros - override for thread-safety */
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

#endif /* QUEUE_CONF_H */