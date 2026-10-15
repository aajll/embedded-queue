# embedded-queue

[![CI](https://github.com/aajll/embedded-queue/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/embedded-queue/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**A C11 single-producer, single-consumer ring buffer whose storage lives in the queue instance, typed at compile time by one macro, with no heap.**

## Quick start

Copy `include/queue_conf.h` and `include/queue.h` into your project. For a Meson subproject or an installed package, see the [integration guide](docs/integration.md).

```c
#include <stdint.h>
#include <stdio.h>

#define QUEUE_OVERWRITE_ON_FULL 0 /* fail on full instead of overwriting */
#include "queue_conf.h"

#include "queue.h"

/* sample_queue_t and its functions, for two uint16_t items. */
QUEUE_DEFINE(sample_queue, uint16_t, 2)

static sample_queue_t samples;

int
main(void)
{
        uint16_t sample = 0U;

        sample_queue_init(&samples);

        /* Prints "enqueue 1: 0", "enqueue 2: 0", then "enqueue 3: 2",
         * QUEUE_STATUS_FULL, because the queue holds two items. */
        for (uint16_t i = 1U; i <= 3U; i++) {
                printf("enqueue %u: %d\n", (unsigned)i,
                       (int)sample_queue_enqueue(&samples, &i));
        }

        /* Prints "dequeue 1" and "dequeue 2", oldest first. */
        while (sample_queue_dequeue(&samples, &sample) == QUEUE_STATUS_OK) {
                printf("dequeue %u\n", (unsigned)sample);
        }
        return 0;
}
```

`queue_conf.h` must come before `queue.h`; the blank line between them keeps include sorting from swapping them.

## Description

`QUEUE_DEFINE(name, type, capacity)` generates a structure `name_t`, holding the head and tail indices and an array of `capacity + 1` items, and typed functions to initialise, enqueue, dequeue, count and clear it. Every queue copies items in and out by value, and every function finishes in constant time.

- One producer and one consumer, such as an interrupt handler and a main loop, with acquire and release ordering on the indices through the GCC `__atomic` builtins, C11 `<stdatomic.h>`, or volatile accesses and configurable fences
- A full policy chosen at build time: overwrite the oldest item, or reject the new one
- Critical-section hooks for overwrite mode, where the producer also moves the consumer's index, and a compile-time warning when overwrite mode is built without them
- Capacity checked by `_Static_assert`, and every pointer argument checked at run time
- Tests under ASan, UBSan and ThreadSanitizer with 100% line and branch coverage, and MISRA C:2023 analysis with [`misch`](https://pypi.org/project/misch/) for each atomic backend

## Validated toolchains

| Toolchain               | Target                        | Validation                                                     |
| ----------------------- | ----------------------------- | -------------------------------------------------------------- |
| GCC                     | x86_64 Linux                  | All tests, all three atomic backends                           |
| DSP vendor C11 compiler | 16-bit MAU (`CHAR_BIT == 16`) | Library and a `QUEUE_DEFINE` compile, both full policies, two object formats |

Target linking and execution belong to the consuming firmware. See [cross builds](docs/integration.md#cross-builds).

## Documentation

- [Integration guide](docs/integration.md) — copy-in, Meson subproject, installed package, configuration, building, and cross builds
- [API reference](docs/api.md) — `QUEUE_DEFINE`, the generated functions, status codes, the configuration macros, and recipes
- [Design](docs/design.md) — the ring, the atomic backends and memory ordering, the full policies and critical sections, the footprint, failure behaviour, and verification
- [Changelog](CHANGELOG.md) — release history

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request. Report security issues as described in [SECURITY.md](.github/SECURITY.md).

## License

MIT. See [LICENSE](LICENSE).
