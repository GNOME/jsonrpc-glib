lib_LTLIBRARIES = libjsonrpc-glib-@API_VERSION@.la

libjsonrpc_glib_@API_VERSION@_la_SOURCES = \
	jsonrpc-input-stream.c \
	jsonrpc-input-stream.h \
	jsonrpc-output-stream.c \
	jsonrpc-output-stream.h \
	jsonrpc-client.c \
	jsonrpc-client.h \
	jsonrpc-version.h \
	jsonrpc-glib.h \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_CFLAGS = \
	$(JSONRPC_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_LIBADD = \
	$(JSONRPC_LIBS) \
	$(NULL)

libjsonrpc_glib_@API_VERSION@_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(NULL)

-include $(top_srcdir)/git.mk
