/**
 * SPDX-License-Identifier: MIT
 *
 * @file: queue.c
 *
 * @brief
 *    Implementation stub for the queue library.
 *
 * The queue is header-only (all functions are static inline in queue.h).
 * This translation unit exists so Meson can build a `libqueue` artifact.
 */

/* No queue is instantiated here, so no critical-section hooks are needed. */
#define QUEUE_SINGLE_CONTEXT 1
#include "queue_conf.h"

#include "queue.h"
