#include "jcon.h"

static void
test_basic (void)
{
  g_autoptr(JsonNode) node = NULL;
  const gchar *foo1 = NULL;
  gint baz_baz_baz = 0;
  gboolean r;

  node = JCON_NEW (
    "foo", "foo1",
    "bar", "foo2",
    "baz", "{",
      "baz", "[", "{", "baz", JCON_INT (123), "}", "]",
    "}"
  );

  g_assert (node != NULL);

  r = JCON_EXTRACT (node,
    "foo", JCONE_STRING (foo1),
    "baz", "{",
      "baz", "[", "{", "baz", JCONE_INT (baz_baz_baz), "}", "]",
    "}"
  );

  g_assert_cmpstr (foo1, ==, "foo1");
  g_assert_cmpint (baz_baz_baz, ==, 123);
  g_assert_cmpint (r, ==, 1);
}

static void
test_deep_array (void)
{
  g_autoptr(JsonNode) node = NULL;
  const gchar *abc = NULL;
  const gchar *xyz = NULL;
  gboolean r;

  node = JCON_NEW ("foo", "[","[","[","[","[","[","[","[","[","[", "abc", "]", "]","]","]","]","]","]","]","]","]");
  g_assert (node != NULL);

  r = JCON_EXTRACT (node, "foo", "[","[","[","[","[","[","[","[","[","[", JCONE_STRING (abc), "]", "]","]","]","]","]","]","]","]","]");
  g_assert_cmpstr (abc, ==, "abc");
  g_assert_cmpint (r, ==, 1);

  g_clear_pointer (&node, json_node_unref);

  node = JCON_NEW ("foo", "[","[","[","[","[","[","[","[","[","{", "foo", "xyz", "}", "]","]","]","]","]","]","]","]","]");
  g_assert (node != NULL);

  r = JCON_EXTRACT (node, "foo", "[","[","[","[","[","[","[","[","[","{", "foo", JCONE_STRING (xyz), "}", "]","]","]","]","]","]","]","]","]");
  g_assert_cmpstr (xyz, ==, "xyz");
  g_assert_cmpint (r, ==, 1);

}

gint
main (gint argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/Jcon/basic", test_basic);
  g_test_add_func ("/Jcon/deep_array", test_deep_array);
  return g_test_run ();
}
