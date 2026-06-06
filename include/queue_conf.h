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

/* Memory ordering fences - override before including queue_impl.h */
#ifndef QUEUE_FENCE_ACQUIRE
#if defined(__GNUC__) || defined(__clang__)
#define QUEUE_FENCE_ACQUIRE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)               \
    && !defined(__STDC_NO_ATOMICS__)
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