# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `embedded-queue` (Meson project `queue`)
**Primary goal:** A C11 single-producer, single-consumer ring buffer generated per item type by `QUEUE_DEFINE`, with storage inside the queue object, no heap, constant-time operations, and acquire/release index ordering that is lock-free in fail-on-full mode.

### 1.1 Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose
```

The suite forces the C11 and volatile backends and both full policies in separate executables, so one configuration runs all of them. ThreadSanitizer needs its own build with `-Db_sanitize=thread`.

#### MISRA C:2023, one run per atomic backend (reads `build/compile_commands.json`)

```sh
env -u MISRA_RULE_TEXTS misch run --baseline
env -u MISRA_RULE_TEXTS misch run --baseline --profile c11
env -u MISRA_RULE_TEXTS misch run --baseline --profile no_atomics
misch deviations
```

#### Formatting (pinned clang-format through `uvx`)

```sh
scripts/format.sh          # check, as CI does
scripts/format.sh --fix    # rewrite in place
```

Coverage commands are in `CONTRIBUTING.md`.

### 1.2 Notes

- The queue is header-only: `QUEUE_DEFINE` generates `static inline` code in the caller's translation unit. `src/queue.c` only includes the headers so the build has a library target.
- `queue_conf.h` must be included before `queue.h`. Keep a blank line between the two includes so include sorting cannot swap them.
- `include/queue.h` defines the version macros, and `meson setup` fails when they differ from `project(version:)`, so a release updates both. The build installs `libqueue.a`, headers under `queue/`, and `queue.pc`. The only Meson option is `build_tests`.
- `misra.toml` defines `SIZE_MAX` for the analysis, because cppcheck does not read the system `<stdint.h>` and `queue.h` stops at `#error` without it.

---

## 2) Non-negotiable design invariants

- **SPSC indices.** The producer publishes `head` with a release store after copying the item; the consumer publishes `tail` with a release store after copying it out. Each reads the other index with an acquire load. Keep this pairing in every backend.
- **Lock-free fail-on-full.** In fail-on-full mode the producer writes only `head` and the consumer only `tail`. Only overwrite mode may write `tail` from the producer, and it relies on the critical-section hooks.
- **Backend ladder.** `__atomic`, then `<stdatomic.h>`, then volatile, overridable by exactly one `QUEUE_USE_*` macro. The atomic backends keep their lock-free `_Static_assert`s.
- **One spare slot.** The ring holds `capacity + 1` items; empty is `head == tail`, full is `next(head) == tail`. No shared count.
- **Hooks wrap enqueue and dequeue only.** `init`, `clear` and the queries never run them. The overwrite-mode warning stays in place for GCC, Clang and TI compilers.
- Constant time, no loops, no division, no heap, no global state; `memcpy` is the only library call.

---

## 3) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs (see §1.1 above).
- Install Python tools (Meson, Ninja, gcovr, misch) with `uv tool install`, never `pip`; CI does the same through `astral-sh/setup-uv`. CI installs `cppcheck` for the MISRA job from Homebrew because Ubuntu's 2.13 reports different findings.
- `misch run --baseline` must pass against `analysis/baseline/` for every backend profile. Never regenerate a baseline to admit a new finding. Run it with `MISRA_RULE_TEXTS` unset, as CI does.
- The CI jobs are: tests under ASan/UBSan, ThreadSanitizer, a release build, a 100 percent line and branch coverage gate, a TI C2000 build (CGT 22.6.2 and 25.11.1, COFF and EABI, the library and `tests/test_queue_wrap.c` in both full policies, warnings as errors), `clang-format`, and `misch` for each backend.

---

## 4) Docs / commit conventions

### Documentation layout

- `README.md` is a short landing page (under 100 lines). Put detail in `docs/`, never in the README; link to the owning file instead.
- `docs/integration.md` owns requirements, install methods, configuration, building, and cross builds.
- `docs/api.md` owns the API reference, the status codes, the configuration macros and recipes. Change it together with the headers; keep signatures in fenced `c` blocks and describe them in prose, not function tables. Check every value an example states against a build.
- `docs/design.md` is the single authoritative design record: the ring, the atomic backends and memory ordering, the full policies and hooks, the overwrite-mode warning, concurrency, worst-case cost, footprint, failure behaviour, MISRA position, and verification.
- `CONTRIBUTING.md` documents the contributor workflow; `CHANGELOG.md` holds release history (Keep a Changelog); `.github/SECURITY.md` covers vulnerability reporting.
- Code blocks in `README.md`, `CONTRIBUTING.md`, and `docs/` are compiled by the `queue docs examples` test (`tests/docs/check_snippets.py`). Each document is compiled as one source file and includes the headers itself, so its first block sets the configuration. A `c` fence is file-level code, `<!-- snippet: body -->` wraps statements, and `<!-- snippet: skip -->` excludes pseudo-code or a copy of a definition. Do not use `skip` to silence a broken example.
- Prose uses British spelling. Do not hard-wrap Markdown: write one line per paragraph. The 80-column limit applies to C source, not Markdown.

### Commits

- Use **Conventional Commits** format when asked to commit.
- Keep commits focused; explain *why* in the message body.

---

## 5) C style expectations

### Build & configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- Update `meson.build` and `tests/meson.build` when adding or removing files.
- Every configuration macro in `queue_conf.h` stays behind an `#ifndef` guard.

### Formatting

- `.clang-format` is present and **mandatory**. Run `scripts/format.sh --fix` before committing. It runs the clang-format release pinned in the script through `uvx`. Do not use a system `clang-format`: another release can format the same code differently.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Style & correctness

- Match conventions in the existing files (indentation, braces, naming).
- Every public identifier uses the `queue_` or `QUEUE_` prefix; internal ones use `queue__` or `QUEUE__`.
- Validate pointer arguments at every public API boundary and return `QUEUE_STATUS_BAD_ARG`; no `errno`.
- Use fixed-width types from `<stdint.h>` and `size_t` for indices, so the code builds where `CHAR_BIT` is 16. Tests that must build for that target avoid `uint8_t`.
- A change to the layout of a generated queue type needs a `CHANGELOG.md` entry.

### MISRA deviations

- Prefer compliant code; never restructure only to hide a finding. Deviate at the point of use with `/* cppcheck-suppress misra-c2012-<rule> ; @deviation <rationale> */`, or project-wide in `analysis/deviations/misra-deviations.txt` for a house-style rule.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on `enum`/`struct` members are allowed only when the resulting line fits the 80-column limit.
- If any member's annotation would overrun, move **all** of that aggregate's member docs into a single structured Doxygen block above the type, as an `@details` list of `- ::SYMBOL  description` entries.
- Never mix inline and block forms within one aggregate, and never leave a trailing comment that clang-format would wrap onto a second line.
- After editing, run `scripts/format.sh` and an 80-column scan.

### Testing

- Run `meson test -C build` after every change; all tests must pass, and the concurrent tests must also pass under ThreadSanitizer.
- Tests live in `tests/test_*.c`, one executable each. Do not add Unity or another external framework.
- A change to the index ordering needs a test with a real producer and consumer thread for each backend it touches.
- Add a test case for each bug fix and each new feature.
- Keep `src/` and `include/` at the coverage gate (100 percent line and branch).
