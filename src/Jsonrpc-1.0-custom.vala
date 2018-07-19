namespace Jsonrpc {
	namespace Message {
		public struct GetBoolean {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_BOOLEAN")]
			public static void* create (ref bool val);
		}
		public struct GetDict {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_DICT")]
			public static void* create (ref GLib.VariantDict val);
		}
		public struct GetDouble {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_DOUBLE")]
			public static void* create (ref double val);
		}
		public struct GetInt32 {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_INT32")]
			public static void* create (ref int32 val);
		}
		public struct GetInt64 {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_INT64")]
			public static void* create (ref int64 val);
		}
		public struct GetIter {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_ITER")]
			public static void* create (ref GLib.VariantIter val);
		}
		public struct GetString {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_STRING")]
			public static void* create (ref unowned string val);
		}
		public struct GetStrv {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_STRV")]
			public static void* create ([CCode (array_length = false)] ref string[] val);
		}
		public struct GetVariant {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_GET_VARIANT")]
			public static void* create (ref GLib.Variant val);
		}
		public struct PutBoolean {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_BOOLEAN")]
			public static void* create (bool val);
		}
		public struct PutDouble {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_DOUBLE")]
			public static void* create (double val);
		}
		public struct PutInt32 {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_INT32")]
			public static void* create (int32 val);
		}
		public struct PutInt64 {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_INT64")]
			public static void* create (int64 val);
		}
		public struct PutString {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_STRING")]
			public static void* create (string val);
		}
		public struct PutStrv {
			[CCode (cheader_filename = "jsonrpc-glib.h", cname = "JSONRPC_MESSAGE_PUT_STRV")]
			public static void* create ([CCode (array_length = false)] string[] val);
		}
	}
}
