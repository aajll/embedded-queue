# SPDX-License-Identifier: MIT

project(
  'queue',
  'c',
  version: '1.3.0',
  meson_version: '>=0.57.0',
  default_options: ['c_std=c11', 'warning_level=3'],
  license: 'MIT',
)

# === Version ==================================================================
# include/queue.h defines the version macros, so a copy-in build has them.
# Configuration fails when they differ from the project version.

fs = import('fs')
version_parts = meson.project_version().split('.')
version_macros = {
  'QUEUE_VERSION_MAJOR': version_parts[0],
  'QUEUE_VERSION_MINOR': version_parts[1],
  'QUEUE_VERSION_PATCH': version_parts[2],
  'QUEUE_VERSION': '"' + meson.project_version() + '"',
}
header_lines = []
foreach line : fs.read('include' / 'queue.h').split('\n')
  header_lines += ' '.join(line.split())
endforeach
foreach name, value : version_macros
  if '#define @0@ @1@'.format(name, value) not in header_lines
    error('include/queue.h must define @0@ as @1@'.format(name, value))
  endif
endforeach

# === Include directories ======================================================

public_headers = include_directories('include')

# === Application core library =================================================

library_sources = ['src/queue.c']

queue_lib = static_library(
  'queue',
  library_sources,
  include_directories: public_headers,
  install: true,
  install_dir: get_option('libdir'),
)

# Install public headers
install_headers(
  ['include/queue.h', 'include/queue_conf.h'],
  subdir: 'queue',
)

queue_dep = declare_dependency(
  include_directories: public_headers,
  link_with: queue_lib,
)

# Registered under the pkg-config name and the repository name, so any project
# in the build finds queue_dep through either without a fallback.
meson.override_dependency('queue', queue_dep)
meson.override_dependency('embedded-queue', queue_dep)

# === pkg-config ===============================================================

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  queue_lib,
  name: 'queue',
  description: 'Deterministic, type-safe SPSC ring-buffer queue in C11 for embedded and safety-critical systems',
  subdirs: 'queue',
)

# === Host unit tests ==========================================================

if get_option('build_tests')
  subdir('tests')
endif
