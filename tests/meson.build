queue_test_overwrite_exe = executable(
  'queue_test_overwrite',
  'test_queue.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)

queue_test_clear_exe = executable(
  'queue_test_clear',
  'test_queue_clear.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_clear', queue_test_clear_exe)

queue_test_fail_exe = executable(
  'queue_test_fail',
  'test_queue_fail.c',
  include_directories: public_headers,
  link_with: [queue_lib],
  c_args: ['-DQUEUE_OVERWRITE_ON_FULL=0'],
)

test('queue_test_overwrite', queue_test_overwrite_exe)
test('queue_test_fail', queue_test_fail_exe)

queue_test_soak_exe = executable(
  'queue_test_soak',
  'test_queue_soak.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_soak', queue_test_soak_exe)

thread_dep = dependency('threads')
queue_test_mt_exe = executable(
  'queue_test_mt',
  'test_queue_mt.c',
  include_directories: public_headers,
  dependencies: [thread_dep],
  link_with: [queue_lib],
)
test('queue_test_mt', queue_test_mt_exe)

queue_test_overwrite_mt_exe = executable(
  'queue_test_overwrite_mt',
  'test_queue_overwrite_mt.c',
  include_directories: public_headers,
  dependencies: [thread_dep],
  link_with: [queue_lib],
)
test('queue_test_overwrite_mt', queue_test_overwrite_mt_exe)

# Additional tests for coverage
wrap_exe = executable(
  'queue_test_wrap',
  'test_queue_wrap.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_wrap', wrap_exe)

fail_mode_exe = executable(
  'queue_test_fail_mode',
  'test_queue_fail_mode.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_fail_mode', fail_mode_exe)

critical_exe = executable(
  'queue_test_critical',
  'test_queue_critical.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_critical', critical_exe)

safety_exe = executable(
  'queue_test_safety',
  'test_queue_safety.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_safety', safety_exe)

count_mt_exe = executable(
  'queue_test_count_mt',
  'test_queue_count_mt.c',
  include_directories: public_headers,
  dependencies: [thread_dep],
  link_with: [queue_lib],
)
test('queue_test_count_mt', count_mt_exe)

queue_test_capacity_1_exe = executable(
  'queue_test_capacity_1',
  'test_queue_capacity_1.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_capacity_1', queue_test_capacity_1_exe)

queue_test_immediate_dequeue_exe = executable(
  'queue_test_immediate_dequeue',
  'test_queue_immediate_dequeue.c',
  include_directories: public_headers,
  link_with: [queue_lib],
  c_args: ['-DQUEUE_OVERWRITE_ON_FULL=0'],
)
test('queue_test_immediate_dequeue', queue_test_immediate_dequeue_exe)

queue_test_exact_capacity_exe = executable(
  'queue_test_exact_capacity',
  'test_queue_exact_capacity.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_exact_capacity', queue_test_exact_capacity_exe)

queue_test_empty_after_cycles_exe = executable(
  'queue_test_empty_after_cycles',
  'test_queue_empty_after_cycles.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_empty_after_cycles', queue_test_empty_after_cycles_exe)

queue_test_null_validation_exe = executable(
  'queue_test_null_validation',
  'test_queue_null_validation.c',
  include_directories: public_headers,
  link_with: [queue_lib],
)
test('queue_test_null_validation', queue_test_null_validation_exe)
