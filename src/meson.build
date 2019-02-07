libjsonrpc_glib_header_subdir = join_paths(package_subdir, 'jsonrpc-glib-' + apiversion)
libjsonrpc_glib_header_dir = join_paths(get_option('includedir'), libjsonrpc_glib_header_subdir)

version_data = configuration_data()
version_data.set('MAJOR_VERSION', jsonrpc_glib_version_major)
version_data.set('MINOR_VERSION', jsonrpc_glib_version_minor)
version_data.set('MICRO_VERSION', jsonrpc_glib_version_micro)
version_data.set('VERSION', meson.project_version())

jsonrpc_version_h = configure_file(
          input: 'jsonrpc-version.h.in',
         output: 'jsonrpc-version.h',
    install_dir: libjsonrpc_glib_header_dir,
        install: true,
  configuration: version_data)

libjsonrpc_glib_generated_headers = [
  jsonrpc_version_h,
]

libjsonrpc_glib_public_headers = [
  'jsonrpc-client.h',
  'jsonrpc-glib.h',
  'jsonrpc-input-stream-private.h',
  'jsonrpc-input-stream.h',
  'jsonrpc-message.h',
  'jsonrpc-output-stream.h',
  'jsonrpc-server.h',
  'jsonrpc-version-macros.h',
]

libjsonrpc_glib_public_sources = [
  'jsonrpc-client.c',
  'jsonrpc-input-stream.c',
  'jsonrpc-message.c',
  'jsonrpc-output-stream.c',
  'jsonrpc-server.c',
]

libjsonrpc_glib_deps = [
  dependency('gio-2.0'),
  dependency('json-glib-1.0'),
]

libjsonrpc_glib_sources = [
  libjsonrpc_glib_generated_headers,
  libjsonrpc_glib_public_headers,
  libjsonrpc_glib_public_sources,
]

libjsonrpc_glib = library(
  'jsonrpc-glib-' + apiversion,
  libjsonrpc_glib_sources,

        c_args: hidden_visibility_args + release_args,
  dependencies: libjsonrpc_glib_deps,
     soversion: soversion,
       version: libversion,
       install: true,
)

libjsonrpc_glib_dep = declare_dependency(
              sources: libjsonrpc_glib_generated_headers,
         dependencies: libjsonrpc_glib_deps,
            link_with: libjsonrpc_glib,
  include_directories: include_directories('.'),
)

if get_option('with_introspection')

  libjsonrpc_glib_gir = gnome.generate_gir(libjsonrpc_glib,
                sources: libjsonrpc_glib_generated_headers + libjsonrpc_glib_public_headers + libjsonrpc_glib_public_sources,
              nsversion: apiversion,
              namespace: 'Jsonrpc',
          symbol_prefix: 'jsonrpc',
      identifier_prefix: 'Jsonrpc',
              link_with: libjsonrpc_glib,
               includes: [ 'Gio-2.0', 'Json-1.0' ],
                install: true,
        install_dir_gir: girdir,
    install_dir_typelib: typelibdir,
             extra_args: [
                 '--c-include=jsonrpc-glib.h',
                 '-DJSONRPC_GLIB_COMPILATION',
             ],
  )

  if get_option('with_vapi')

    libjsonrpc_glib_vapi = gnome.generate_vapi('jsonrpc-glib-' + apiversion,
          sources: [libjsonrpc_glib_gir[0], 'Jsonrpc-' + apiversion + '-custom.vala'],
         packages: [ 'gio-2.0', 'json-glib-1.0' ],
          install: true,
      install_dir: vapidir,
    )

  endif
endif

install_headers(libjsonrpc_glib_public_headers, subdir: libjsonrpc_glib_header_subdir)

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libjsonrpc_glib],
      subdirs: libjsonrpc_glib_header_subdir,
      version: meson.project_version(),
         name: 'Jsonrpc-GLib',
     filebase: 'jsonrpc-glib-@0@'.format(apiversion),
  description: 'A jsonrpc library for GLib',
     requires: [ 'gio-2.0', 'json-glib-1.0' ],
  install_dir: join_paths(libdir, 'pkgconfig'),
)

