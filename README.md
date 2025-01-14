# Jsonrpc-GLib

Jsonrpc-GLib is a library to communicate with JSON-RPC based peers in either a synchronous or asynchronous fashion.
It also allows communicating using the GVariant serialization format instead of JSON when both peers support it.
You might want that when communicating on a single host to avoid parser overhead and memory-allocator fragmentation.

## Building

We use the Meson build system for building jsonrpc-glib.

```sh
# Some common options
meson --prefix=/opt/gnome --libdir=/opt/gnome/lib -Denable_gtk_doc=true build
cd build
ninja install
```

## Documentation

Nightly documentations can be found at https://gnome.pages.gitlab.gnome.org/jsonrpc-glib/jsonrpc-glib/
