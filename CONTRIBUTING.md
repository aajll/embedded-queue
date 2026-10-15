# Contributing to embedded-queue

`embedded-queue` is a small C11 single-producer, single-consumer ring buffer whose code `QUEUE_DEFINE` generates in the caller's files. A wrong memory order or a missed critical section loses or tears items only under preemption, and no single-threaded test shows it, so read [design.md](docs/design.md) before you change the indices, the backends or the full policies.

## Getting started

Install the Python tools with [uv](https://docs.astral.sh/uv/), as CI does. The MISRA analysis also needs a current `cppcheck`; CI installs it from Homebrew because the 2.13 release in Ubuntu 24.04 reports different findings. The same commands CI runs, locally:

```sh
# Python tools
uv tool install meson
uv tool install ninja
uv tool install gcovr
uv tool install misch

# Configure with tests and sanitisers (CI default)
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# ThreadSanitizer
meson setup build-tsan --buildtype=debug -Dbuild_tests=true -Db_sanitize=thread
meson compile -C build-tsan && meson test -C build-tsan

# Coverage (CI gate is 100% line and branch)
meson setup build_cov --buildtype=debug -Dbuild_tests=true -Db_coverage=true -Dc_args=-fprofile-update=atomic
meson compile -C build_cov && meson test -C build_cov
gcovr --root . --filter 'src/' --filter 'include/' --fail-under-line 100 --fail-under-branch 100 --print-summary build_cov

# MISRA C:2023, one run per atomic backend (reads build/compile_commands.json)
env -u MISRA_RULE_TEXTS misch run --baseline
env -u MISRA_RULE_TEXTS misch run --baseline --profile c11
env -u MISRA_RULE_TEXTS misch run --baseline --profile no_atomics
misch deviations

# Formatting (pinned clang-format through uvx)
scripts/format.sh --fix
```

## Source style

- Formatting is mandatory. Run `scripts/format.sh --fix` before submitting, or `scripts/format.sh` to check without writing, as CI does. The script pins the clang-format release and runs it through `uvx`, so an editor formatter gives the same result only when it runs the same release.
- `queue_conf.h` must be included before `queue.h`. Keep a blank line between the two includes; clang-format sorts includes only within a block.
- 8-space indent, Linux brace style, 80-column limit. Match the existing conventions and do not reformat unrelated code.
- The Meson build system is the single source of truth. Update `meson.build` and `tests/meson.build` when you add or remove files. No CMake, no Make, and no other build systems.

## C language rules

- C11 only.
- Use fixed-width types from `<stdint.h>` and `<stdbool.h>`, and `size_t` for indices. Code that must build where `CHAR_BIT` is 16 cannot use `uint8_t`.
- No heap allocation (`malloc`, `free`, VLAs), no loops, no division, no global state. `memcpy` is the only library call.
- Validate every pointer argument at the public API boundary and return `QUEUE_STATUS_BAD_ARG` rather than fault.
- Public identifiers use the `queue_` or `QUEUE_` prefix, internal ones `queue__` or `QUEUE__`.

## Queue rules

- The producer releases `head` after copying an item in, and the consumer releases `tail` after copying one out; each acquires the other index before touching a slot. Every backend keeps this pairing.
- In fail-on-full mode the producer writes only `head` and the consumer only `tail`, so the queue stays lock-free with empty hooks.
- The critical-section hooks run around every generated `enqueue` and `dequeue`, and nowhere else.
- A change to the generated queue type changes the layout callers allocate. Keep it compatible within a release line.

## MISRA C:2023

The library is analysed with `misch` (cppcheck-backed MISRA C:2023 analysis), configured by `misra.toml`, through `src/queue.c`, once for each atomic backend: the default `__atomic` run and the `c11` and `no_atomics` profiles. CI fails on any finding that is not in that backend's baseline in `analysis/baseline/`. Run it from a configured `build/` directory, with `MISRA_RULE_TEXTS` unset, as CI does. If your change introduces a finding:

1. Prefer restructuring the code so the rule is satisfied. Deviate only when compliance would make the code genuinely worse, and never restructure purely to hide a finding from the checker; an honest deviation beats evasion.
2. Suppress at the point of use with `/* cppcheck-suppress misra-c2012-<rule> ; @deviation <rationale> */`, or add a justified project-wide entry to `analysis/deviations/misra-deviations.txt` for a house-style rule.
3. Verify with `misch run --baseline` for every backend and `misch deviations`, which requires every suppression to carry a rationale, and justify the deviation in the pull request description.

Do not regenerate a baseline to admit a new finding. It changes only after a deliberate review, with `misch baseline`.

## Tests and coverage

- Add a test for every bug fix and every new feature.
- Each test is its own executable in `tests/test_*.c`, registered in `tests/meson.build`. Tests that force a backend or a full policy do so with `c_args` there.
- A change to the index ordering needs a test with a real producer and consumer thread, run for each backend it touches, and must pass under ThreadSanitizer.
- CI enforces 100 percent line and branch coverage of `src/` and `include/`. Every new branch needs a test that fails when the branch is broken.
- The `queue docs examples` test compiles every C code block in `README.md`, `CONTRIBUTING.md`, and `docs/` through `tests/docs/check_snippets.py`. Each document is one source file and includes the headers itself, so its first block sets the configuration. A block compiles at file scope by default. On the line directly above the fence, put `<!-- snippet: body -->` for a block of statements, or `<!-- snippet: skip -->` for pseudo-code or a copy of a header definition.

## Documentation

| File | Purpose |
| --- | --- |
| `README.md` | Landing page: quick start, description, validated toolchains, and links. Keep it short. |
| `docs/integration.md` | Requirements, installation, configuration, building, and cross builds. |
| `docs/api.md` | Reference for `QUEUE_DEFINE`, the generated functions, the status codes, the configuration macros, and recipes. |
| `docs/design.md` | The ring, the atomic backends and memory ordering, the full policies and hooks, the overwrite-mode warning, concurrency, worst-case cost, the footprint, failure behaviour, MISRA position, and verification. |

Update `docs/api.md` and the header comments together when an interface changes, and check every value an example states against a build. Update `docs/design.md` when the ordering, a backend, a full policy or the verification changes, and the MISRA table when a baseline changes. Keep it standalone: describe how the library works, not how it was built or which other projects use it. Write prose in British English, one line per paragraph.

## API stability

- `QUEUE_DEFINE`, the generated function signatures, the status codes and the configuration macros are stable within a major release line.
- Which item a full overwrite-mode enqueue discards is observable behaviour. Changing it or a signature is a breaking change and needs a major release.
- The members of a generated queue type are private, but its size is not: callers allocate it, so a layout change needs a `CHANGELOG.md` entry.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `docs: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` a change that neither fixes a bug nor adds a feature

Keep the subject under about 70 characters. Use the body to explain why the change is needed.

## Pull requests

- Open an issue first for non-trivial changes, and always for a change to the memory ordering, a backend or a full policy, so the design can be agreed before implementation.
- Keep pull requests focused: one feature or one fix.
- All CI checks must pass: tests under ASan and UBSan, TSan, release build, coverage gate, the TI C2000 build, clang-format, and MISRA analysis against the baselines in `analysis/baseline/`.
- Add a `CHANGELOG.md` entry under `## [Unreleased]` for a user-visible change.
- Flag any new MISRA deviation, and any change to the layout of a generated queue type, in the description.

## When in doubt

Open an issue. The library is small, but a queue that loses or tears items only under preemption fails in the field, not in review.
