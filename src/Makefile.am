lib_LTLIBRARIES = libjsonrpc-glib-@API_VERSION@.la
noinst_PROGRAMS =
TESTS =
LOG_COMPILER = $(top_srcdir)/build-aux/tap-test

headersdir = $(includedir)/jsonrpc-glib-@API_VERSION@
nobase_headers_DATA = \
	jsonrpc-client.h \
	jsonrpc-glib.h \
	jsonrpc-input-stream.h \
	jsonrpc-output-stream.h \
	jsonrpc-version.h \
	jcon.h \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_SOURCES = \
	$(nobase_headers_DATA) \
	jsonrpc-client.c \
	jsonrpc-input-stream.c \
	jsonrpc-output-stream.c \
	jcon.c \
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

noinst_PROGRAMS += test-jcon
TESTS += test-jcon
test_jcon_CFLAGS = $(JSONRPC_CFLAGS)
test_jcon_LDADD = \
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

-include $(top_srcdir)/git.mk
