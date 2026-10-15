# Integration guide

This guide shows how to add `embedded-queue` to a project, configure it, build it, and run its tests. The API is described in [api.md](api.md), and the ring and its guarantees in [design.md](design.md).

## Requirements

- A C11 compiler with `_Static_assert`, and a target where `CHAR_BIT` is 8 or 16.
- `<stdint.h>` defining `SIZE_MAX`, `<stdbool.h>`, `<stddef.h>`, and `<string.h>` for `memcpy`, which copies every item.
- For lock-free use between two execution contexts: the GCC or Clang `__atomic` builtins, or C11 `<stdatomic.h>`, with lock-free `size_t` atomics; or a target where an aligned `size_t` load or store is indivisible. [design.md](design.md#atomic-backends) explains the choice.
- For the tests: Meson, Ninja and Python 3.

## Copy-in

The queue is header-only. Copy two files into your source tree:

```
include/queue_conf.h
include/queue.h
```

Put their directory on the include path and include them in this order, with any configuration before the first:

```c
#include <stdint.h>

#define QUEUE_OVERWRITE_ON_FULL 0
#include "queue_conf.h"

#include "queue.h"
```

`queue.h` stops with `#error` if `queue_conf.h` has not been included. Keep the blank line between the two includes if your formatter sorts includes, because sorting works only within a block.

`queue.h` defines the version macros described in [api.md](api.md#version).

## Meson subproject

Put the repository in `subprojects/embedded-queue`, as a checkout or through a wrap file, then declare the dependency:

```meson
queue_dep = dependency('queue', fallback: ['embedded-queue', 'queue_dep'])
```

`dependency('queue')` finds an installed package through `pkg-config`, and otherwise falls back to the subproject. `queue_dep` adds `include/` and links `libqueue.a`, which holds no queue code; every function is generated in the file that calls `QUEUE_DEFINE`. The only option is `build_tests` (default `false`), which builds the unit tests and the documentation example test.

## Installed package

```sh
meson setup build --buildtype=release -Dbuild_tests=false --prefix=/usr/local
meson compile -C build
meson install -C build
cc app.c $(pkg-config --cflags --libs queue) -o app
```

The install contains `queue.h` and `queue_conf.h` in a `queue/` subdirectory of the include directory, the static archive `libqueue.a`, and `queue.pc`. The `pkg-config` flags put `queue/` on the include path, so `#include "queue.h"` works unchanged.

## Configuration

Every configuration macro in `queue_conf.h` sits behind an `#ifndef` guard, so a definition placed before `#include "queue_conf.h"`, or a `-D` flag, replaces the default. The macros are described in [api.md](api.md#configuration-macros).

The configuration applies to the translation unit that includes the headers, because the code is generated there. Two files that share a queue must therefore see the same configuration and the same `QUEUE_DEFINE`. A project header that holds both is the simplest way to guarantee it:

```c
/* adc_queue.h, included by the interrupt handler and the main loop */
#include <stdint.h>

#define QUEUE_OVERWRITE_ON_FULL 0
#include "queue_conf.h"

#include "queue.h"

QUEUE_DEFINE(adc_queue, uint16_t, 32)

extern adc_queue_t adc_samples; /* defined in one source file */
```

One source file defines the queue. The interrupt handler produces and the main loop consumes, and in fail-on-full mode the pair needs no critical-section hooks:

```c
/* adc.c */
uint16_t adc_read(void);
void     process(uint16_t sample);

adc_queue_t adc_samples;
static volatile uint32_t adc_dropped;

void
adc_isr(void)
{
        const uint16_t sample = adc_read();

        if (adc_queue_enqueue(&adc_samples, &sample) == QUEUE_STATUS_FULL) {
                adc_dropped++;
        }
}

void
adc_poll(void)
{
        uint16_t sample;

        while (adc_queue_dequeue(&adc_samples, &sample) == QUEUE_STATUS_OK) {
                process(sample);
        }
}
```

The critical-section hooks expand inside the functions `QUEUE_DEFINE` generates, so they may refer to anything declared before it.

## Build and test

```sh
# Library only
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# All tests under ASan and UBSan
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose
```

The test suite forces each atomic backend and each full policy in separate executables, so one configuration runs all of them. ThreadSanitizer, coverage, formatting and MISRA commands are in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Cross builds

The consuming firmware owns its cross file, linker command file, and memory placement. No cross file ships with this project.

CI compiles the library and a `QUEUE_DEFINE` instantiation with warnings as errors using two releases of a vendor C11 compiler for a 16-bit-MAU DSP target, under both of its object formats and in both full policies. That compiler has no `<stdatomic.h>` and is neither GCC nor Clang, so it selects the volatile backend, and the default fences are empty; [design.md](design.md#volatile-backend) says what to define. This covers the source dialect and type assumptions only; linking and execution remain checks for the consumer.

On such a target `CHAR_BIT` is 16 and `sizeof` counts 16-bit words. [design.md](design.md#footprint) gives the size of a queue there.
