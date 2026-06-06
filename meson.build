project(
  'queue',
  'c',
  version: '1.2.6',
  default_options: ['c_std=c11', 'warning_level=3'],
  license: 'MIT',
)

# === Version header ===========================================================

ver = meson.project_version().split('.')

major = ver[0].to_int()
minor = ver[1].to_int()
patch = ver[2].to_int()

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('MAJOR', major)
conf.set('MINOR', minor)
conf.set('PATCH', patch)

configure_file(
  input: 'config' / 'queue_version.h.in',
  output: 'queue_version.h',
  configuration: conf,
  install: true,
  install_dir: get_option('includedir') / 'queue',
)

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
