# Design

`embedded-queue` passes items of one type from a producer to a consumer through a fixed ring of slots inside the queue object, with no heap and constant-time operations. This document describes the ring, how the indices are shared between two execution contexts, the two full policies, the footprint, and how the library is verified. The API is in [api.md](api.md).

## Goals

- All storage is in one object the caller places, and its size is known at compile time.
- An interrupt handler and a main loop, or two threads, can exchange items without a lock when the queue rejects items on full.
- Every operation finishes in constant time.
- Portable C11, including targets where `CHAR_BIT` is 16 and compilers without atomics.

Several producers or consumers without external exclusion, variable-size items, blocking waits and dynamic resizing are out of scope.

## The ring

`QUEUE_DEFINE(name, type, capacity)` defines:

<!-- snippet: skip -->
```c
typedef struct {
        queue_core_t core;                       /* head and tail */
        type buffer[((size_t)(capacity)) + 1U];
} name_t;
```

`head` is the slot the producer writes next and `tail` the slot the consumer reads next. The queue is empty when they are equal and full when the slot after `head` is `tail`. One slot always stays unused, so empty and full are told apart from the two indices alone, without a shared count that both sides would write. An index advances by an increment and a comparison with the ring size, never by division.

The queue logic is in `static inline` functions in `queue.h` that take a `void *` to the queue, the offset of `buffer`, `sizeof(type)` and the ring size. `core` is the first member, so a pointer to any queue is also a pointer to its indices. The functions `QUEUE_DEFINE` generates are typed wrappers that pass those constants and run the critical-section hooks, so every instantiation shares one implementation and the compiler can fold the constants. Items are copied with `memcpy`.

## Atomic backends

The producer and consumer exchange items through the indices, so each index update must become visible only after the item copy it publishes. Enqueue reads its own `head` relaxed and `tail` with acquire, copies the item into the slot, then stores the new `head` with release. Dequeue mirrors it: relaxed `tail`, acquire `head`, copy out, release `tail`. The consumer therefore never reads a slot before its item is complete, and the producer never reuses a slot before the consumer has copied it out.

`queue_conf.h` picks the first available backend for those accesses, unless one `QUEUE_USE_*` macro forces it:

1. `QUEUE_USE_GNU_ATOMICS` — the `__atomic` builtins, on GCC and Clang.
2. `QUEUE_USE_C11_ATOMICS` — `_Atomic size_t` and `<stdatomic.h>`, on a C11 compiler that does not define `__STDC_NO_ATOMICS__`.
3. `QUEUE_USE_NO_ATOMICS` — `volatile size_t` and explicit fences, everywhere else.

Both atomic backends end in a `_Static_assert` that `size_t` atomics are always lock-free on the target, because a lock-based atomic could deadlock when an interrupt handler interrupts the main loop inside one. A target that fails it must use the volatile backend.

### Volatile backend

The volatile backend loads and stores the indices as plain `volatile` accesses. `QUEUE_FENCE_ACQUIRE()` follows each load of the other side's index, and `QUEUE_FENCE_RELEASE()` precedes each store of its own. It is correct only where an aligned `size_t` load or store is indivisible and the fences order the item copy against the index accesses, which holds for two contexts on one core, such as an interrupt handler and a main loop. On a target where `CHAR_BIT` is 16, `size_t` is usually two memory words wide, so it must be loaded and stored by one instruction.

The default fences depend on the compiler:

| Compiler | Default fences |
| --- | --- |
| GCC or Clang, volatile backend forced | `__asm__ volatile("" ::: "memory")`, a compiler barrier |
| Any other | Empty |

With empty fences, nothing in C stops the compiler from moving the non-volatile item copy past the volatile index store, and the consumer would then read a slot before its item is written. Define `QUEUE_FENCE_ACQUIRE()` and `QUEUE_FENCE_RELEASE()` as the toolchain's compiler barrier on such a compiler, or confirm in the generated code that the copy precedes the store. The fences never need to be hardware barriers on a single core.

## Full policies

`QUEUE_OVERWRITE_ON_FULL` chooses what an enqueue does when the queue is full.

In fail-on-full mode (`0`), the enqueue returns `QUEUE_STATUS_FULL` and changes nothing. The producer then writes only `head` and the consumer only `tail`, so with the hooks left empty the queue is lock-free for one producer and one consumer. A rejected item stays with the producer, which can count or retry it.

In overwrite mode (`1`, the default), the enqueue advances `tail` past the oldest item, stores the new one, and returns `QUEUE_STATUS_OVERWROTE`. The consumer always sees the newest `capacity` items, which suits sensor readings and logs where the latest data matters most. But the producer now writes `tail` too, so the two contexts race on `tail` and on the oldest slot: without exclusion, a dequeue can return an item the producer is overwriting, and one context's update of `tail` can be lost.

### Critical sections

`QUEUE_ENTER_CRITICAL()` and `QUEUE_EXIT_CRITICAL()` run around every generated `enqueue` and `dequeue`, in either policy, so each of those calls is atomic with respect to the others. Overwrite mode needs them whenever the producer and consumer can preempt each other; masking interrupts suits an interrupt handler and a main loop, and a mutex suits two threads. The section lasts one item copy plus a few index operations, which is the added interrupt latency. Hooks in fail-on-full mode make the queue lock-based, so leave them empty there unless several producers or consumers share the queue. `init`, `clear` and the queries never run the hooks.

### The overwrite-mode warning

A queue in overwrite mode with the default, empty `QUEUE_ENTER_CRITICAL` is unsafe between preempting contexts, so `queue.h` warns at compile time unless `QUEUE_SINGLE_CONTEXT` is `1`:

| Compiler | Diagnostic |
| --- | --- |
| GCC | `#pragma GCC warning`, an error under `-Werror` |
| Clang | `#pragma GCC warning`, an error only under `-Werror=#pragma-messages` |
| TI compilers | `#warning` |
| Others | none |

The warning appears in every translation unit that includes `queue.h` with that configuration, whether or not it defines a queue. Defining `QUEUE_ENTER_CRITICAL`, choosing fail-on-full mode, or setting `QUEUE_SINGLE_CONTEXT` to `1` when one execution context performs every queue operation each silences it.

## Concurrency

The contract is one producer context, calling `enqueue`, and one consumer context, calling `dequeue`. Either may call `is_empty`, `is_full`, `count` and `capacity` at any time; each is a snapshot of the two indices, and `count` stays between 0 and the capacity even when both indices move during the call. `init` and `clear` write both indices without the hooks, so call them only while neither context is using the queue.

Several producers or several consumers need exclusion among themselves. Hooks that exclude every context using the queue provide it, at the cost of lock-freedom.

The atomic backends are correct between threads on different cores. The volatile backend is correct only between contexts on one core, as [described above](#volatile-backend).

## Worst-case cost

Every function runs in constant time. `enqueue` and `dequeue` copy `sizeof(type)` bytes with `memcpy` and perform two index loads, at most two index stores and a few comparisons, plus the hooks. The queries perform two index loads and a few comparisons or subtractions. Nothing loops in the library, nothing divides, and nothing calls the standard library except `memcpy`.

## Footprint

`sizeof(name_t)` is two `size_t` indices and `capacity + 1` items, padded to the larger of their alignments. The spare slot costs one item.

| Instantiation | x86-64 | Target where `CHAR_BIT` is 16 |
| --- | --- | --- |
| `QUEUE_DEFINE(q, uint16_t, 2)` | 24 bytes: 16 of indices, 6 of items, 2 of padding | 8 words: 4 of indices, 3 of items, 1 of padding |
| `QUEUE_DEFINE(q, uint32_t, 15)` | 80 bytes: 16 of indices, 64 of items | 36 words: 4 of indices, 32 of items |

On a target where `CHAR_BIT` is 16, `sizeof` counts 16-bit words and `size_t` is two of them. A queue of small items pays relatively more for its indices, so prefer one queue of a structure over several parallel queues.

## Failure behaviour

| Condition | Result |
| --- | --- |
| NULL queue to `init` or `clear` | Nothing is done. |
| NULL queue, `item` or `out` to `enqueue` or `dequeue` | `QUEUE_STATUS_BAD_ARG`; nothing is read or written. The hooks still run. |
| NULL queue to a query | `is_empty` returns true, `is_full` false and `count` 0. |
| `enqueue` on a full queue | `QUEUE_STATUS_OVERWROTE` after discarding the oldest item, or `QUEUE_STATUS_FULL` with the queue unchanged. |
| `dequeue` on an empty queue | `QUEUE_STATUS_EMPTY`; `*out` is not written. |
| Capacity 0, or above `QUEUE_MAX_CAPACITY` | Compile error from a `_Static_assert`. |
| `queue.h` included without `queue_conf.h`, or two `QUEUE_USE_*` macros | Compile error. |
| Atomic backend on a target without lock-free `size_t` atomics | Compile error from a `_Static_assert`. |
| Overwrite mode without hooks, contexts that preempt each other | A dequeue can return a partly overwritten item, and an update of `tail` can be lost. |
| Two translation units with different configurations for one queue | Undefined; the two sides disagree on the policy, the hooks or the index type. |
| Queue used before `init`, unless it has static storage duration | Undefined; the indices hold arbitrary values. |

## MISRA C:2023

The library is analysed with [`misch`](https://pypi.org/project/misch/), a cppcheck-based MISRA C:2023 checker, once for each atomic backend. The analysis covers `src/queue.c`, which includes both headers but expands no `QUEUE_DEFINE`, so the generated wrappers are checked only as macro text. Each backend has its own baseline of accepted findings in `analysis/baseline/`, and CI fails on any finding outside it. The version macros in `queue.h` carry an inline deviation of Rule 2.5 (unused macro), because they exist for consumers and the library does not use them.

| Rule | `__atomic` | C11 | Volatile | Cause |
| --- | --- | --- | --- | --- |
| 17.3 | 16 | 0 | 0 | Calls to the GCC `__atomic` builtins, which the checker treats as undeclared functions. |
| 2.5 | 9 | 9 | 7 | Macros not used in the analysed code: the backend selection, default fences, `QUEUE_DEFINE` and `QUEUE_CLEAR`. |
| 11.5 | 8 | 8 | 8 | Conversions from `void *` to the index and byte pointers in the type-erased functions. |
| 15.5 | 8 | 8 | 8 | Early returns on NULL arguments and on an empty queue. |
| 13.4 | 0 | 0 | 5 | Index stores of the volatile backend, assignments used as expressions inside the store macro. |
| 12.1 | 0 | 3 | 0 | Operator precedence in the C11 backend's lock-free `_Static_assert`. |
| 17.8 | 2 | 2 | 2 | The index increment modifies its parameter. |
| 18.4 | 2 | 2 | 2 | Pointer addition from the queue to its item array. |
| 20.7 | 2 | 2 | 2 | Macro parameters joined with `##` in `QUEUE_DEFINE` and `QUEUE_CLEAR`, which cannot be parenthesised. |
| 20.10 | 2 | 2 | 2 | The `##` operator in those macros. |

The totals are 49, 36 and 36. The baselines record the findings as they stand. Formal compliance needs a certified analyser and a deviation record from the consuming project.

## Verification

The tests run on the host:

| Test | What it checks |
| --- | --- |
| `queue_test_overwrite`, `queue_test_fail`, `queue_test_fail_mode`, `queue_test_exact_capacity`, `queue_test_capacity_1` | Enqueue on a full queue in each policy, at capacity 1 and at exact capacity, and FIFO order afterwards. |
| `queue_test_wrap`, `queue_test_immediate_dequeue`, `queue_test_empty_after_cycles`, `queue_test_soak` | FIFO order across the end of the ring, repeated fill and drain cycles, and a million enqueue and dequeue pairs in order. |
| `queue_test_clear`, `queue_test_null_validation`, `queue_test_safety` | `clear` and `QUEUE_CLEAR`, NULL arguments to every function, and large capacities. |
| `queue_test_critical` | Each `enqueue` and `dequeue` enters and exits the hooks once. |
| `queue_test_mt`, `queue_test_overwrite_mt`, `queue_test_count_mt` | A producer and a consumer thread passing a million items in fail-on-full mode and in overwrite mode with mutex hooks, and `count` never exceeding the capacity. |
| `queue_test_mt_c11`, `queue_test_count_mt_c11`, `queue_test_no_atomics`, `queue_test_fail_no_atomics` | The same checks with the C11 or the volatile backend forced. |
| `queue_test_unused_override` | A consumer's `QUEUE__UNUSED` is used without a redefinition warning. |
| `queue docs examples` | Every C block in the README, `CONTRIBUTING.md` and `docs/` compiles against the headers. |

At configure time, the test build also checks that the overwrite-mode warning fires without hooks and stays silent with each way of silencing it.

CI runs every test under AddressSanitizer and UndefinedBehaviorSanitizer and again under ThreadSanitizer, gates line and branch coverage of `src/` and `include/` at 100 percent, builds the library in release mode, checks formatting, and runs the MISRA analysis for each backend. It also compiles the library, and `tests/test_queue_wrap.c` in both full policies, with warnings as errors under TI C2000 CGT 22.6.2 and 25.11.1, in both the COFF and EABI formats. Those compilers select the volatile backend with empty default fences; at `-O4`, both releases emit the item copy as a `memcpy` call that precedes the `head` store.
