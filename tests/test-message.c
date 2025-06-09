#include <json-glib/json-glib.h>

#include "jsonrpc-message.h"

static void
test_basic (void)
{
  g_autoptr(GVariant) node = NULL;
  g_autoptr(JsonParser) parser = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) deserialized = NULL;
  const gchar *foo1 = NULL;
  gint64 baz_baz_baz = 0;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW (
    "foo", "foo1",
    "bar", "foo2",
    "baz", "{",
      "baz", "[", "{", "baz", JSONRPC_MESSAGE_PUT_INT64 (123), "}", "]",
    "}"
  );

  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node,
    "foo", JSONRPC_MESSAGE_GET_STRING (&foo1),
    "baz", "{",
      "baz", "[", "{", "baz", JSONRPC_MESSAGE_GET_INT64 (&baz_baz_baz), "}", "]",
    "}"
  );

  g_assert_cmpstr (foo1, ==, "foo1");
  g_assert_cmpint (baz_baz_baz, ==, 123);
  g_assert_cmpint (r, ==, 1);

  /* compare json gvariant encoding to ensure it matches */
#define TESTSTR "{\"foo\": \"foo1\", \"bar\": \"foo2\", \"baz\": {\"baz\": [{\"baz\": 123}]}}"
  parser = json_parser_new ();
  r = json_parser_load_from_data (parser, TESTSTR, -1, &error);
  g_assert_true (r);
  g_assert_no_error (error);
  deserialized = json_gvariant_deserialize (json_parser_get_root (parser), NULL, &error);
  g_assert_true (deserialized);
  g_assert_no_error (error);
  g_assert_true (g_variant_equal (deserialized, node));
#undef TESTSTR
}

static void
test_deep_array (void)
{
  g_autoptr(GVariant) node = NULL;
  const gchar *abc = NULL;
  const gchar *xyz = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW ("foo", "[","[","[","[","[","[","[","[","[","[", "abc", "]", "]","]","]","]","]","]","]","]","]");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", "[","[","[","[","[","[","[","[","[","[", JSONRPC_MESSAGE_GET_STRING (&abc), "]", "]","]","]","]","]","]","]","]","]");
  g_assert_cmpstr (abc, ==, "abc");
  g_assert_cmpint (r, ==, 1);

  g_clear_pointer (&node, g_variant_unref);

  node = JSONRPC_MESSAGE_NEW ("foo", "[","[","[","[","[","[","[","[","[","{", "foo", "xyz", "}", "]","]","]","]","]","]","]","]","]");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", "[","[","[","[","[","[","[","[","[","{", "foo", JSONRPC_MESSAGE_GET_STRING (&xyz), "}", "]","]","]","]","]","]","]","]","]");
  g_assert_cmpstr (xyz, ==, "xyz");
  g_assert_cmpint (r, ==, 1);
}

static void
test_extract_array (void)
{
  g_autoptr(GVariant) node = NULL;
  g_autoptr(GVariant) ar123 = NULL;
  gboolean r;
  gint32 a=0, b=0, c=0;

  node = JSONRPC_MESSAGE_NEW ("foo", "[", JSONRPC_MESSAGE_PUT_INT32 (1), JSONRPC_MESSAGE_PUT_INT32 (2), JSONRPC_MESSAGE_PUT_INT32 (3), "]");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", JSONRPC_MESSAGE_GET_VARIANT (&ar123));
  g_assert_nonnull (ar123);
  g_assert_cmpint (r, ==, 1);
  g_assert_cmpint (3, ==, g_variant_n_children (ar123));

  r = JSONRPC_MESSAGE_PARSE (node, "foo", "[", JSONRPC_MESSAGE_GET_INT32 (&a), JSONRPC_MESSAGE_GET_INT32 (&b), JSONRPC_MESSAGE_GET_INT32 (&c), "]");
  g_assert_cmpint (r, ==, 1);
  g_assert_cmpint (a, ==, 1);
  g_assert_cmpint (b, ==, 2);
  g_assert_cmpint (c, ==, 3);
}

static void
test_extract_object (void)
{
  g_autoptr(GVariant) node = NULL;
  g_autoptr(GVariantDict) dict = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW ("foo", "{", "bar", "[", JSONRPC_MESSAGE_PUT_INT32 (1), "two", JSONRPC_MESSAGE_PUT_INT32 (3), "]", "}");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", JSONRPC_MESSAGE_GET_DICT (&dict));
  g_assert_nonnull (dict);
  g_assert_true (g_variant_dict_contains (dict, "bar"));
  g_assert_cmpint (r, ==, TRUE);
}

static void
test_extract_node (void)
{
  g_autoptr(GVariant) node = NULL;
  g_autoptr(GVariant) node2 = NULL;
  g_autoptr(GVariant) ar = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW ("foo", "{", "bar", "[", JSONRPC_MESSAGE_PUT_INT32 (1), "two", JSONRPC_MESSAGE_PUT_INT32 (3), "]", "}");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", "{", "bar", JSONRPC_MESSAGE_GET_VARIANT (&ar), "}");
  g_assert_nonnull (ar);
  g_assert_cmpint (r, ==, TRUE);
  g_clear_pointer (&node, g_variant_unref);
  g_clear_pointer (&ar, g_variant_unref);

  node = jsonrpc_message_new ("foo", "{", "bar", "[", JSONRPC_MESSAGE_PUT_INT32 (1), "]", "}", NULL);
  g_assert_nonnull (node);
  g_clear_pointer (&node, g_variant_unref);

  node2 = JSONRPC_MESSAGE_NEW ("bar", JSONRPC_MESSAGE_PUT_INT32 (1));
  g_assert_nonnull (node2);
  node = JSONRPC_MESSAGE_NEW ("foo", "{", JSONRPC_MESSAGE_PUT_VARIANT (node2), "}");
  g_assert_nonnull (node);
  g_clear_pointer (&node, g_variant_unref);
  g_clear_pointer (&node2, g_variant_unref);
}

static void
test_paren (void)
{
  g_autoptr(GVariant) node = NULL;
  const gchar *paren = "{";
  const gchar *str = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW ("foo", "{", "bar", "[", JSONRPC_MESSAGE_PUT_STRING (paren), "]", "}");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", "{", "bar", "[", JSONRPC_MESSAGE_GET_STRING (&str), "]", "}");
  g_assert_cmpstr (str, ==, "{");
  g_assert_cmpint (r, ==, TRUE);
}

static void
test_array_toplevel (void)
{
  g_autoptr(GVariant) node = NULL;
  g_autoptr(GVariantIter) iter = NULL;
  const gchar *a = NULL;
  const gchar *b = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW ("foo", "[", "a", "b", "c", "d", "e", "]");
  g_assert_nonnull (node);

  r = JSONRPC_MESSAGE_PARSE (node, "foo", JSONRPC_MESSAGE_GET_ITER (&iter));
  g_assert_cmpint (r, ==, TRUE);
  g_assert_nonnull (iter);

  r = JSONRPC_MESSAGE_PARSE_ARRAY (iter, JSONRPC_MESSAGE_GET_STRING (&a), JSONRPC_MESSAGE_GET_STRING (&b));
  g_assert_cmpint (r, ==, TRUE);
  g_assert_cmpstr (a, ==, "a");
}

static void
test_new_array (void)
{
  g_autoptr(GVariant) node = NULL;
  GVariantIter iter;
  const gchar *a = NULL;
  const gchar *b = NULL;
  const gchar *c = NULL;
  const gchar *d = NULL;
  const gchar *e = NULL;
  gboolean r;

  node = JSONRPC_MESSAGE_NEW_ARRAY ("a", "b", "c", "d", "e");
  g_assert_nonnull (node);
  g_assert_true (g_variant_is_of_type (node, G_VARIANT_TYPE ("av")));

  r = g_variant_iter_init (&iter, node);
  g_assert_true (r);

  r = JSONRPC_MESSAGE_PARSE_ARRAY (&iter,
                                   JSONRPC_MESSAGE_GET_STRING (&a),
                                   JSONRPC_MESSAGE_GET_STRING (&b),
                                   JSONRPC_MESSAGE_GET_STRING (&c),
                                   JSONRPC_MESSAGE_GET_STRING (&d),
                                   JSONRPC_MESSAGE_GET_STRING (&e));
  g_assert_cmpint (r, ==, TRUE);
  g_assert_cmpstr (a, ==, "a");
  g_assert_cmpstr (b, ==, "b");
  g_assert_cmpstr (c, ==, "c");
  g_assert_cmpstr (d, ==, "d");
  g_assert_cmpstr (e, ==, "e");
}

static void
test_new_array_objs (void)
{
  g_autoptr(GVariant) node = NULL;

  node = JSONRPC_MESSAGE_NEW_ARRAY ("{","}", "{", "}");
  g_assert_nonnull (node);
  g_assert_true (g_variant_is_of_type (node, G_VARIANT_TYPE ("av")));
  g_assert_cmpint (g_variant_n_children (node), ==, 2);
}

static void
test_null_string (void)
{
  g_autoptr(GVariant) msg = NULL;
  const gchar *foo = NULL;
  const gchar *foo2 = NULL;
  const gchar *content_type = "xx";
  gboolean success;

  msg = JSONRPC_MESSAGE_NEW (
    "foo", "bar",
    "foo2", JSONRPC_MESSAGE_PUT_STRING ("baz"),
    "content-type", JSONRPC_MESSAGE_PUT_STRING (NULL)
  );

  g_assert_nonnull (msg);

  success = JSONRPC_MESSAGE_PARSE (msg,
    "foo", JSONRPC_MESSAGE_GET_STRING (&foo),
    "foo2", JSONRPC_MESSAGE_GET_STRING (&foo2),
    "content-type", JSONRPC_MESSAGE_GET_STRING (&content_type)
  );

  g_assert_true (success);
  g_assert_cmpstr (foo, ==, "bar");
  g_assert_cmpstr (foo2, ==, "baz");
  g_assert_cmpstr (content_type, ==, NULL);
}

static void
test_strv (void)
{
  g_autoptr(GVariant) src = NULL;
  static const gchar *ar[] = { "a", "b", "c", NULL };
  g_auto(GStrv) get_ar = NULL;
  g_autofree gchar *print = NULL;
  gboolean r;

  src = JSONRPC_MESSAGE_NEW ("key", JSONRPC_MESSAGE_PUT_STRV (ar));
  print = g_variant_print (src, TRUE);
  g_assert_cmpstr (print, ==, "{'key': <['a', 'b', 'c']>}");

  r = JSONRPC_MESSAGE_PARSE (src, "key", JSONRPC_MESSAGE_GET_STRV (&get_ar));
  g_assert_true (r);
  g_assert_nonnull (get_ar);
  g_assert_cmpstr (get_ar[0], ==, "a");
  g_assert_cmpstr (get_ar[1], ==, "b");
  g_assert_cmpstr (get_ar[2], ==, "c");
  g_assert_null (get_ar[3]);
}

static void
test_null_strv (void)
{
  g_autoptr(GVariant) src = NULL;
  g_auto(GStrv) get_ar = NULL;
  g_auto(GStrv) get_ar_from_v = NULL;
  g_autofree gchar *print = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) v = NULL;
  gboolean r;

  src = JSONRPC_MESSAGE_NEW ("key", JSONRPC_MESSAGE_PUT_STRV (NULL));
  print = g_variant_print (src, TRUE);
  g_assert_cmpstr (print, ==, "{'key': <@mas nothing>}");

  r = JSONRPC_MESSAGE_PARSE (src, "key", JSONRPC_MESSAGE_GET_STRV (&get_ar));
  g_assert_true (r);
  g_assert_null (get_ar);

  node = json_from_string ("{\"key\": null}", &error);
  g_assert_no_error (error);
  g_assert_nonnull (node);

  v = json_gvariant_deserialize (node, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);

  r = JSONRPC_MESSAGE_PARSE (v, "key", JSONRPC_MESSAGE_GET_STRV (&get_ar_from_v));
  g_assert_true (r);
  g_assert_null (get_ar_from_v);
}

static void
test_putv_null (void)
{
  g_autoptr(GVariant) src = NULL;
  g_autoptr(GVariant) dst = NULL;
  GVariantDict dict;

  src = JSONRPC_MESSAGE_NEW ("key", "{", JSONRPC_MESSAGE_PUT_VARIANT (NULL), "}");
  g_assert_nonnull (src);

  g_variant_dict_init (&dict, NULL);
  g_variant_dict_insert (&dict, "key", "mav", NULL);
  dst = g_variant_dict_end (&dict);

  g_assert_nonnull (dst);
  g_assert_true (g_variant_equal (dst, src));
}

static void
test_putv_nonnull (void)
{
  g_autoptr(GVariant) src = NULL;
  g_autoptr(GVariant) dst = NULL;
  g_autoptr(GVariant) child = NULL;
  g_autoptr(GVariant) child2 = NULL;
  GVariantDict cdict;
  GVariantDict dict;

  g_variant_dict_init (&cdict, NULL);
  g_variant_dict_insert (&cdict, "hello", "s", "world");
  child = g_variant_take_ref (g_variant_dict_end (&cdict));

  src = JSONRPC_MESSAGE_NEW ("key", "{", JSONRPC_MESSAGE_PUT_VARIANT (child), "}");
  g_assert_nonnull (src);

  g_variant_dict_init (&dict, NULL);
  g_variant_dict_insert (&dict, "key", "v", child);
  dst = g_variant_take_ref (g_variant_dict_end (&dict));
  g_assert_nonnull (dst);

  g_assert_false (g_variant_is_floating (src));
  g_assert_false (g_variant_is_floating (dst));
  g_assert_false (g_variant_is_floating (child));
  g_assert_true (g_variant_equal (dst, src));

  JSONRPC_MESSAGE_PARSE (src, "key", JSONRPC_MESSAGE_GET_VARIANT (&child2));
  g_assert_true (g_variant_equal (child, child2));
}

gint
main (gint argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/Jsonrpc/Message/basic", test_basic);
  g_test_add_func ("/Jsonrpc/Message/deep_array", test_deep_array);
  g_test_add_func ("/Jsonrpc/Message/extract_array", test_extract_array);
  g_test_add_func ("/Jsonrpc/Message/extract_object", test_extract_object);
  g_test_add_func ("/Jsonrpc/Message/extract_node", test_extract_node);
  g_test_add_func ("/Jsonrpc/Message/paren", test_paren);
  g_test_add_func ("/Jsonrpc/Message/array_toplevel", test_array_toplevel);
  g_test_add_func ("/Jsonrpc/Message/new_array", test_new_array);
  g_test_add_func ("/Jsonrpc/Message/new_array_objs", test_new_array_objs);
  g_test_add_func ("/Jsonrpc/Message/null_string", test_null_string);
  g_test_add_func ("/Jsonrpc/Message/strv", test_strv);
  g_test_add_func ("/Jsonrpc/Message/null_strv", test_null_strv);
  g_test_add_func ("/Jsonrpc/Message/putv_null", test_putv_null);
  g_test_add_func ("/Jsonrpc/Message/putv_nonnull", test_putv_nonnull);
  return g_test_run ();
}
