# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Added changelog, contributor guide, security policy, standard CI workflow, installed version header, pkg-config metadata, README badge, SPDX headers, and registration for the previously skipped `test_queue_clear` test.

### Changed

- Renamed `LICENCE` to `LICENSE`, standardised copyright text, switched the library target to `static_library`, installed public headers, and aligned Meson defaults with the primitive family baseline.

### Fixed

- `queue_version.h` now defines `QUEUE_VERSION` as a single string literal and `QUEUE_VERSION_MAJOR`, `_MINOR` and `_PATCH` as integers, so the macros compile when used.

## [1.2.4] - 2026-03-10

### Fixed

- Corrected the Meson dependency export typo.

## [1.2.3] - 2026-03-10

### Changed

- Improved README formatting and content.
