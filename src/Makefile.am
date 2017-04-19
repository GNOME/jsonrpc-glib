lib_LTLIBRARIES = libjsonrpc-glib-@API_VERSION@.la
noinst_PROGRAMS =


TESTS =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =
LOG_COMPILER = $(top_srcdir)/build-aux/tap-test


headersdir = $(includedir)/jsonrpc-glib-@API_VERSION@
nobase_headers_DATA = \
	jsonrpc-client.h \
	jsonrpc-glib.h \
	jsonrpc-input-stream.h \
	jsonrpc-message.h \
	jsonrpc-output-stream.h \
	jsonrpc-server.h \
	jsonrpc-version.h \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_SOURCES = \
	$(nobase_headers_DATA) \
	jsonrpc-client.c \
	jsonrpc-input-stream.c \
	jsonrpc-input-stream-private.h \
	jsonrpc-message.c \
	jsonrpc-output-stream.c \
	jsonrpc-server.c \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_CFLAGS = \
	-DJSONRPC_GLIB_INSIDE \
	$(JSONRPC_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_LIBADD = \
	$(JSONRPC_LIBS) \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(NULL)


noinst_PROGRAMS += test-message
TESTS += test-message
test_message_CFLAGS = $(JSONRPC_CFLAGS)
test_message_LDADD = \
	$(JSONRPC_LIBS) \
	libjsonrpc-glib-@API_VERSION@.la \
	$(NULL)

noinst_PROGRAMS += test-server
TESTS += test-server
test_server_CFLAGS = $(JSONRPC_CFLAGS)
test_server_LDADD = \
	$(JSONRPC_LIBS) \
	libjsonrpc-glib-@API_VERSION@.la \
	$(NULL)

noinst_PROGRAMS += test-stress
TESTS += test-stress
test_stress_CFLAGS = $(JSONRPC_CFLAGS)
test_stress_LDADD = \
	$(JSONRPC_LIBS) \
	libjsonrpc-glib-@API_VERSION@.la \
	$(NULL)

# Tests against the Rust Language Server "rls" tool.
noinst_PROGRAMS += test-client
test_client_CFLAGS = $(JSONRPC_CFLAGS)
test_client_LDADD = \
	$(JSONRPC_LIBS) \
	libjsonrpc-glib-@API_VERSION@.la \
	$(NULL)


if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

introspection_sources = \
	jsonrpc-client.c \
	jsonrpc-client.h \
	jsonrpc-server.c \
	jsonrpc-server.h \
	jsonrpc-input-stream.c \
	jsonrpc-input-stream.h \
	jsonrpc-output-stream.c \
	jsonrpc-output-stream.h \
	jsonrpc-version.h \
	$(NULL)

Jsonrpc-@API_VERSION@.gir: libjsonrpc-glib-@API_VERSION@.la
Jsonrpc_@API_VERSION@_gir_INCLUDES = Gio-2.0
Jsonrpc_@API_VERSION@_gir_CFLAGS = $(libjsonrpc_glib_@API_VERSION@_la_CFLAGS)
Jsonrpc_@API_VERSION@_gir_LIBS = libjsonrpc-glib-@API_VERSION@.la
Jsonrpc_@API_VERSION@_gir_FILES = $(introspection_sources)
Jsonrpc_@API_VERSION@_gir_SCANNERFLAGS = \
	-n Jsonrpc \
	--c-include "jsonrpc-glib.h" \
	--identifier-prefix Jsonrpc \
	--symbol-prefix jsonrpc \
	-DJSONRPC_GLIB_COMPILATION \
	$(NULL)
INTROSPECTION_GIRS += Jsonrpc-@API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif


if ENABLE_VAPIGEN
-include $(VAPIGEN_MAKEFILE)

jsonrpc-glib-@API_VERSION@.vapi: Jsonrpc-@API_VERSION@.gir

VAPIGEN_VAPIS = jsonrpc-glib-@API_VERSION@.vapi

jsonrpc_glib_@API_VERSION@_vapi_DEPS = gio-2.0 json-glib-1.0
jsonrpc_glib_@API_VERSION@_vapi_METADATADIRS = $(srcdir)
jsonrpc_glib_@API_VERSION@_vapi_FILES = Jsonrpc-@API_VERSION@.gir

jsonrpc-glib-@API_VERSION@.deps: Makefile
	$(AM_V_GEN) echo $(libjsonrpc_glib_@API_VERSION@_vapi_DEPS) | tr ' ' '\n' > $@

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)

EXTRA_DIST += jsonrpc-glib-@API_VERSION@.deps

DISTCLEANFILES += $(vapi_DATA)
endif

-include $(top_srcdir)/git.mk
