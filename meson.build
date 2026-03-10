project(
  'queue',
  'c',
  version: '1.2.4',
  default_options: ['c_std=c11', 'warning_level=2'],
  license: 'MIT',
)

ver = meson.project_version().split('.')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())

major = ver[0].to_int()
minor = ver[1].to_int()
patch = ver[2].to_int()

conf.set('MAJOR', major)
conf.set('MINOR', minor)
conf.set('PATCH', patch)

configure_file(
  input: 'config/queue_version.h.in',
  output: 'queue_version.h',
  configuration: conf,
)

queue_inc = include_directories('include', 'config')
queue_src = ['src/queue.c']

queue_lib = both_libraries(
  'queue',
  queue_src,
  version: meson.project_version(),
  soversion: major,
  include_directories: queue_inc,
)

queue_dep = declare_dependency(include_directories: queue_inc, link_with: queue_lib)

if get_option('build_tests')
  subdir('tests')
endif
