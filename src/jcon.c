/* Copyright 2009-2013 MongoDB, Inc.
 * Copyright      2016 Christian Hergert <chergert@redhat.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define G_LOG_DOMAIN "jcon"

#include <stdarg.h>

#include "jcon.h"

typedef union
{
  const gchar *v_string;
  gdouble      v_double;
  JsonObject  *v_object;
  JsonArray   *v_array;
  gboolean     v_boolean;
  gint         v_int;
} JconAppend;

const char *
jcon_magic  (void)
{
  return "THIS_IS_JCON_MAGIC";
}

const char *
jcone_magic (void)
{
  return "THIS_IS_JCONE_MAGIC";
}

static JsonNode *
jcon_append_to_node (JconType    type,
                     JconAppend *val)
{
  JsonNode *node;

  switch (type)
    {
    case JCON_TYPE_STRING:
      node = json_node_new (JSON_NODE_VALUE);
      json_node_set_string (node, val->v_string);
      return node;

    case JCON_TYPE_DOUBLE:
      node = json_node_new (JSON_NODE_VALUE);
      json_node_set_double (node, val->v_double);
      return node;

    case JCON_TYPE_BOOLEAN:
      node = json_node_new (JSON_NODE_VALUE);
      json_node_set_boolean (node, val->v_boolean);
      return node;

    case JCON_TYPE_NULL:
      return json_node_new (JSON_NODE_NULL);

    case JCON_TYPE_INT:
      node = json_node_new (JSON_NODE_VALUE);
      json_node_set_int (node, val->v_int);
      return node;

    case JCON_TYPE_ARRAY:
      node = json_node_new (JSON_NODE_ARRAY);
      json_node_set_array (node, val->v_array);
      return node;

    case JCON_TYPE_OBJECT:
      node = json_node_new (JSON_NODE_OBJECT);
      json_node_set_object (node, val->v_object);
      return node;

    case JCON_TYPE_OBJECT_START:
    case JCON_TYPE_OBJECT_END:
    case JCON_TYPE_ARRAY_START:
    case JCON_TYPE_ARRAY_END:
    case JCON_TYPE_END:
    default:
      return NULL;
    }
}

static JconType
jcon_append_tokenize (va_list    *ap,
                      JconAppend *u)
{
  const gchar *mark;
  JconType type;

  /* Consumes ap, storing output values into u and returning the type of the
   * captured token.
   *
   * The basic workflow goes like this:
   *
   * 1. Look at the current arg.  It will be a gchar *
   *    a. If it's a NULL, we're done processing.
   *    b. If it's JCON_MAGIC (a symbol with storage in this module)
   *       I. The next token is the type
   *       II. The type specifies how many args to eat and their types
   *    c. Otherwise it's either recursion related or a raw string
   *       I. If the first byte is '{', '}', '[', or ']' pass back an
   *          appropriate recursion token
   *       II. If not, just call it a v_string token and pass that back
   */

  mark = va_arg (*ap, const gchar *);

  g_assert (mark != JCONE_MAGIC);

  if (mark == NULL)
    {
      type = JCON_TYPE_END;
    }
  else if (mark == JCON_MAGIC)
    {
      type = va_arg (*ap, JconType);

      switch (type)
        {
        case JCON_TYPE_STRING:
          u->v_string = va_arg (*ap, const gchar *);
          break;

        case JCON_TYPE_DOUBLE:
          u->v_double = va_arg (*ap, double);
          break;

        case JCON_TYPE_OBJECT:
          u->v_object = va_arg (*ap, JsonObject *);
          break;

        case JCON_TYPE_ARRAY:
          u->v_array = va_arg (*ap, JsonArray *);
          break;

        case JCON_TYPE_BOOLEAN:
          u->v_boolean = va_arg (*ap, gboolean);
          break;

        case JCON_TYPE_NULL:
          break;

        case JCON_TYPE_INT:
          u->v_int = va_arg (*ap, gint);
          break;

        case JCON_TYPE_ARRAY_START:
        case JCON_TYPE_ARRAY_END:
        case JCON_TYPE_OBJECT_START:
        case JCON_TYPE_OBJECT_END:
        case JCON_TYPE_END:
        default:
          g_assert_not_reached ();
          break;
      }
    }
  else
    {
      switch (mark[0])
        {
        case '{':
          type = JCON_TYPE_OBJECT_START;
          break;

        case '}':
          type = JCON_TYPE_OBJECT_END;
          break;

        case '[':
          type = JCON_TYPE_ARRAY_START;
          break;

        case ']':
          type = JCON_TYPE_ARRAY_END;
          break;

        default:
          type = JCON_TYPE_STRING;
          u->v_string = mark;
          break;
        }
    }

  return type;
}

static void
jcon_append_va_list (JsonNode *node,
                     va_list  *args)
{
  g_assert (JSON_NODE_HOLDS_OBJECT (node));
  g_assert (args != NULL);

  for (;;)
    {
      const gchar *key = NULL;
      JconAppend val = { 0 };
      JconType type;

      g_assert (node != NULL);

      if (!JSON_NODE_HOLDS_ARRAY (node))
        {
          type = jcon_append_tokenize (args, &val);

          if (type == JCON_TYPE_END)
            return;

          if (type == JCON_TYPE_OBJECT_END)
            {
              node = json_node_get_parent (node);
              continue;
            }

          if (type != JCON_TYPE_STRING)
            g_error ("string keys are required for objects");

          key = val.v_string;
        }

      type = jcon_append_tokenize (args, &val);

      if (type == JCON_TYPE_END)
        g_error ("implausable time to reach end token");

      if (type == JCON_TYPE_OBJECT_START)
        {
          JsonNode *child_node = json_node_new (JSON_NODE_OBJECT);
          JsonObject *child_object = json_object_new ();

          json_node_take_object (child_node, child_object);

          if (JSON_NODE_HOLDS_ARRAY (node))
            json_array_add_element (json_node_get_array (node), child_node);
          else
            json_object_set_member (json_node_get_object (node), key, child_node);

          json_node_set_parent (child_node, node);

          node = child_node;

          continue;
        }
      else if (type == JCON_TYPE_ARRAY_START)
        {
          JsonNode *child_node = json_node_new (JSON_NODE_ARRAY);
          JsonArray *child_array = json_array_new ();

          json_node_take_array (child_node, child_array);

          if (JSON_NODE_HOLDS_ARRAY (node))
            json_array_add_element (json_node_get_array (node), child_node);
          else
            json_object_set_member (json_node_get_object (node), key, child_node);

          json_node_set_parent (child_node, node);

          node = child_node;

          continue;
        }
      else if (type == JCON_TYPE_OBJECT_END || type == JCON_TYPE_ARRAY_END)
        {
          node = json_node_get_parent (node);
          continue;
        }

      if (JSON_NODE_HOLDS_ARRAY (node))
        json_array_add_element (json_node_get_array (node), jcon_append_to_node (type, &val));
      else
        json_object_set_member (json_node_get_object (node), key, jcon_append_to_node (type, &val));
    }
}

JsonNode *
jcon_new (gpointer unused,
          ...)
{
  g_autoptr(JsonNode) node = NULL;
  va_list args;

  g_return_val_if_fail (unused == NULL, NULL);

  node = json_node_new (JSON_NODE_OBJECT);
  json_node_take_object (node, json_object_new ());

  va_start (args, unused);
  jcon_append_va_list (node, &args);
  va_end (args);

  return g_steal_pointer (&node);
}
