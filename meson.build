project('jsonrpc-glib', 'c',
          version: '3.25.3',
          license: 'LGPLv2.1+',
    meson_version: '>= 0.38.1',
  default_options: [ 'warning_level=1', 'buildtype=debugoptimized' ],
)

version_arr = meson.project_version().split('.')
jsonrpc_glib_version_major = version_arr[0].to_int()
jsonrpc_glib_version_minor = version_arr[1].to_int()
jsonrpc_glib_version_micro = version_arr[2].to_int()

apiversion = '1.0'
soversion = 0

if jsonrpc_glib_version_minor.is_odd()
  jsonrpc_glib_interface_age = 0
else
  jsonrpc_glib_interface_age = jsonrpc_glib_version_micro
endif

# maintaining compatibility with the previous libtool versioning
# current = minor * 100 + micro - interface
# revision = interface
current = jsonrpc_glib_version_minor * 100 + jsonrpc_glib_version_micro - jsonrpc_glib_interface_age
revision = jsonrpc_glib_interface_age
libversion = '@0@.@1@.@2@'.format(soversion, current, revision)

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', 'libjsonrpc_glib')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))

configure_file(
         output: 'config.h',
  configuration: config_h,
)

add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-I' + meson.build_root(),
  '-I' + join_paths(meson.source_root(), 'src'),
  '-DJSONRPC_GLIB_COMPILATION',
], language: 'c')

cc = meson.get_compiler('c')

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wmissing-include-dirs',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-uninitialized',
  '-Wno-unused-parameter',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif
if get_option('enable_profiling')
  test_c_args += '-pg'
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  global_c_args,
  language: 'c'
)

# Check if we can use version scripts for ABI exports
ld_supports_version_script = cc.links('''
  int main (void) { return 0; }
''', args: '-Wl,--version-script,' + join_paths(meson.source_root(), 'src/jsonrpc-glib.map'))
message('Linker supports --version-script: @0@'.format(ld_supports_version_script))

# Setup various paths that subdirectory meson.build files need
package_subdir = get_option('package_subdir')
girdir = join_paths(get_option('datadir'), package_subdir, 'gir-1.0')
typelibdir = join_paths(get_option('libdir'), package_subdir, 'girepository-1.0')
libdir = join_paths(get_option('libdir'), package_subdir)
if package_subdir != ''
  vapidir = join_paths(get_option('datadir'), package_subdir, 'vapi')
else
  vapidir = join_paths(get_option('datadir'), 'vala', 'vapi')
endif

gnome = import('gnome')

subdir('src')
subdir('tests')

if get_option('enable_gtk_doc')
  subdir('doc')
endif
