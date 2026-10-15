# SPDX-License-Identifier: MIT

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

# The default backend on GCC/Clang is __atomic. These force the C11 backend
# for the concurrent tests and the volatile backend for single-threaded ones,
# so every backend and its lock-free assertion is compiled and exercised.
foreach t : [
  ['queue_test_mt_c11', 'test_queue_mt.c', '-DQUEUE_USE_C11_ATOMICS=1', true],
  ['queue_test_count_mt_c11', 'test_queue_count_mt.c', '-DQUEUE_USE_C11_ATOMICS=1', true],
  ['queue_test_no_atomics', 'test_queue.c', '-DQUEUE_USE_NO_ATOMICS=1', false],
  ['queue_test_fail_no_atomics', 'test_queue_fail.c', ['-DQUEUE_USE_NO_ATOMICS=1', '-DQUEUE_OVERWRITE_ON_FULL=0'], false],
]
  exe = executable(
    t[0],
    t[1],
    include_directories: public_headers,
    dependencies: t[3] ? [thread_dep] : [],
    link_with: [queue_lib],
    c_args: t[2],
  )
  test(t[0], exe)
endforeach

# A consumer-defined QUEUE__UNUSED must not trigger a redefinition warning.
queue_test_unused_override_exe = executable(
  'queue_test_unused_override',
  'test_queue_unused_override.c',
  include_directories: public_headers,
  link_with: [queue_lib],
  c_args: ['-Werror', '-Wunused-function'],
)
test('queue_test_unused_override', queue_test_unused_override_exe)

# Every C code block in the README, CONTRIBUTING.md and docs/ compiles against
# the public headers, so a documented signature cannot drift from the code.
# Each document includes the headers itself, because a block may configure
# the queue before queue_conf.h.
python = import('python').find_installation('python3')

test(
  'queue docs examples',
  python,
  args: [
    files('docs/check_snippets.py'),
    '--root', meson.project_source_root(),
    '--lang', 'c',
    '--',
  ] + meson.get_compiler('c').cmd_array() + [
    '-std=c11',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Werror',
    '-Wno-unused-function',
    '-Wno-unused-variable',
    '-I' + (meson.project_source_root() / 'include'),
    '-c', '{source}', '-o', '{object}',
  ],
  suite: 'docs',
  timeout: 60,
)

# Overwrite mode without critical-section hooks must warn unless the consumer
# declares a single execution context, selects fail-on-full or supplies hooks.
cc = meson.get_compiler('c')
if cc.get_id() in ['gcc', 'clang']
  # Clang reports #pragma GCC warning in a group that -Werror leaves alone.
  diag_werror = cc.get_id() == 'clang' ? ['-Werror=#pragma-messages'] : ['-Werror']
  overwrite_diag_src = '''
#include "queue_conf.h"
#include "queue.h"
int main(void) { return 0; }
'''
  assert(
    not cc.compiles(overwrite_diag_src, args: diag_werror, include_directories: public_headers),
    'overwrite mode without critical-section hooks must warn',
  )
  foreach opt_out : [
    ['-DQUEUE_SINGLE_CONTEXT=1'],
    ['-DQUEUE_OVERWRITE_ON_FULL=0'],
    ['-DQUEUE_ENTER_CRITICAL()=(void)0', '-DQUEUE_EXIT_CRITICAL()=(void)0'],
  ]
    assert(
      cc.compiles(overwrite_diag_src, args: diag_werror + opt_out, include_directories: public_headers),
      'overwrite diagnostic must stay silent with ' + ' '.join(opt_out),
    )
  endforeach
endif
