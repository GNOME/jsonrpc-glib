if get_option('enable_gtk_doc') and not get_option('with_introspection')
  error('API reference requires introspection.')
endif

toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

gidocgen = find_program('gi-docgen', required: get_option('enable_gtk_doc'))

gidocgen_common_args = [
  '--quiet',
  '--no-namespace-dir',
]

if get_option('werror')
  gidocgen_common_args += ['--fatal-warnings']
endif

docs_dir = datadir / 'doc'

if get_option('enable_gtk_doc')
  source_toml = configure_file(
    input: 'Jsonrpc.toml.in',
    output: 'Jsonrpc.toml',
    configuration: toml_conf,
    install: true,
    install_dir: docs_dir / 'jsonrpc-glib',
  )

  custom_target('jsonrpc-glib-doc',
    input: [ source_toml, libjsonrpc_glib_gir[0] ],
    output: 'jsonrpc-glib',
    command: [
      gidocgen,
      'generate',
      gidocgen_common_args,
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
  )
endif
