# API reference

`queue_conf.h` holds the configuration and `queue.h` the interface. The examples in this document compile together as one file, in order.

## Conventions

- `QUEUE_DEFINE` generates a queue type and its functions. The caller owns every queue object; it may be static, on the stack, or embedded in another structure. No function allocates or blocks.
- The members of a queue are visible because the caller allocates it, but treat them as private: read and change the queue only through the functions.
- Items are copied in and out by value with `memcpy`, so the queue never holds a pointer to caller storage.
- Every pointer argument is checked. A NULL argument never dereferences.
- One context, the producer, calls `enqueue`; one other context, the consumer, calls `dequeue`. Either may call the query functions. Call `init` and `clear` only while neither is using the queue. [design.md](design.md#concurrency) gives the full contract.

## Configuring and including

Define the configuration before `queue_conf.h`, then include `queue.h`. This configuration keeps the default overwrite-on-full policy and supplies critical-section hooks that save and restore an interrupt mask:

```c
#include <stdint.h>

uint16_t irq_save(void);             /* masks interrupts, returns the old mask */
void     irq_restore(uint16_t mask); /* restores the mask */

#define QUEUE_ENTER_CRITICAL() const uint16_t queue_irq_mask = irq_save()
#define QUEUE_EXIT_CRITICAL()  irq_restore(queue_irq_mask)
#include "queue_conf.h"

#include "queue.h"
```

The hooks expand as two statements in the same block of each generated `enqueue` and `dequeue`, so a declaration in `QUEUE_ENTER_CRITICAL` is visible to `QUEUE_EXIT_CRITICAL`, and functions they call need only be declared before `QUEUE_DEFINE`.

## Defining a queue

<!-- snippet: skip -->
```c
QUEUE_DEFINE(name, type, capacity)
```

`QUEUE_DEFINE` defines a structure type `name_t` and the functions below at file scope, all `static inline`. `type` is any complete object type, and `capacity` is an integer constant expression for the number of items the queue holds. A `_Static_assert` stops the build if `capacity` exceeds `QUEUE_MAX_CAPACITY`. Use a capacity of at least 1: a capacity of 0 compiles, but the queue is then always both empty and full and stores nothing.

```c
typedef struct {
        uint32_t id;
        uint16_t length;
        uint16_t data[4];
} frame_t;

QUEUE_DEFINE(frame_queue, frame_t, 8)

static frame_queue_t frames;
```

Each instantiation needs its own `name`. Two files that share a queue must expand the same `QUEUE_DEFINE` under the same configuration; [integration.md](integration.md#configuration) shows how.

## Initialisation

<!-- snippet: skip -->
```c
void name_init(name_t *q);
void name_clear(name_t *q);
QUEUE_CLEAR(name, q)
```

`name_init` sets both indices to zero, which empties the queue. Call it before any other function on the queue; a queue with static storage duration is already empty, because its indices start at zero. `name_clear` does the same, and `QUEUE_CLEAR(name, q)` calls `name_clear(q)` as a single statement for code where `name` is itself a macro parameter. None of them changes the stored items or runs the critical-section hooks, and a NULL `q` does nothing.

<!-- snippet: body -->
```c
frame_queue_init(&frames);
QUEUE_CLEAR(frame_queue, &frames);
```

## Enqueue and dequeue

<!-- snippet: skip -->
```c
queue_status_t name_enqueue(name_t *q, const type *item);
queue_status_t name_dequeue(name_t *q, type *out);
```

`name_enqueue` copies `*item` into the queue as the newest item. When the queue is full, the result depends on `QUEUE_OVERWRITE_ON_FULL`:

- In overwrite mode (the default), the oldest item is discarded, `*item` is stored, and the function returns `QUEUE_STATUS_OVERWROTE`.
- In fail-on-full mode, nothing changes and the function returns `QUEUE_STATUS_FULL`.

Otherwise it returns `QUEUE_STATUS_OK`.

`name_dequeue` copies the oldest item to `*out`, removes it, and returns `QUEUE_STATUS_OK`. It returns `QUEUE_STATUS_EMPTY`, without writing `*out`, when the queue is empty.

Both return `QUEUE_STATUS_BAD_ARG` and change nothing when either pointer is NULL. Both run `QUEUE_ENTER_CRITICAL` before the operation and `QUEUE_EXIT_CRITICAL` after it, in either full policy and for every call, including one rejected with `QUEUE_STATUS_BAD_ARG`.

<!-- snippet: body -->
```c
frame_t rx;
const frame_t tx = {.id = 0x123U, .length = 2U, .data = {1U, 2U}};

if (frame_queue_enqueue(&frames, &tx) == QUEUE_STATUS_OVERWROTE) {
        /* the oldest frame was lost to make room */
}
while (frame_queue_dequeue(&frames, &rx) == QUEUE_STATUS_OK) {
        /* handle rx */
}
```

## Queries

<!-- snippet: skip -->
```c
bool   name_is_empty(const name_t *q);
bool   name_is_full(const name_t *q);
size_t name_count(const name_t *q);
size_t name_capacity(void);
```

`name_is_empty` and `name_is_full` report whether a dequeue would return `QUEUE_STATUS_EMPTY` and whether an enqueue would find the queue full. `name_count` returns the number of items held, from 0 to the capacity. `name_capacity` returns the `capacity` given to `QUEUE_DEFINE`. A NULL `q` reads as empty, not full, and a count of 0.

The queries take no critical section. While the other context is active, each result is a snapshot that may already be out of date when it returns, but `name_count` never exceeds the capacity.

## Status codes

<!-- snippet: skip -->
```c
typedef enum {
        QUEUE_STATUS_OK = 0,
        QUEUE_STATUS_EMPTY,
        QUEUE_STATUS_FULL,
        QUEUE_STATUS_BAD_ARG,
        QUEUE_STATUS_OVERWROTE
} queue_status_t;
```

The values are 0 to 4 in this order. `QUEUE_STATUS_FULL` occurs only in fail-on-full mode and `QUEUE_STATUS_OVERWROTE` only in overwrite mode.

## Configuration macros

Each macro is defined in `queue_conf.h` behind an `#ifndef` guard and is overridden by a definition before that header or by a `-D` flag.

| Macro | Default | Effect |
| --- | --- | --- |
| `QUEUE_OVERWRITE_ON_FULL` | `1` | `1` discards the oldest item when an enqueue finds the queue full; `0` rejects the new item. |
| `QUEUE_ENTER_CRITICAL()`, `QUEUE_EXIT_CRITICAL()` | empty statements | Run around every `enqueue` and `dequeue`. Overwrite mode needs them when the producer and consumer can preempt each other. |
| `QUEUE_SINGLE_CONTEXT` | `0` | `1` declares that one execution context performs every queue operation, which silences the warning for overwrite mode without hooks. |
| `QUEUE_MAX_CAPACITY` | `SIZE_MAX - 1U` | Largest capacity `QUEUE_DEFINE` accepts. Lower it to cap queue sizes across a project. |
| `QUEUE_USE_GNU_ATOMICS`, `QUEUE_USE_C11_ATOMICS`, `QUEUE_USE_NO_ATOMICS` | the first available | Define one to choose how the indices are accessed; see [design.md](design.md#atomic-backends). |
| `QUEUE_FENCE_ACQUIRE()`, `QUEUE_FENCE_RELEASE()` | the backend's fence | Order item copies against index accesses under `QUEUE_USE_NO_ATOMICS` only. |
| `QUEUE_FENCE_SEQ_CST()` | the backend's fence | Full fence. The queue does not use it; `QUEUE_BARRIER()` is a deprecated alias. |
| `QUEUE__UNUSED` | `__attribute__((unused))` on GCC, Clang and TI compilers, otherwise empty | Marks every generated function so that unused ones raise no diagnostic. |

Defining more than one `QUEUE_USE_*` macro is a compile error.

## Recipes

### An interrupt handler and a main loop

[integration.md](integration.md#configuration) shows a fail-on-full queue shared between an interrupt handler that produces and a main loop that consumes, with a count of the items the handler could not store. Fail-on-full mode needs no hooks for that pattern.

### A RAM budget

Check the size of the queue type where it is defined, so a capacity or item change that outgrows its budget stops the build:

```c
_Static_assert(sizeof(frame_queue_t) <= 192U, "frame queue exceeds its RAM budget");
```

`frame_queue_t` is 160 bytes on x86-64: 16 for the indices and nine 16-byte frames. `sizeof` counts 16-bit words on a target where `CHAR_BIT` is 16, so express the budget in the target's units. [design.md](design.md#footprint) gives the size formula.

## Version

`queue.h` defines the release it belongs to, so a copied-in header still identifies itself:

<!-- snippet: skip -->
```c
#define QUEUE_VERSION_MAJOR 1
#define QUEUE_VERSION_MINOR 2
#define QUEUE_VERSION_PATCH 6
#define QUEUE_VERSION "1.2.6"
```

`QUEUE_VERSION_NUMBER` is `MAJOR * 10000 + MINOR * 100 + PATCH` as a `long`, so it holds any version where `int` is 16 bits. Compare it in the preprocessor to require a minimum release:

```c
#if QUEUE_VERSION_NUMBER < 10206L
#error "queue 1.2.6 or later is required"
#endif
```
