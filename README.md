# embedded-queue

[![CI](https://github.com/aajll/embedded-queue/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/embedded-queue/actions/workflows/ci.yml)

A deterministic, type-safe SPSC ring-buffer queue in C11 for embedded and safety-critical systems.

## Features

- **No dynamic memory** — storage is embedded directly in the queue instance; no `malloc` / `free`.
- **Type-safe** — `QUEUE_DEFINE` generates a fully typed queue struct and API at compile time.
- **Compile-time validation** — capacity overflow is caught with `_Static_assert` before any code runs.
- **Configurable full-policy** — choose between overwrite-oldest (default) or fail-on-full modes.
- **Lock-free SPSC** — fail-on-full mode is lock-free for single-producer / single-consumer use when `size_t` loads/stores are atomic on the target.
- **Memory ordering** — uses C11 `atomic_thread_fence` or GCC/Clang `__atomic_thread_fence` to handle weakly-ordered CPUs; falls back to compiler barriers.
- **Critical section hooks** — `QUEUE_ENTER_CRITICAL` / `QUEUE_EXIT_CRITICAL` let you plug in interrupt disable or mutex locking for overwrite mode.
- **MISRA-oriented** — no VLAs, no dynamic allocation, no undefined behaviour; suitable for IEC 61508 environments.

## Installation

### Copy-in (recommended for embedded targets)

Copy two files into your project tree — no build system required:

```
include/queue.h
config/queue_conf.h
```

Place them so they are visible to your compiler, then include in order:

```c
#include "queue_conf.h"   /* must come first */
#include "queue.h"
```

### Meson subproject

Add this repo as a wrap dependency or subproject. The library exposes a
`queue_dep` dependency object that carries the correct include paths:

```meson
queue_dep = dependency('embedded-queue', fallback : ['embedded-queue', 'queue_dep'])
```

## Quick Start

```c
#include <stdint.h>
#include "queue_conf.h"
#include "queue.h"

typedef struct {
        uint32_t id;
        uint8_t  dlc;
        uint8_t  data[8];
} can_msg_t;

/* Define a typed queue of 16 CAN messages — validated at compile time. */
QUEUE_DEFINE(can_msg_queue, can_msg_t, 16)

int main(void)
{
        can_msg_queue_t q = {0};
        queue_status_t  st;
        can_msg_t       tx = {.id = 0x123U, .dlc = 8U, .data = {0}};
        can_msg_t       rx;

        can_msg_queue_init(&q);

        st = can_msg_queue_enqueue(&q, &tx);

        if (can_msg_queue_dequeue(&q, &rx) == QUEUE_STATUS_OK) {
                /* process rx */
        }

        can_msg_queue_clear(&q);
        return 0;
}
```

## Concurrency

This library targets **single-producer / single-consumer (SPSC)** use cases only.

**Fail-on-full mode** (`QUEUE_OVERWRITE_ON_FULL 0`) is lock-free for SPSC provided:
- `size_t` loads and stores are naturally atomic on the target architecture.
- The fence macros provide the required memory ordering.

**Overwrite-on-full mode** (`QUEUE_OVERWRITE_ON_FULL 1`, default) is **not lock-free** because the producer may advance the consumer's `tail` index when the queue is full. If the producer and consumer can preempt each other (e.g. an ISR vs. a main loop), you must supply critical section macros:

```c
#define QUEUE_ENTER_CRITICAL()  disable_interrupts()
#define QUEUE_EXIT_CRITICAL()   enable_interrupts()
#define QUEUE_OVERWRITE_ON_FULL 1
#include "queue_conf.h"
#include "queue.h"
```

## Configuration

All macros live in `config/queue_conf.h` and can be overridden before including
the header or passed as `-D` flags on the compiler command line.

| Macro | Description | Default |
|---|---|---|
| `QUEUE_OVERWRITE_ON_FULL` | `1`: enqueue overwrites oldest item on full, returns `QUEUE_STATUS_OVERWROTE`. `0`: enqueue fails, returns `QUEUE_STATUS_FULL` (lock-free SPSC safe). | `1` |
| `QUEUE_MAX_CAPACITY` | Compile-time upper bound on capacity. The internal ring size is `capacity + 1U`; this guards against `size_t` overflow. Lower it to cap RAM usage project-wide. | `SIZE_MAX - 1U` |
| `QUEUE_ENTER_CRITICAL()` | Hook called before any non-atomic read-modify-write in overwrite mode. | no-op |
| `QUEUE_EXIT_CRITICAL()` | Hook called after the critical region exits. | no-op |
| `QUEUE_FENCE_ACQUIRE()` | Acquire memory fence. | `atomic_thread_fence` / `__atomic_thread_fence` / compiler barrier |
| `QUEUE_FENCE_RELEASE()` | Release memory fence. | `atomic_thread_fence` / `__atomic_thread_fence` / compiler barrier |
| `QUEUE_FENCE_SEQ_CST()` | Sequential-consistency fence. Also aliased as `QUEUE_BARRIER()`. | `atomic_thread_fence` / `__atomic_thread_fence` / compiler barrier |

## Building

```sh
# Library only (release)
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# With unit tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build
```

## Notes

| Topic | Note |
|---|---|
| **Memory** | All storage lives inside the queue instance. The instance must outlive any producer or consumer that references it. |
| **Thread safety** | SPSC only. Multi-producer or multi-consumer use requires external locking. |
| **Full policy** | Choose fail-on-full for lock-free SPSC; choose overwrite-on-full (with critical sections) for lossy sensor or logging pipelines where the newest data matters most. |
| **Error handling** | All functions return `queue_status_t`; no `errno`, no exceptions. |
| **WCET** | All operations are O(1) with no branching on the data path beyond the full/empty check. |
| **`count()` under concurrency** | In fail-on-full lock-free mode, `count()` is not a stable snapshot but is guaranteed to remain within `[0, capacity]`. |

## API Reference

### Status codes

All functions that can fail return a `queue_status_t`:

| Value | Name | Meaning |
|---|---|---|
| `0` | `QUEUE_STATUS_OK` | Operation succeeded |
| `1` | `QUEUE_STATUS_EMPTY` | Dequeue attempted on an empty queue |
| `2` | `QUEUE_STATUS_FULL` | Enqueue attempted on a full queue (fail-on-full mode only) |
| `3` | `QUEUE_STATUS_BAD_ARG` | A required pointer argument was `NULL` |
| `4` | `QUEUE_STATUS_OVERWROTE` | Enqueue succeeded by overwriting the oldest item (overwrite mode only) |

### Queue definition

```c
QUEUE_DEFINE(name, type, capacity)
```

Defines a new queue type `name_t` and its complete API. `capacity` must be a
compile-time constant greater than zero and no larger than `QUEUE_MAX_CAPACITY`.
A `_Static_assert` fires at compile time if either constraint is violated.

### Initialisation

```c
void name_init(name_t *q);
```

Reset `head` and `tail` to zero. Call once before first use, or to re-use a
queue after `name_clear()`.

### Enqueue / Dequeue

```c
queue_status_t name_enqueue(name_t *q, const type *item);
queue_status_t name_dequeue(name_t *q, type *out);
```

`enqueue` copies `*item` into the next available slot. In overwrite mode it
advances `tail` on full and returns `QUEUE_STATUS_OVERWROTE`; in fail-on-full
mode it returns `QUEUE_STATUS_FULL` without modifying the queue. Both return
`QUEUE_STATUS_BAD_ARG` if any pointer is `NULL`.

`dequeue` copies the oldest item into `*out` and advances `tail`. Returns
`QUEUE_STATUS_EMPTY` if the queue is empty, `QUEUE_STATUS_BAD_ARG` if any
pointer is `NULL`.

### Query functions

```c
bool   name_is_empty(const name_t *q);
bool   name_is_full (const name_t *q);
size_t name_count   (const name_t *q);
size_t name_capacity(void);
```

`capacity()` is a compile-time constant wrapped in a function for uniformity.
`count()` reads `head` and `tail` snapshots and returns the number of items
currently in the queue.

### Clear

```c
void name_clear(name_t *q);
```

Reset `head` and `tail` to zero without touching the buffer contents.

```c
QUEUE_CLEAR(name, q)
```

Generic macro equivalent to `name_clear(q)`. Useful when the queue type name is
itself a macro parameter.
