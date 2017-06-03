subdir('xml')

private_headers = [
  'config.h',
  'jsonrpc-glib.h',
  'jsonrpc-version.h',
]

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
json_prefix = dependency('json-glib-1.0').get_pkgconfig_variable('prefix')
json_docpath = join_paths(json_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

gnome.gtkdoc('jsonrpc-glib',
           main_xml: 'jsonrpc-glib-docs.sgml',
            src_dir: [
              join_paths(meson.source_root(), 'src'),
              join_paths(meson.build_root(), 'src'),
            ],

       dependencies: libjsonrpc_glib_dep,
  gobject_typesfile: 'jsonrpc-glib.types',
          scan_args: [
            '--rebuild-types',
            '--ignore-decorators=_JSONRPC_EXTERN',
            '--ignore-headers=' + ' '.join(private_headers),
          ],

       fixxref_args: [
         '--html-dir=@0@'.format(docpath),
         '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
         '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
         '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
         '--extra-dir=@0@'.format(json_docpath),
       ],
            install: true)

