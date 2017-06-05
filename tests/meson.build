if get_option('enable-tests')

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
  '-fPIE',
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
]

test_link_args = [
  '-fPIC',
]

test_deps = libjsonrpc_glib_deps + [
  libjsonrpc_glib_dep,
  dependency('gio-unix-2.0'),
]

test_client = executable('test-client', 'test-client.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)

test_message = executable('test-message', 'test-message.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)

test_server = executable('test-server', 'test-server.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)

test_stress = executable('test-stress', 'test-stress.c',
        c_args: test_cflags,
     link_args: test_link_args,
  dependencies: test_deps,
)

endif
