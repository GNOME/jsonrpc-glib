if get_option('enable_tests')

test_env = [
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  'G_DEBUG=gc-friendly',
  'GSETTINGS_BACKEND=memory',
  'PYTHONDONTWRITEBYTECODE=yes',
  'MALLOC_CHECK_=2',
#  'MALLOC_PERTURB_=$((${RANDOM:-256} % 256))',
]

test_cflags = [
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
]

test_link_args = [
]

test_deps = libjsonrpc_glib_deps + [
  libjsonrpc_glib_dep,
  dependency('gio-unix-2.0'),
]

test_message = executable('test-message', 'test-message.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)
test('test-message', test_message, env: test_env)

test_server = executable('test-server', 'test-server.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)
test('test-server', test_server, env: test_env)

test_stress = executable('test-stress', 'test-stress.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)
test('test-stress', test_stress, env: test_env)

test_gauntlet = executable('test-gauntlet', 'test-gauntlet.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)
test('test-gauntlet', test_gauntlet, env: test_env)

endif
