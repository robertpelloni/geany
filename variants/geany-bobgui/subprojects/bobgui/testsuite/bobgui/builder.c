/* buildertest.c
 * Copyright (C) 2006-2007 Async Open Source
 * Authors: Johan Dahlin
 *          Henrique Romano
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <math.h>

#include <bobgui/bobgui.h>
#include <gdk/gdkkeysyms.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#ifdef G_OS_WIN32
# define _BUILDER_TEST_EXPORT __declspec(dllexport)
#else
# define _BUILDER_TEST_EXPORT __attribute__((visibility("default")))
#endif

/* exported for BobguiBuilder */
_BUILDER_TEST_EXPORT void signal_normal (BobguiWindow *window, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_after (BobguiWindow *window, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_object (BobguiButton *button, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_object_after (BobguiButton *button, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_first (BobguiButton *button, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_second (BobguiButton *button, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_extra (BobguiButton *button, GParamSpec *spec);
_BUILDER_TEST_EXPORT void signal_extra2 (BobguiButton *button, GParamSpec *spec);


static BobguiBuilder *
builder_new_from_string (const char *buffer,
                         gsize length,
                         const char *domain)
{
  BobguiBuilder *builder;
  GError *error = NULL;

  builder = bobgui_builder_new ();
  if (domain)
    bobgui_builder_set_translation_domain (builder, domain);
  bobgui_builder_add_from_string (builder, buffer, length, &error);
  if (error)
    {
      g_print ("ERROR: %s", error->message);
      g_error_free (error);
    }

  return builder;
}

static void
test_parser (void)
{
  BobguiBuilder *builder;
  GError *error;

  builder = bobgui_builder_new ();

  error = NULL;
  bobgui_builder_add_from_string (builder, "<xxx/>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_UNHANDLED_TAG);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface invalid=\"X\"/>", -1, &error);
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><child/></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_TAG);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiBox\" id=\"a\"><object class=\"BobguiBox\" id=\"b\"/></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_TAG);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"Unknown\" id=\"a\"></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiWidget\" id=\"a\" constructor=\"none\"></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiButton\" id=\"a\"><child internal-child=\"foobar\"><object class=\"BobguiButton\" id=\"int\"/></child></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiButton\" id=\"a\"></object><object class=\"BobguiButton\" id=\"a\"/></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_DUPLICATE_ID);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiButton\" id=\"a\"><property name=\"deafbeef\"></property></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_PROPERTY);
  g_error_free (error);

  error = NULL;
  bobgui_builder_add_from_string (builder, "<interface><object class=\"BobguiButton\" id=\"a\"><signal name=\"deafbeef\" handler=\"bobgui_true\"/></object></interface>", -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_SIGNAL);
  g_error_free (error);

  g_object_unref (builder);
}

static int normal = 0;
static int after = 0;
static int object = 0;
static int object_after = 0;

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_normal (BobguiWindow *window, GParamSpec *spec)
{
  g_assert_true (BOBGUI_IS_WINDOW (window));
  g_assert_true (normal == 0);
  g_assert_true (after == 0);

  normal++;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_after (BobguiWindow *window, GParamSpec *spec)
{
  g_assert_true (BOBGUI_IS_WINDOW (window));
  g_assert_true (normal == 1);
  g_assert_true (after == 0);

  after++;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_object (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (BOBGUI_IS_BUTTON (button));
  g_assert_true (object == 0);
  g_assert_true (object_after == 0);

  object++;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_object_after (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (BOBGUI_IS_BUTTON (button));
  g_assert_true (object == 1);
  g_assert_true (object_after == 0);

  object_after++;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_first (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (normal == 0);
  normal = 10;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_second (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (normal == 10);
  normal = 20;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_extra (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (normal == 20);
  normal = 30;
}

_BUILDER_TEST_EXPORT
void /* exported for BobguiBuilder */
signal_extra2 (BobguiButton *button, GParamSpec *spec)
{
  g_assert_true (normal == 30);
  normal = 40;
}

static void
test_connect_signals (void)
{
  BobguiBuilder *builder;
  GObject *window;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiButton\" id=\"button\"/>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <signal name=\"notify::title\" handler=\"signal_normal\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_after\" after=\"yes\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_object\""
    "            object=\"button\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_object_after\""
    "            object=\"button\" after=\"yes\"/>"
    "  </object>"
    "</interface>";
  const char buffer_order[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <signal name=\"notify::title\" handler=\"signal_first\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_second\"/>"
    "  </object>"
    "</interface>";
  const char buffer_extra[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window2\">"
    "    <signal name=\"notify::title\" handler=\"signal_extra\"/>"
    "  </object>"
    "</interface>";
  const char buffer_extra2[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window3\">"
    "    <signal name=\"notify::title\" handler=\"signal_extra2\"/>"
    "  </object>"
    "</interface>";
  const char buffer_after_child[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiButton\" id=\"button1\"/>"
    "    </child>"
    "    <signal name=\"notify::title\" handler=\"signal_normal\"/>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_set_title (BOBGUI_WINDOW (window), "test");

  g_assert_cmpint (normal, ==, 1);
  g_assert_cmpint (after, ==, 1);
  g_assert_cmpint (object, ==, 1);
  g_assert_cmpint (object_after, ==, 1);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer_order, -1, NULL);
  window = bobgui_builder_get_object (builder, "window1");
  normal = 0;
  bobgui_window_set_title (BOBGUI_WINDOW (window), "test");
  g_assert_true (normal == 20);

  bobgui_window_destroy (BOBGUI_WINDOW (window));

  bobgui_builder_add_from_string (builder, buffer_extra,
			       strlen (buffer_extra), NULL);
  bobgui_builder_add_from_string (builder, buffer_extra2,
			       strlen (buffer_extra2), NULL);
  window = bobgui_builder_get_object (builder, "window2");
  bobgui_window_set_title (BOBGUI_WINDOW (window), "test");
  g_assert_true (normal == 30);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
  window = bobgui_builder_get_object (builder, "window3");
  bobgui_window_set_title (BOBGUI_WINDOW (window), "test");
  g_assert_true (normal == 40);
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);

  /* new test, reset globals */
  after = 0;
  normal = 0;

  builder = builder_new_from_string (buffer_after_child, -1, NULL);
  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_set_title (BOBGUI_WINDOW (window), "test");

  g_assert_true (normal == 1);
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);
}

static void
test_domain (void)
{
  BobguiBuilder *builder;
  const char buffer1[] = "<interface/>";
  const char buffer2[] = "<interface domain=\"domain\"/>";
  const char *domain;

  builder = builder_new_from_string (buffer1, -1, NULL);
  domain = bobgui_builder_get_translation_domain (builder);
  g_assert_true (domain == NULL);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer1, -1, "domain-1");
  domain = bobgui_builder_get_translation_domain (builder);
  g_assert_nonnull (domain);
  g_assert_true (strcmp (domain, "domain-1") == 0);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  domain = bobgui_builder_get_translation_domain (builder);
  g_assert_null (domain);
  g_object_unref (builder);
}

#if 0
static void
test_translation (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label\">"
    "        <property name=\"label\" translatable=\"yes\">File</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BobguiLabel *window, *label;

  setlocale (LC_ALL, "sv_SE");
  textdomain ("builder");
  bindtextdomain ("builder", "tests");

  builder = builder_new_from_string (buffer, -1, NULL);
  label = BOBGUI_LABEL (bobgui_builder_get_object (builder, "label"));
  g_assert_true (strcmp (bobgui_label_get_text (label), "Arkiv") == 0);

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);
}
#endif

static void
test_sizegroup (void)
{
  BobguiBuilder * builder;
  const char buffer1[] =
    "<interface domain=\"test\">"
    "  <object class=\"BobguiSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">horizontal</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"vbox1\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiCheckButton\" id=\"radio1\"/>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiCheckButton\" id=\"radio2\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface domain=\"test\">"
    "  <object class=\"BobguiSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">horizontal</property>"
    "    <widgets>"
    "    </widgets>"
    "   </object>"
    "</interface>";
  const char buffer3[] =
    "<interface domain=\"test\">"
    "  <object class=\"BobguiSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">horizontal</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BobguiSizeGroup\" id=\"sizegroup2\">"
    "    <property name=\"mode\">horizontal</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"vbox1\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiCheckButton\" id=\"radio1\"/>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiCheckButton\" id=\"radio2\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *sizegroup;
  GSList *widgets;

  builder = builder_new_from_string (buffer1, -1, NULL);
  sizegroup = bobgui_builder_get_object (builder, "sizegroup1");
  widgets = bobgui_size_group_get_widgets (BOBGUI_SIZE_GROUP (sizegroup));
  g_assert_cmpint (g_slist_length (widgets), ==, 2);
  g_slist_free (widgets);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  sizegroup = bobgui_builder_get_object (builder, "sizegroup1");
  widgets = bobgui_size_group_get_widgets (BOBGUI_SIZE_GROUP (sizegroup));
  g_assert_cmpint (g_slist_length (widgets), ==, 0);
  g_slist_free (widgets);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);
  sizegroup = bobgui_builder_get_object (builder, "sizegroup1");
  widgets = bobgui_size_group_get_widgets (BOBGUI_SIZE_GROUP (sizegroup));
  g_assert_cmpint (g_slist_length (widgets), ==, 2);
  g_slist_free (widgets);
  sizegroup = bobgui_builder_get_object (builder, "sizegroup2");
  widgets = bobgui_size_group_get_widgets (BOBGUI_SIZE_GROUP (sizegroup));
  g_assert_cmpint (g_slist_length (widgets), ==, 2);
  g_slist_free (widgets);

#if 0
  {
    GObject *window;
    window = bobgui_builder_get_object (builder, "window1");
    bobgui_window_destroy (BOBGUI_WINDOW (window));
  }
#endif
  g_object_unref (builder);
}

static void
test_list_store (void)
{
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"guint\"/>"
    "    </columns>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"2\" comments=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">Johan</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"2\">50</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  const char buffer3[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"2\" comments=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">50</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"0\">Johan</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">19</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  BobguiBuilder *builder;
  GObject *store;
  BobguiTreeIter iter;
  char *surname, *lastname;
  int age;

  builder = builder_new_from_string (buffer1, -1, NULL);
  store = bobgui_builder_get_object (builder, "liststore1");
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 2);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_UINT);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  store = bobgui_builder_get_object (builder, "liststore1");
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 3);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 2), ==, G_TYPE_INT);

  g_assert_cmpint (bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter), ==, TRUE);
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "John");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Doe");
  g_free (lastname);
  g_assert_cmpint (age, ==, 25);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "Johan");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Dole");
  g_free (lastname);
  g_assert_cmpint (age, ==, 50);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);
  store = bobgui_builder_get_object (builder, "liststore1");
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 3);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 2), ==, G_TYPE_INT);

  g_assert_true (bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter));
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "John");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Doe");
  g_free (lastname);
  g_assert_cmpint (age, ==, 25);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "Johan");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Dole");
  g_free (lastname);
  g_assert_cmpint (age, ==, 50);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_null (surname);
  g_assert_null (lastname);
  g_assert_cmpint (age, ==, 19);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  g_object_unref (builder);
}

static void
test_tree_store (void)
{
  const char buffer[] =
    "<interface domain=\"test\">"
    "  <object class=\"BobguiTreeStore\" id=\"treestore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"guint\"/>"
    "    </columns>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiTreeStore\" id=\"treestore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"2\" comments=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">Johan</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"2\">50</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  const char buffer3[] =
    "<interface>"
    "  <object class=\"BobguiTreeStore\" id=\"treestore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"2\" comments=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">50</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"0\">Johan</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">19</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  const char buffer4[] =
    "<interface>"
    "  <object class=\"BobguiTreeStore\" id=\"treestore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"gint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"2\" comments=\"foobar\">25</col>"
    "        <row>"
    "          <col id=\"2\">50</col>"
    "          <col id=\"1\">Dole</col>"
    "          <col id=\"0\">Johan</col>"
    "        </row>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">19</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  BobguiBuilder *builder;
  GObject *store;
  BobguiTreeIter iter, parent;
  char *surname, *lastname;
  int age;

  builder = builder_new_from_string (buffer, -1, NULL);
  store = bobgui_builder_get_object (builder, "treestore1");
  g_assert_true (BOBGUI_IS_TREE_STORE (store));
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 2);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_UINT);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  store = bobgui_builder_get_object (builder, "treestore1");
  g_assert_true (BOBGUI_IS_TREE_STORE (store));
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 3);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 2), ==, G_TYPE_INT);

  g_assert_cmpint (bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter), ==, TRUE);
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "John");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Doe");
  g_free (lastname);
  g_assert_cmpint (age, ==, 25);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "Johan");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Dole");
  g_free (lastname);
  g_assert_cmpint (age, ==, 50);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);
  store = bobgui_builder_get_object (builder, "treestore1");
  g_assert_true (BOBGUI_IS_TREE_STORE (store));
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 3);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 2), ==, G_TYPE_INT);

  g_assert_true (bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter));
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "John");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Doe");
  g_free (lastname);
  g_assert_cmpint (age, ==, 25);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "Johan");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Dole");
  g_free (lastname);
  g_assert_cmpint (age, ==, 50);
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_null (surname);
  g_assert_null (lastname);
  g_assert_cmpint (age, ==, 19);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  g_object_unref (builder);

  builder = builder_new_from_string (buffer4, -1, NULL);
  store = bobgui_builder_get_object (builder, "treestore1");
  g_assert_true (BOBGUI_IS_TREE_STORE (store));
  g_assert_cmpint (bobgui_tree_model_get_n_columns (BOBGUI_TREE_MODEL (store)), ==, 3);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 0), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 1), ==, G_TYPE_STRING);
  g_assert_cmpint (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), 2), ==, G_TYPE_INT);

  g_assert_true (bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter));
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "John");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Doe");
  g_free (lastname);
  g_assert_cmpint (age, ==, 25);
  parent = iter;
  g_assert_true (bobgui_tree_model_iter_children (BOBGUI_TREE_MODEL (store), &iter, &parent));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_cmpstr (surname, ==, "Johan");
  g_free (surname);
  g_assert_cmpstr (lastname, ==, "Dole");
  g_free (lastname);
  g_assert_cmpint (age, ==, 50);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));
  iter = parent;
  g_assert_true (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert_null (surname);
  g_assert_null (lastname);
  g_assert_cmpint (age, ==, 19);
  g_assert_false (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter));

  g_object_unref (builder);
}

static void
test_types (void)
{
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiBox\" id=\"box\"/>"
    "  <object class=\"BobguiButton\" id=\"button\"/>"
    "  <object class=\"BobguiCheckButton\" id=\"checkbutton\"/>"
    "  <object class=\"BobguiDialog\" id=\"dialog\"/>"
    "  <object class=\"BobguiDrawingArea\" id=\"drawingarea\"/>"
    "  <object class=\"BobguiEntry\" id=\"entry\"/>"
    "  <object class=\"BobguiFontButton\" id=\"fontbutton\"/>"
    "  <object class=\"BobguiImage\" id=\"image\"/>"
    "  <object class=\"BobguiLabel\" id=\"label\"/>"
    "  <object class=\"BobguiListStore\" id=\"liststore\"/>"
    "  <object class=\"BobguiNotebook\" id=\"notebook\"/>"
    "  <object class=\"BobguiProgressBar\" id=\"progressbar\"/>"
    "  <object class=\"BobguiSizeGroup\" id=\"sizegroup\"/>"
    "  <object class=\"BobguiScrolledWindow\" id=\"scrolledwindow\"/>"
    "  <object class=\"BobguiSpinButton\" id=\"spinbutton\"/>"
    "  <object class=\"BobguiStatusbar\" id=\"statusbar\"/>"
    "  <object class=\"BobguiTextView\" id=\"textview\"/>"
    "  <object class=\"BobguiToggleButton\" id=\"togglebutton\"/>"
    "  <object class=\"BobguiTreeStore\" id=\"treestore\"/>"
    "  <object class=\"BobguiTreeView\" id=\"treeview\"/>"
    "  <object class=\"BobguiViewport\" id=\"viewport\"/>"
    "  <object class=\"BobguiWindow\" id=\"window\"/>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" type-func=\"bobgui_window_get_type\" id=\"window\"/>"
    "</interface>";
  const char buffer3[] =
    "<interface>"
    "  <object class=\"XXXInvalidType\" type-func=\"bobgui_window_get_type\" id=\"window\"/>"
    "</interface>";
  const char buffer4[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" type-func=\"xxx_invalid_get_type_function\" id=\"window\"/>"
    "</interface>";
  const char buffer5[] =
    "<interface>"
    "  <object type-func=\"bobgui_window_get_type\" id=\"window\"/>"
    "</interface>";
  BobguiBuilder *builder;
  GObject *window;
  GError *error;

  builder = builder_new_from_string (buffer, -1, NULL);
  bobgui_window_destroy (BOBGUI_WINDOW (bobgui_builder_get_object (builder, "dialog")));
  bobgui_window_destroy (BOBGUI_WINDOW (bobgui_builder_get_object (builder, "window")));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  window = bobgui_builder_get_object (builder, "window");
  g_assert_true (BOBGUI_IS_WINDOW (window));
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);
  window = bobgui_builder_get_object (builder, "window");
  g_assert_true (BOBGUI_IS_WINDOW (window));
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);

  error = NULL;
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer4, -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_TYPE_FUNCTION);
  g_error_free (error);
  g_object_unref (builder);

  error = NULL;
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer5, -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_MISSING_ATTRIBUTE);
  g_error_free (error);
  g_object_unref (builder);
}

static void
test_spin_button (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "<object class=\"BobguiAdjustment\" id=\"adjustment1\">"
    "<property name=\"lower\">0</property>"
    "<property name=\"upper\">10</property>"
    "<property name=\"step-increment\">2</property>"
    "<property name=\"page-increment\">3</property>"
    "<property name=\"page-size\">0</property>"
    "<property name=\"value\">1</property>"
    "</object>"
    "<object class=\"BobguiSpinButton\" id=\"spinbutton1\">"
    "<property name=\"visible\">True</property>"
    "<property name=\"adjustment\">adjustment1</property>"
    "</object>"
    "</interface>";
  GObject *obj;
  BobguiAdjustment *adjustment;
  double value;

  builder = builder_new_from_string (buffer, -1, NULL);
  obj = bobgui_builder_get_object (builder, "spinbutton1");
  g_assert_true (BOBGUI_IS_SPIN_BUTTON (obj));
  adjustment = bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (obj));
  g_assert_true (BOBGUI_IS_ADJUSTMENT (adjustment));
  g_object_get (adjustment, "value", &value, NULL);
  g_assert_cmpint (value, ==, 1);
  g_object_get (adjustment, "lower", &value, NULL);
  g_assert_cmpint (value, ==, 0);
  g_object_get (adjustment, "upper", &value, NULL);
  g_assert_cmpint (value, ==, 10);
  g_object_get (adjustment, "step-increment", &value, NULL);
  g_assert_cmpint (value, ==, 2);
  g_object_get (adjustment, "page-increment", &value, NULL);
  g_assert_cmpint (value, ==, 3);
  g_object_get (adjustment, "page-size", &value, NULL);
  g_assert_cmpint (value, ==, 0);

  g_object_unref (builder);
}

static void
test_notebook (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiNotebook\" id=\"notebook1\">"
    "    <child>"
    "      <object class=\"BobguiNotebookPage\">"
    "        <property name=\"child\">"
    "          <object class=\"BobguiLabel\" id=\"label1\">"
    "            <property name=\"label\">label1</property>"
    "          </object>"
    "        </property>"
    "        <property name=\"tab\">"
    "          <object class=\"BobguiLabel\" id=\"tablabel1\">"
    "           <property name=\"label\">tab_label1</property>"
    "          </object>"
    "        </property>"
    "      </object>"
    "    </child>"
    "    <child>"
    "      <object class=\"BobguiNotebookPage\">"
    "        <property name=\"child\">"
    "          <object class=\"BobguiLabel\" id=\"label2\">"
    "            <property name=\"label\">label2</property>"
    "          </object>"
    "        </property>"
    "        <property name=\"tab\">"
    "          <object class=\"BobguiLabel\" id=\"tablabel2\">"
    "            <property name=\"label\">tab_label2</property>"
    "          </object>"
    "        </property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *notebook;
  BobguiWidget *label;

  builder = builder_new_from_string (buffer, -1, NULL);
  notebook = bobgui_builder_get_object (builder, "notebook1");
  g_assert_nonnull (notebook);
  g_assert_cmpint (bobgui_notebook_get_n_pages (BOBGUI_NOTEBOOK (notebook)), ==, 2);

  label = bobgui_notebook_get_nth_page (BOBGUI_NOTEBOOK (notebook), 0);
  g_assert_true (BOBGUI_IS_LABEL (label));
  g_assert_cmpstr (bobgui_label_get_label (BOBGUI_LABEL (label)), ==, "label1");
  label = bobgui_notebook_get_tab_label (BOBGUI_NOTEBOOK (notebook), label);
  g_assert_true (BOBGUI_IS_LABEL (label));
  g_assert_cmpstr (bobgui_label_get_label (BOBGUI_LABEL (label)), ==, "tab_label1");

  label = bobgui_notebook_get_nth_page (BOBGUI_NOTEBOOK (notebook), 1);
  g_assert_true (BOBGUI_IS_LABEL (label));
  g_assert_cmpstr (bobgui_label_get_label (BOBGUI_LABEL (label)), ==, "label2");
  label = bobgui_notebook_get_tab_label (BOBGUI_NOTEBOOK (notebook), label);
  g_assert_true (BOBGUI_IS_LABEL (label));
  g_assert_cmpstr (bobgui_label_get_label (BOBGUI_LABEL (label)), ==, "tab_label2");

  g_object_unref (builder);
}

static void
test_construct_only_property (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <property name=\"css-name\">amazing</property>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiTextTagTable\" id=\"tagtable1\"/>"
    "  <object class=\"BobguiTextBuffer\" id=\"textbuffer1\">"
    "    <property name=\"tag-table\">tagtable1</property>"
    "  </object>"
    "</interface>";
  GObject *widget, *tagtable, *textbuffer;

  builder = builder_new_from_string (buffer, -1, NULL);
  widget = bobgui_builder_get_object (builder, "window1");
  g_assert_cmpstr (bobgui_widget_get_css_name (BOBGUI_WIDGET (widget)), ==, "amazing");

  bobgui_window_destroy (BOBGUI_WINDOW (widget));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  textbuffer = bobgui_builder_get_object (builder, "textbuffer1");
  g_assert_nonnull (textbuffer);
  g_object_get (textbuffer, "tag-table", &tagtable, NULL);
  g_assert_true (tagtable == bobgui_builder_get_object (builder, "tagtable1"));
  g_object_unref (tagtable);
  g_object_unref (builder);
}

static void
test_object_properties (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"vbox\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiLabel\" id=\"label1\">"
    "            <property name=\"mnemonic-widget\">spinbutton1</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiSpinButton\" id=\"spinbutton1\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window2\"/>"
    "</interface>";
  GObject *label, *spinbutton, *window;

  builder = builder_new_from_string (buffer, -1, NULL);
  label = bobgui_builder_get_object (builder, "label1");
  g_assert_nonnull (label);
  spinbutton = bobgui_builder_get_object (builder, "spinbutton1");
  g_assert_nonnull (spinbutton);
  g_assert_true (spinbutton == (GObject*)bobgui_label_get_mnemonic_widget (BOBGUI_LABEL (label)));

  bobgui_builder_add_from_string (builder, buffer2, -1, NULL);
  window = bobgui_builder_get_object (builder, "window2");
  g_assert_nonnull (window);
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);
}

static void
test_children (void)
{
  BobguiBuilder * builder;
  BobguiWidget *content_area;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiButton\" id=\"button1\">"
    "        <property name=\"label\">Hello</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiDialog\" id=\"dialog1\">"
    "    <property name=\"use_header_bar\">1</property>"
    "    <child internal-child=\"content_area\">"
    "      <object class=\"BobguiBox\" id=\"dialog1-vbox\">"
    "        <property name=\"orientation\">vertical</property>"
    "          <child internal-child=\"action_area\">"
    "            <object class=\"BobguiBox\" id=\"dialog1-action_area\">"
    "              <property name=\"orientation\">horizontal</property>"
    "            </object>"
    "          </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  GObject *window, *button;
  GObject *dialog, *vbox, *action_area;
  BobguiWidget *child;
  int count;

  builder = builder_new_from_string (buffer1, -1, NULL);
  window = bobgui_builder_get_object (builder, "window1");
  g_assert_true (BOBGUI_IS_WINDOW (window));

  button = bobgui_builder_get_object (builder, "button1");
  g_assert_true (BOBGUI_IS_BUTTON (button));
  g_assert_nonnull (bobgui_widget_get_parent (BOBGUI_WIDGET(button)));
  g_assert_cmpstr (bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (bobgui_widget_get_parent (BOBGUI_WIDGET (button)))), ==, "window1");

  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  dialog = bobgui_builder_get_object (builder, "dialog1");
  g_assert_true (BOBGUI_IS_DIALOG (dialog));
  count = 0;
  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (dialog));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    count++;
  g_assert_cmpint (count, ==, 2);

  vbox = bobgui_builder_get_object (builder, "dialog1-vbox");
  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog));
  g_assert_true (BOBGUI_IS_BOX (vbox));
  g_assert_cmpint (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (vbox)), ==, BOBGUI_ORIENTATION_VERTICAL);
  g_assert_cmpstr (bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (content_area)), ==, "dialog1-vbox");

  action_area = bobgui_builder_get_object (builder, "dialog1-action_area");
  g_assert_true (BOBGUI_IS_BOX (action_area));
  g_assert_cmpint (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (action_area)), ==, BOBGUI_ORIENTATION_HORIZONTAL);
  g_assert_nonnull (bobgui_widget_get_parent (BOBGUI_WIDGET (action_area)));
  g_assert_nonnull (bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (action_area)));
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
  g_object_unref (builder);
}

static void
test_layout_properties (void)
{
  BobguiBuilder * builder;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiGrid\" id=\"grid1\">"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label1\">"
    "        <layout>"
    "          <property name=\"column\">1</property>"
    "        </layout>"
    "      </object>"
    "    </child>"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label2\">"
    "        <layout>"
    "          <property name=\"column\">0</property>"
    "        </layout>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  GObject *label, *vbox;

  builder = builder_new_from_string (buffer1, -1, NULL);
  vbox = bobgui_builder_get_object (builder, "grid1");
  g_assert_true (BOBGUI_IS_GRID (vbox));

  label = bobgui_builder_get_object (builder, "label1");
  g_assert_true (BOBGUI_IS_LABEL (label));

  label = bobgui_builder_get_object (builder, "label2");
  g_assert_true (BOBGUI_IS_LABEL (label));

  g_object_unref (builder);
}

static void
test_treeview_column (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "<object class=\"BobguiListStore\" id=\"liststore1\">"
    "  <columns>"
    "    <column type=\"gchararray\"/>"
    "    <column type=\"guint\"/>"
    "  </columns>"
    "  <data>"
    "    <row>"
    "      <col id=\"0\">John</col>"
    "      <col id=\"1\">25</col>"
    "    </row>"
    "  </data>"
    "</object>"
    "<object class=\"BobguiWindow\" id=\"window1\">"
    "  <child>"
    "    <object class=\"BobguiTreeView\" id=\"treeview1\">"
    "      <property name=\"visible\">True</property>"
    "      <property name=\"model\">liststore1</property>"
    "      <child>"
    "        <object class=\"BobguiTreeViewColumn\" id=\"column1\">"
    "          <property name=\"title\">Test</property>"
    "          <child>"
    "            <object class=\"BobguiCellRendererText\" id=\"renderer1\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">1</attribute>"
    "            </attributes>"
    "          </child>"
    "        </object>"
    "      </child>"
    "      <child>"
    "        <object class=\"BobguiTreeViewColumn\" id=\"column2\">"
    "          <property name=\"title\">Number</property>"
    "          <child>"
    "            <object class=\"BobguiCellRendererText\" id=\"renderer2\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">0</attribute>"
    "            </attributes>"
    "          </child>"
    "        </object>"
    "      </child>"
    "    </object>"
    "  </child>"
    "</object>"
    "</interface>";
  GObject *window, *treeview;
  BobguiTreeViewColumn *column;
  GList *renderers;
  GObject *renderer;

  builder = builder_new_from_string (buffer, -1, NULL);
  treeview = bobgui_builder_get_object (builder, "treeview1");
  g_assert_true (BOBGUI_IS_TREE_VIEW (treeview));
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), 0);
  g_assert_true (BOBGUI_IS_TREE_VIEW_COLUMN (column));
  g_assert_cmpstr (bobgui_tree_view_column_get_title (column), ==, "Test");

  renderers = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (column));
  g_assert_cmpint (g_list_length (renderers), ==, 1);
  renderer = g_list_nth_data (renderers, 0);
  g_assert_true (BOBGUI_IS_CELL_RENDERER_TEXT (renderer));
  g_list_free (renderers);

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);
}

static void
test_icon_view (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "      <column type=\"GdkPixbuf\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">test</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiIconView\" id=\"iconview1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"text-column\">0</property>"
    "        <property name=\"pixbuf-column\">1</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *window, *iconview;

  builder = builder_new_from_string (buffer, -1, NULL);
  iconview = bobgui_builder_get_object (builder, "iconview1");
  g_assert_true (BOBGUI_IS_ICON_VIEW (iconview));

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);
}

static void
test_combo_box (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"guint\"/>"
    "      <column type=\"gchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">1</col>"
    "        <col id=\"1\">Foo</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">2</col>"
    "        <col id=\"1\">Bar</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiComboBox\" id=\"combobox1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer2\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">1</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *window, *combobox;

  builder = builder_new_from_string (buffer, -1, NULL);
  combobox = bobgui_builder_get_object (builder, "combobox1");
  g_assert_nonnull (combobox);

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);
}

#if 0
static void
test_combo_box_entry (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"guint\"/>"
    "      <column type=\"gchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">1</col>"
    "        <col id=\"1\">Foo</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">2</col>"
    "        <col id=\"1\">Bar</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiComboBox\" id=\"comboboxentry1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"has-entry\">True</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer1\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">0</attribute>"
    "            </attributes>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer2\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">1</attribute>"
    "            </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *window, *combobox, *renderer;
  char *text;

  builder = builder_new_from_string (buffer, -1, NULL);
  combobox = bobgui_builder_get_object (builder, "comboboxentry1");
  g_assert_nonnull (combobox);

  renderer = bobgui_builder_get_object (builder, "renderer2");
  g_assert_nonnull (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert_cmpstr (text, ==, "Bar");
  g_free (text);

  renderer = bobgui_builder_get_object (builder, "renderer1");
  g_assert_nonnull (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert_cmpstr (text, ==, "2");
  g_free (text);

  window = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);
}
#endif

static void
test_cell_view (void)
{
  BobguiBuilder *builder;
  const char *buffer =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"gchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">test</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiCellView\" id=\"cellview1\">"
    "        <property name=\"visible\">True</property>"
    "        <property name=\"model\">liststore1</property>"
    "        <child>"
    "          <object class=\"BobguiCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *cellview;
  GObject *model, *window;
  BobguiTreePath *path;
  GList *renderers;

  builder = builder_new_from_string (buffer, -1, NULL);
  cellview = bobgui_builder_get_object (builder, "cellview1");
  g_assert_nonnull (builder);
  g_assert_true (BOBGUI_IS_CELL_VIEW (cellview));
  g_object_get (cellview, "model", &model, NULL);
  g_assert_true (BOBGUI_IS_TREE_MODEL (model));
  g_object_unref (model);
  path = bobgui_tree_path_new_first ();
  bobgui_cell_view_set_displayed_row (BOBGUI_CELL_VIEW (cellview), path);
  bobgui_tree_path_free (path);

  renderers = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (cellview));
  g_assert_cmpint (g_list_length (renderers), ==, 1);
  g_list_free (renderers);

  window = bobgui_builder_get_object (builder, "window1");
  g_assert_nonnull (window);
  bobgui_window_destroy (BOBGUI_WINDOW (window));

  g_object_unref (builder);
}

static void
test_dialog (void)
{
  BobguiBuilder * builder;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiDialog\" id=\"dialog1\">"
    "    <child internal-child=\"content_area\">"
    "      <object class=\"BobguiBox\" id=\"dialog1-vbox\">"
    "        <property name=\"orientation\">vertical</property>"
    "          <child internal-child=\"action_area\">"
    "            <object class=\"BobguiBox\" id=\"dialog1-action_area\">"
    "              <property name=\"orientation\">horizontal</property>"
    "              <child>"
    "                <object class=\"BobguiButton\" id=\"button_cancel\"/>"
    "              </child>"
    "              <child>"
    "                <object class=\"BobguiButton\" id=\"button_ok\"/>"
    "              </child>"
    "            </object>"
    "          </child>"
    "      </object>"
    "    </child>"
    "    <action-widgets>"
    "      <action-widget response=\"3\">button_ok</action-widget>"
    "      <action-widget response=\"-5\">button_cancel</action-widget>"
    "    </action-widgets>"
    "  </object>"
    "</interface>";

  GObject *dialog1;
  GObject *button_ok;
  GObject *button_cancel;

  builder = builder_new_from_string (buffer1, -1, NULL);
  dialog1 = bobgui_builder_get_object (builder, "dialog1");
  button_ok = bobgui_builder_get_object (builder, "button_ok");
  g_assert_cmpint (bobgui_dialog_get_response_for_widget (BOBGUI_DIALOG (dialog1), BOBGUI_WIDGET (button_ok)), ==, 3);
  button_cancel = bobgui_builder_get_object (builder, "button_cancel");
  g_assert_cmpint (bobgui_dialog_get_response_for_widget (BOBGUI_DIALOG (dialog1), BOBGUI_WIDGET (button_cancel)), ==, -5);

  bobgui_window_destroy (BOBGUI_WINDOW (dialog1));
  g_object_unref (builder);
}

static void
test_message_dialog (void)
{
  BobguiBuilder * builder;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiMessageDialog\" id=\"dialog1\">"
    "    <child internal-child=\"message_area\">"
    "      <object class=\"BobguiBox\" id=\"dialog-message-area\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiExpander\" id=\"expander\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  GObject *dialog1;
  GObject *expander;

  builder = builder_new_from_string (buffer1, -1, NULL);
  dialog1 = bobgui_builder_get_object (builder, "dialog1");
  expander = bobgui_builder_get_object (builder, "expander");
  g_assert_true (BOBGUI_IS_EXPANDER (expander));
  g_assert_true (bobgui_widget_get_parent (BOBGUI_WIDGET (expander)) == bobgui_message_dialog_get_message_area (BOBGUI_MESSAGE_DIALOG (dialog1)));

  bobgui_window_destroy (BOBGUI_WINDOW (dialog1));
  g_object_unref (builder);
}

static void
test_widget (void)
{
  const char *buffer =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <property name=\"focus-widget\">button1</property>"
    "    <child>"
    "      <object class=\"BobguiButton\" id=\"button1\">"
    "      </object>"
    "    </child>"
    "  </object>"
   "</interface>";
  const char *buffer2 =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiButton\" id=\"button1\">"
    "         <property name=\"receives-default\">True</property>"
    "      </object>"
    "    </child>"
    "  </object>"
   "</interface>";
  const char *buffer3 =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"vbox1\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiLabel\" id=\"label1\">"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiButton\" id=\"button1\">"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char *buffer4 =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label1\">"
    "         <property name=\"label\">Thelabel</property>"
    "      </object>"
    "    </child>"
    "  </object>"
   "</interface>";
  BobguiBuilder *builder;
  GObject *window1, *button1, *label1;

  builder = builder_new_from_string (buffer, -1, NULL);
  button1 = bobgui_builder_get_object (builder, "button1");

  g_assert_true (bobgui_widget_is_focus (BOBGUI_WIDGET (button1)));
  window1 = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window1));

  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  button1 = bobgui_builder_get_object (builder, "button1");

  g_assert_true (bobgui_widget_get_receives_default (BOBGUI_WIDGET (button1)));

  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);

  window1 = bobgui_builder_get_object (builder, "window1");
  label1 = bobgui_builder_get_object (builder, "label1");
  g_assert_true (BOBGUI_IS_LABEL (label1));

  bobgui_window_destroy (BOBGUI_WINDOW (window1));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer4, -1, NULL);
  label1 = bobgui_builder_get_object (builder, "label1");
  g_assert_true (BOBGUI_IS_LABEL (label1));

  g_object_unref (builder);
}

static void
test_window (void)
{
  const char *buffer1 =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "     <property name=\"title\"></property>"
    "  </object>"
   "</interface>";
  const char *buffer2 =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "  </object>"
   "</interface>";
  BobguiBuilder *builder;
  GObject *window1;
  char *title;

  builder = builder_new_from_string (buffer1, -1, NULL);
  window1 = bobgui_builder_get_object (builder, "window1");
  g_object_get (window1, "title", &title, NULL);
  g_assert_cmpstr (title, ==, "");
  g_free (title);
  bobgui_window_destroy (BOBGUI_WINDOW (window1));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  window1 = bobgui_builder_get_object (builder, "window1");
  bobgui_window_destroy (BOBGUI_WINDOW (window1));
  g_object_unref (builder);
}

static void
test_value_from_string (void)
{
  GValue value = G_VALUE_INIT;
  GError *error = NULL;
  BobguiBuilder *builder;

  builder = bobgui_builder_new ();

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_STRING, "test", &value, &error));
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  g_assert_cmpstr (g_value_get_string (&value), ==, "test");
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "true", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_true (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "false", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_false (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "yes", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_true (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "no", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_false (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "0", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_false (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "1", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_true (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "tRuE", &value, &error));
  g_assert_true (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert_true (g_value_get_boolean (&value));
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "blaurgh", &value, &error) == FALSE);
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "yess", &value, &error) == FALSE);
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "trueee", &value, &error) == FALSE);
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_false (bobgui_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, "", &value, &error));
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_INT, "12345", &value, &error));
  g_assert_true (G_VALUE_HOLDS_INT (&value));
  g_assert_cmpint (g_value_get_int (&value), ==, 12345);
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_LONG, "9912345", &value, &error));
  g_assert_true (G_VALUE_HOLDS_LONG (&value));
  g_assert_cmpint (g_value_get_long (&value), ==, 9912345);
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_UINT, "2345", &value, &error));
  g_assert_true (G_VALUE_HOLDS_UINT (&value));
  g_assert_cmpint (g_value_get_uint (&value), ==, 2345);
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_INT64, "-2345", &value, &error));
  g_assert_true (G_VALUE_HOLDS_INT64 (&value));
  g_assert_cmpint (g_value_get_int64 (&value), ==, -2345);
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_UINT64, "2345", &value, &error));
  g_assert_true (G_VALUE_HOLDS_UINT64 (&value));
  g_assert_cmpint (g_value_get_uint64 (&value), ==, 2345);
  g_value_unset (&value);

  g_assert_true (bobgui_builder_value_from_string_type (builder, G_TYPE_FLOAT, "1.454", &value, &error));
  g_assert_true (G_VALUE_HOLDS_FLOAT (&value));
  g_assert_cmpfloat (fabs (g_value_get_float (&value) - 1.454), <, 0.00001);
  g_value_unset (&value);

  g_assert_false (bobgui_builder_value_from_string_type (builder, G_TYPE_FLOAT, "abc", &value, &error));
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_false (bobgui_builder_value_from_string_type (builder, G_TYPE_INT, "/-+,abc", &value, &error));
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_true (bobgui_builder_value_from_string_type (builder, BOBGUI_TYPE_TEXT_DIRECTION, "rtl", &value, &error));
  g_assert_true (G_VALUE_HOLDS_ENUM (&value));
  g_assert_cmpint (g_value_get_enum (&value), ==, BOBGUI_TEXT_DIR_RTL);
  g_value_unset (&value);

  g_assert_false (bobgui_builder_value_from_string_type (builder, BOBGUI_TYPE_TEXT_DIRECTION, "sliff", &value, &error));
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_assert_false (bobgui_builder_value_from_string_type (builder, BOBGUI_TYPE_TEXT_DIRECTION, "foobar", &value, &error));
  g_value_unset (&value);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE);
  g_error_free (error);
  error = NULL;

  g_object_unref (builder);
}

static gboolean model_freed = FALSE;

static void
model_weakref (gpointer data,
               GObject *model)
{
  model_freed = TRUE;
}

static void
test_reference_counting (void)
{
  BobguiBuilder *builder;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiListStore\" id=\"liststore1\"/>"
    "  <object class=\"BobguiListStore\" id=\"liststore2\"/>"
    "  <object class=\"BobguiWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BobguiTreeView\" id=\"treeview1\">"
    "        <property name=\"model\">liststore1</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiBox\" id=\"vbox1\">"
    "        <property name=\"orientation\">vertical</property>"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label1\"/>"
    "    </child>"
    "  </object>"
    "</interface>";
  GObject *window, *treeview, *model;

  builder = builder_new_from_string (buffer1, -1, NULL);
  window = bobgui_builder_get_object (builder, "window1");
  treeview = bobgui_builder_get_object (builder, "treeview1");
  model = bobgui_builder_get_object (builder, "liststore1");
  g_object_unref (builder);

  g_object_weak_ref (model, (GWeakNotify)model_weakref, NULL);

  g_assert_false (model_freed);
  bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (treeview), NULL);
  g_assert_true (model_freed);

  bobgui_window_destroy (BOBGUI_WINDOW (window));

  builder = builder_new_from_string (buffer2, -1, NULL);
  g_object_unref (builder);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct {
  gboolean weight;
  gboolean foreground;
  gboolean underline;
  gboolean size;
  gboolean font_desc;
  gboolean language;
} FoundAttrs;

static gboolean
filter_pango_attrs (PangoAttribute *attr,
		    gpointer        data)
{
  FoundAttrs *found = (FoundAttrs *)data;

  if (attr->klass->type == PANGO_ATTR_WEIGHT)
    found->weight = TRUE;
  else if (attr->klass->type == PANGO_ATTR_FOREGROUND)
    found->foreground = TRUE;
  else if (attr->klass->type == PANGO_ATTR_UNDERLINE)
    found->underline = TRUE;
  /* Make sure optional start/end properties are working */
  else if (attr->klass->type == PANGO_ATTR_SIZE &&
	   attr->start_index == 5 &&
	   attr->end_index   == 10)
    found->size = TRUE;
  else if (attr->klass->type == PANGO_ATTR_FONT_DESC)
    found->font_desc = TRUE;
  else if (attr->klass->type == PANGO_ATTR_LANGUAGE)
    found->language = TRUE;

  return TRUE;
}

static void
test_pango_attributes (void)
{
  BobguiBuilder *builder;
  FoundAttrs found = { 0, };
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\" value=\"PANGO_WEIGHT_BOLD\"/>"
    "      <attribute name=\"foreground\" value=\"DarkSlateGray\"/>"
    "      <attribute name=\"underline\" value=\"True\"/>"
    "      <attribute name=\"size\" value=\"4\" start=\"5\" end=\"10\"/>"
    "      <attribute name=\"font-desc\" value=\"Sans Italic 22\"/>"
    "      <attribute name=\"language\" value=\"pt_BR\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";
  const char err_buffer1[] =
    "<interface>"
    "  <object class=\"BobguiLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";
  const char err_buffer2[] =
    "<interface>"
    "  <object class=\"BobguiLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\" value=\"PANGO_WEIGHT_BOLD\" unrecognized=\"True\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";

  GObject *label;
  GError  *error = NULL;
  PangoAttrList *attrs, *filtered;

  /* Test attributes are set */
  builder = builder_new_from_string (buffer, -1, NULL);
  label = bobgui_builder_get_object (builder, "label1");
  g_assert_nonnull (label);

  attrs = bobgui_label_get_attributes (BOBGUI_LABEL (label));
  g_assert_nonnull (attrs);

  filtered = pango_attr_list_filter (attrs, filter_pango_attrs, &found);
  g_assert_true (filtered);
  pango_attr_list_unref (filtered);

  g_assert_true (found.weight);
  g_assert_true (found.foreground);
  g_assert_true (found.underline);
  g_assert_true (found.size);
  g_assert_true (found.language);
  g_assert_true (found.font_desc);

  g_object_unref (builder);

  /* Test errors are set */
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, err_buffer1, -1, &error);
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_MISSING_ATTRIBUTE);
  g_object_unref (builder);
  g_error_free (error);
  error = NULL;

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, err_buffer2, -1, &error);
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE);
  g_object_unref (builder);
  g_error_free (error);
}

static void
test_requires (void)
{
  BobguiBuilder *builder;
  GError     *error = NULL;
  char       *buffer;
  const char buffer_fmt[] =
    "<interface>"
    "  <requires lib=\"bobgui\" version=\"%d.%d\"/>"
    "</interface>";

  buffer = g_strdup_printf (buffer_fmt, BOBGUI_MAJOR_VERSION, BOBGUI_MINOR_VERSION + 1);
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer, -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_VERSION_MISMATCH);
  g_object_unref (builder);
  g_error_free (error);
  g_free (buffer);
}

static void
test_add_objects (void)
{
  BobguiBuilder *builder;
  GError *error;
  int ret;
  GObject *obj;
  const char *objects[2] = {"mainbox", NULL};
  const char *objects2[3] = {"mainbox", "window2", NULL};
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"mainbox\">"
    "        <property name=\"orientation\">vertical</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BobguiLabel\" id=\"label1\">"
    "            <property name=\"visible\">True</property>"
    "            <property name=\"label\" translatable=\"no\">first label</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiLabel\" id=\"label2\">"
    "            <property name=\"visible\">True</property>"
    "            <property name=\"label\" translatable=\"no\">second label</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "  <object class=\"BobguiWindow\" id=\"window2\">"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label3\">"
    "        <property name=\"label\" translatable=\"no\">second label</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "<interface/>";

  error = NULL;
  builder = bobgui_builder_new ();
  ret = bobgui_builder_add_objects_from_string (builder, buffer, -1, objects, &error);
  g_assert_no_error (error);
  obj = bobgui_builder_get_object (builder, "window");
  g_assert_null (obj);
  obj = bobgui_builder_get_object (builder, "window2");
  g_assert_null (obj);
  obj = bobgui_builder_get_object (builder, "mainbox");
  g_assert_true (BOBGUI_IS_WIDGET (obj));
  g_object_unref (builder);

  error = NULL;
  builder = bobgui_builder_new ();
  ret = bobgui_builder_add_objects_from_string (builder, buffer, -1, objects2, &error);
  g_assert_true (ret);
  g_assert_null (error);
  obj = bobgui_builder_get_object (builder, "window");
  g_assert_null (obj);
  obj = bobgui_builder_get_object (builder, "window2");
  g_assert_true (BOBGUI_IS_WINDOW (obj));
  bobgui_window_destroy (BOBGUI_WINDOW (obj));
  obj = bobgui_builder_get_object (builder, "mainbox");
  g_assert_true (BOBGUI_IS_WIDGET (obj));
  g_object_unref (builder);
}

static void
test_message_area (void)
{
  BobguiBuilder *builder;
  GObject *obj, *obj1;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiInfoBar\" id=\"infobar1\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"contentarea1\">"
    "        <property name=\"orientation\">horizontal</property>"
    "        <child>"
    "          <object class=\"BobguiLabel\" id=\"content\">"
    "            <property name=\"label\" translatable=\"yes\">Message</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <child type=\"action\">"
    "      <object class=\"BobguiButton\" id=\"button_ok\">"
    "        <property name=\"label\">bobgui-ok</property>"
    "      </object>"
    "    </child>"
    "    <action-widgets>"
    "      <action-widget response=\"1\">button_ok</action-widget>"
    "    </action-widgets>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);
  obj = bobgui_builder_get_object (builder, "infobar1");
  g_assert_true (BOBGUI_IS_INFO_BAR (obj));
  obj1 = bobgui_builder_get_object (builder, "content");
  g_assert_true (BOBGUI_IS_LABEL (obj1));

  obj1 = bobgui_builder_get_object (builder, "button_ok");
  g_assert_true (BOBGUI_IS_BUTTON (obj1));

  g_object_unref (builder);
}

static void
test_gmenu (void)
{
  BobguiBuilder *builder;
  GObject *obj, *obj1;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window\">"
    "  </object>"
    "  <menu id='edit-menu'>"
    "    <section>"
    "      <item>"
    "        <attribute name='label'>Undo</attribute>"
    "        <attribute name='action'>undo</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label'>Redo</attribute>"
    "        <attribute name='action'>redo</attribute>"
    "      </item>"
    "    </section>"
    "    <section></section>"
    "    <section>"
    "      <attribute name='label'>Copy &amp; Paste</attribute>"
    "      <item>"
    "        <attribute name='label'>Cut</attribute>"
    "        <attribute name='action'>cut</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label'>Copy</attribute>"
    "        <attribute name='action'>copy</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label'>Paste</attribute>"
    "        <attribute name='action'>paste</attribute>"
    "      </item>"
    "    </section>"
    "    <item><link name='section' id='blargh'>"
    "      <item>"
    "        <attribute name='label'>Bold</attribute>"
    "        <attribute name='action'>bold</attribute>"
    "      </item>"
    "      <submenu>"
    "        <attribute name='label'>Language</attribute>"
    "        <item>"
    "          <attribute name='label'>Latin</attribute>"
    "          <attribute name='action'>lang</attribute>"
    "          <attribute name='target'>'latin'</attribute>"
    "        </item>"
    "        <item>"
    "          <attribute name='label'>Greek</attribute>"
    "          <attribute name='action'>lang</attribute>"
    "          <attribute name='target'>'greek'</attribute>"
    "        </item>"
    "        <item>"
    "          <attribute name='label'>Urdu</attribute>"
    "          <attribute name='action'>lang</attribute>"
    "          <attribute name='target'>'urdu'</attribute>"
    "        </item>"
    "      </submenu>"
    "    </link></item>"
    "  </menu>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);
  obj = bobgui_builder_get_object (builder, "window");
  g_assert_true (BOBGUI_IS_WINDOW (obj));
  obj1 = bobgui_builder_get_object (builder, "edit-menu");
  g_assert_true (G_IS_MENU_MODEL (obj1));
  obj1 = bobgui_builder_get_object (builder, "blargh");
  g_assert_true (G_IS_MENU_MODEL (obj1));
  g_object_unref (builder);
}

static void
test_level_bar (void)
{
  BobguiBuilder *builder;
  GError *error = NULL;
  GObject *obj, *obj1;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window\">"
    "    <child>"
    "      <object class=\"BobguiLevelBar\" id=\"levelbar\">"
    "        <property name=\"value\">4.70</property>"
    "        <property name=\"min-value\">2</property>"
    "        <property name=\"max-value\">5</property>"
    "        <offsets>"
    "          <offset name=\"low\" value=\"2.25\"/>"
    "          <offset name=\"custom\" value=\"3\"/>"
    "          <offset name=\"high\" value=\"3\"/>"
    "        </offsets>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const char buffer2[] =
    "<interface>"
    "  <object class=\"BobguiLevelBar\" id=\"levelbar\">"
    "    <offsets>"
    "      <offset name=\"low\" bogus_attr=\"foo\"/>"
    "    </offsets>"
    "  </object>"
    "</interface>";
  const char buffer3[] =
    "<interface>"
    "  <object class=\"BobguiLevelBar\" id=\"levelbar\">"
    "    <offsets>"
    "      <offset name=\"low\" value=\"1\"/>"
    "    </offsets>"
    "    <bogus_tag>"
    "    </bogus_tag>"
    "  </object>"
    "</interface>";

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer1, -1, &error);
  g_assert_true (error == NULL);

  obj = bobgui_builder_get_object (builder, "window");
  g_assert_true (BOBGUI_IS_WINDOW (obj));
  obj1 = bobgui_builder_get_object (builder, "levelbar");
  g_assert_true (BOBGUI_IS_LEVEL_BAR (obj1));
  g_object_unref (builder);

  error = NULL;
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer2, -1, &error);
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_MISSING_ATTRIBUTE);
  g_error_free (error);
  g_object_unref (builder);

  error = NULL;
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer3, -1, &error);
  g_assert_error (error, BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_UNHANDLED_TAG);
  g_error_free (error);
  g_object_unref (builder);
}

static void
test_expose_object (void)
{
  BobguiBuilder *builder;
  GError *error = NULL;
  BobguiWidget *menu;
  GObject *obj;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiMenuButton\" id=\"button\">"
    "    <property name=\"popover\">external_menu</property>"
    "  </object>"
    "</interface>";

  menu = bobgui_popover_new ();
  builder = bobgui_builder_new ();
  bobgui_builder_expose_object (builder, "external_menu", G_OBJECT (menu));
  bobgui_builder_add_from_string (builder, buffer, -1, &error);
  g_assert_no_error (error);

  obj = bobgui_builder_get_object (builder, "button");
  g_assert_true (BOBGUI_IS_MENU_BUTTON (obj));

  g_assert_true (bobgui_menu_button_get_popover (BOBGUI_MENU_BUTTON (obj)) == BOBGUI_POPOVER (menu));

  g_object_unref (builder);
}

static void
test_no_ids (void)
{
  BobguiBuilder *builder;
  GError *error = NULL;
  GObject *obj;
  const char buffer[] =
    "<interface>"
    "  <object class=\"BobguiInfoBar\">"
    "    <child>"
    "      <object class=\"BobguiBox\">"
    "        <property name=\"orientation\">horizontal</property>"
    "        <child>"
    "          <object class=\"BobguiLabel\">"
    "            <property name=\"label\" translatable=\"yes\">Message</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <child type=\"action\">"
    "      <object class=\"BobguiButton\" id=\"button_ok\">"
    "        <property name=\"label\">bobgui-ok</property>"
    "      </object>"
    "    </child>"
    "    <action-widgets>"
    "      <action-widget response=\"1\">button_ok</action-widget>"
    "    </action-widgets>"
    "  </object>"
    "</interface>";

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, buffer, -1, &error);
  g_assert_null (error);

  obj = bobgui_builder_get_object (builder, "button_ok");
  g_assert_true (BOBGUI_IS_BUTTON (obj));

  g_object_unref (builder);
}

static void
test_property_bindings (void)
{
  const char *buffer =
    "<interface>"
    "  <object class=\"BobguiWindow\" id=\"window\">"
    "    <child>"
    "      <object class=\"BobguiBox\" id=\"vbox\">"
    "        <property name=\"visible\">True</property>"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BobguiCheckButton\" id=\"checkbutton\">"
    "            <property name=\"active\">false</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiButton\" id=\"button\">"
    "            <property name=\"sensitive\" bind-source=\"checkbutton\" bind-property=\"active\" bind-flags=\"sync-create\">false</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiButton\" id=\"button2\">"
    "            <property name=\"sensitive\" bind-source=\"checkbutton\" bind-property=\"active\" />"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BobguiButton\" id=\"button3\">"
    "            <property name=\"sensitive\" bind-source=\"button\" bind-flags=\"sync-create\" />"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  BobguiBuilder *builder;
  GObject *checkbutton, *button, *button2, *button3, *window;

  builder = builder_new_from_string (buffer, -1, NULL);

  checkbutton = bobgui_builder_get_object (builder, "checkbutton");
  g_assert_true (BOBGUI_IS_CHECK_BUTTON (checkbutton));
  g_assert_true (!bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (checkbutton)));

  button = bobgui_builder_get_object (builder, "button");
  g_assert_true (BOBGUI_IS_BUTTON (button));
  g_assert_false (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button)));

  button2 = bobgui_builder_get_object (builder, "button2");
  g_assert_true (BOBGUI_IS_BUTTON (button2));
  g_assert_true (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button2)));

  button3 = bobgui_builder_get_object (builder, "button3");
  g_assert_true (BOBGUI_IS_BUTTON (button3));
  g_assert_false (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button3)));

  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (checkbutton), TRUE);
  g_assert_true (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button)));
  g_assert_true (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button2)));
  g_assert_true (bobgui_widget_get_sensitive (BOBGUI_WIDGET (button3)));

  window = bobgui_builder_get_object (builder, "window");
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  g_object_unref (builder);
}

#define MY_BOBGUI_GRID_TEMPLATE "\
<interface>\n\
 <template class=\"MyBobguiGrid\" parent=\"BobguiGrid\">\n\
   <property name=\"visible\">True</property>\n\
    <child>\n\
     <object class=\"BobguiLabel\" id=\"label\">\n\
       <property name=\"visible\">True</property>\n\
     </object>\n\
  </child>\n\
 </template>\n\
</interface>\n"

#define MY_TYPE_BOBGUI_GRID             (my_bobgui_grid_get_type ())
#define MY_IS_BOBGUI_GRID(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_BOBGUI_GRID))

typedef struct
{
  BobguiGridClass parent_class;
} MyBobguiGridClass;

typedef struct
{
  BobguiLabel *label;
} MyBobguiGridPrivate;

typedef struct
{
  BobguiGrid parent_instance;
  BobguiLabel *label;
  MyBobguiGridPrivate *priv;
} MyBobguiGrid;

G_DEFINE_TYPE_WITH_PRIVATE (MyBobguiGrid, my_bobgui_grid, BOBGUI_TYPE_GRID);

static void
my_bobgui_grid_init (MyBobguiGrid *grid)
{
  grid->priv = my_bobgui_grid_get_instance_private (grid);
  bobgui_widget_init_template (BOBGUI_WIDGET (grid));
}

static void
my_bobgui_grid_class_init (MyBobguiGridClass *klass)
{
  GBytes *template = g_bytes_new_static (MY_BOBGUI_GRID_TEMPLATE, strlen (MY_BOBGUI_GRID_TEMPLATE));
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  bobgui_widget_class_set_template (widget_class, template);
  bobgui_widget_class_bind_template_child (widget_class, MyBobguiGrid, label);
  bobgui_widget_class_bind_template_child_private (widget_class, MyBobguiGrid, label);

  g_bytes_unref (template);
}

static void
test_template (void)
{
  MyBobguiGrid *my_bobgui_grid;

  /* make sure the type we are trying to register does not exist */
  g_assert_false (g_type_from_name ("MyBobguiGrid"));

  /* create the template object */
  my_bobgui_grid = g_object_new (MY_TYPE_BOBGUI_GRID, NULL);

  /* Check everything is fine */
  g_assert_true (g_type_from_name ("MyBobguiGrid"));
  g_assert_true (MY_IS_BOBGUI_GRID (my_bobgui_grid));
  g_assert_true (my_bobgui_grid->label == my_bobgui_grid->priv->label);
  g_assert_true (BOBGUI_IS_LABEL (my_bobgui_grid->label));
  g_assert_true (BOBGUI_IS_LABEL (my_bobgui_grid->priv->label));

  g_object_ref_sink (my_bobgui_grid);
  g_object_unref (my_bobgui_grid);
}

_BUILDER_TEST_EXPORT void
on_cellrenderertoggle1_toggled (BobguiCellRendererToggle *cell)
{
}

static void
test_anaconda_signal (void)
{
  BobguiBuilder *builder;
  const char buffer[] =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<interface>"
    "  <requires lib='bobgui' version='4.0'/>"
    "  <object class='BobguiListStore' id='liststore1'>"
    "    <columns>"
    "      <!-- column-name use -->"
    "      <column type='gboolean'/>"
    "    </columns>"
    "  </object>"
    "  <object class='BobguiWindow' id='window1'>"
    "    <child>"
    "      <object class='BobguiTreeView' id='treeview1'>"
    "        <property name='visible'>True</property>"
    "        <property name='model'>liststore1</property>"
    "        <child internal-child='selection'>"
    "          <object class='BobguiTreeSelection' id='treeview-selection1'/>"
    "        </child>"
    "        <child>"
    "          <object class='BobguiTreeViewColumn' id='treeviewcolumn1'>"
    "            <property name='title' translatable='yes'>column</property>"
    "            <child>"
    "              <object class='BobguiCellRendererToggle' id='cellrenderertoggle1'>"
    "                <signal name='toggled' handler='on_cellrenderertoggle1_toggled' swapped='no'/>"
    "              </object>"
    "              <attributes>"
    "                <attribute name='active'>0</attribute>"
    "              </attributes>"
    "            </child>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);

  g_object_unref (builder);
}

static void
test_file_filter (void)
{
  BobguiBuilder *builder;
  GObject *obj;
  BobguiFileFilter *filter;

  const char buffer[] =
    "<interface>"
    "  <object class='BobguiFileFilter' id='filter1'>"
    "    <property name='name'>Text and Images</property>"
    "    <mime-types>"
    "      <mime-type>text/plain</mime-type>"
    "      <mime-type>image/*</mime-type>"
    "    </mime-types>"
    "    <patterns>"
    "      <pattern>*.txt</pattern>"
    "      <pattern>*.png</pattern>"
    "    </patterns>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);
  obj = bobgui_builder_get_object (builder, "filter1");
  g_assert_true (BOBGUI_IS_FILE_FILTER (obj));
  filter = BOBGUI_FILE_FILTER (obj);
  g_assert_cmpstr (bobgui_file_filter_get_name (filter), ==, "Text and Images");
  g_assert_true (g_strv_contains (bobgui_file_filter_get_attributes (filter), G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE));
  g_assert_true (g_strv_contains (bobgui_file_filter_get_attributes (filter), G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME));

  g_object_unref (builder);
}

static void
test_shortcuts (void)
{
  BobguiBuilder *builder;
  GObject *obj;

  const char buffer[] =
    "<interface>"
    "  <object class='BobguiBox' id='box'>"
    "    <child>"
    "      <object class='BobguiShortcutController' id='controller'>"
    "        <property name='scope'>managed</property>"
    "        <child>"
    "          <object class='BobguiShortcut'>"
    "            <property name='trigger'>&lt;Control&gt;k</property>"
    "            <property name='action'>activate</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);
  obj = bobgui_builder_get_object (builder, "controller");
  g_assert_true (BOBGUI_IS_SHORTCUT_CONTROLLER (obj));
  g_object_unref (builder);
}

static void
test_transforms (void)
{
  BobguiBuilder * builder;
  const char buffer1[] =
    "<interface>"
    "  <object class=\"BobguiFixed\" id=\"fixed1\">"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label1\">"
    "        <layout>"
    "          <property name=\"transform\">rotateX(45.0)</property>"
    "        </layout>"
    "      </object>"
    "    </child>"
    "    <child>"
    "      <object class=\"BobguiLabel\" id=\"label2\">"
    "        <layout>"
    "          <property name=\"transform\">scale3d(1,2,3)translate3d(2,3,0)</property>"
    "        </layout>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  GObject *label, *vbox;

  builder = builder_new_from_string (buffer1, -1, NULL);
  vbox = bobgui_builder_get_object (builder, "fixed1");
  g_assert_true (BOBGUI_IS_FIXED (vbox));

  label = bobgui_builder_get_object (builder, "label1");
  g_assert_true (BOBGUI_IS_LABEL (label));

  label = bobgui_builder_get_object (builder, "label2");
  g_assert_true (BOBGUI_IS_LABEL (label));

  g_object_unref (builder);
}

G_MODULE_EXPORT char *
builder_get_search (gpointer item)
{
  return g_strdup (bobgui_string_filter_get_search (item));
}

G_MODULE_EXPORT char *
builder_copy_arg (gpointer item, const char *arg)
{
  return g_strdup (arg);
}

G_MODULE_EXPORT char *
builder_get_search_if_null (gpointer item, const char *arg)
{
  return (arg == NULL) ? g_strdup (bobgui_string_filter_get_search (item)) : g_strdup (arg);
}

static void
test_expressions (void)
{
  const char *tests[] = {
    "<interface>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'><constant type='gchararray'>Hello World</constant></property>"
    "  </object>"
    "</interface>",
    "<interface>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'><closure type='gchararray' function='builder_get_search'></closure></property>"
    "  </object>"
    "</interface>",
    "<interface>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'><lookup type='BobguiStringFilter' name='search'></lookup></property>"
    "  </object>"
    "</interface>",
    "<interface>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'><closure type='gchararray' function='builder_copy_arg'>"
    "      <constant type='gchararray'>Hello World</constant>"
    "    </closure></property>"
    "  </object>"
    "</interface>",
    "<interface>"
    "  <object class='BobguiLabel' id='label'></object>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'>"
    "      <try>"
    "        <lookup name='title' type='BobguiWindow'>"
    "          <lookup name='root' type='BobguiLabel'>label</lookup>"
    "        </lookup>"
    "        <constant type='gchararray'>Hello World</constant>"
    "      </try>"
    "    </property>"
    "  </object>"
    "</interface>",
    "<interface>"
    "  <object class='BobguiStringFilter' id='filter'>"
    "    <property name='search'>Hello World</property>"
    "    <property name='expression'>"
    "      <closure type='gchararray' function='builder_get_search_if_null'>"
    "        <constant type='gchararray' initial='true' />"
    "      </closure>"
    "    </property>"
    "  </object>"
    "</interface>",
  };
  BobguiBuilder *builder;
  GObject *obj;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      builder = builder_new_from_string (tests[i], -1, NULL);
      obj = bobgui_builder_get_object (builder, "filter");
      g_assert_true (BOBGUI_IS_FILTER (obj));
      g_assert_true (bobgui_filter_match (BOBGUI_FILTER (obj), obj));

      g_object_unref (builder);
    }
}

#define MY_BOBGUI_BOX_TEMPLATE "\
<interface>\n\
 <template class=\"MyBobguiBox\" parent=\"BobguiWidget\">\n\
  <child>\n\
   <object class=\"BobguiLabel\" id=\"label\">\n\
    <property name=\"label\">First</property>\n\
   </object>\n\
  </child>\n\
  <child>\n\
   <object class=\"BobguiLabel\" id=\"label2\">\n\
    <property name=\"label\">Second</property>\n\
   </object>\n\
  </child>\n\
 </template>\n\
</interface>\n"

#define MY_TYPE_BOBGUI_BOX (my_bobgui_box_get_type ())
G_DECLARE_FINAL_TYPE    (MyBobguiBox, my_bobgui_box, MY, BOBGUI_BOX, BobguiWidget)

struct _MyBobguiBox
{
  BobguiWidget  parent_instance;
  BobguiLabel  *label;
  BobguiLabel  *label2;
};

G_DEFINE_TYPE (MyBobguiBox, my_bobgui_box, BOBGUI_TYPE_WIDGET);

static void
my_bobgui_box_init (MyBobguiBox *grid)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (grid));
}

static void
my_bobgui_box_dispose (GObject *obj)
{
  MyBobguiBox  *my_bobgui_box = MY_BOBGUI_BOX (obj);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (my_bobgui_box))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (my_bobgui_box_parent_class)->dispose (obj);
}

static void
my_bobgui_box_class_init (MyBobguiBoxClass *klass)
{
  GBytes *template = g_bytes_new_static (MY_BOBGUI_BOX_TEMPLATE, strlen (MY_BOBGUI_BOX_TEMPLATE));
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  bobgui_widget_class_set_template (widget_class, template);
  bobgui_widget_class_bind_template_child (widget_class, MyBobguiBox, label);
  bobgui_widget_class_bind_template_child (widget_class, MyBobguiBox, label2);

  G_OBJECT_CLASS (klass)->dispose = my_bobgui_box_dispose;

  g_bytes_unref (template);
}

typedef struct
{
  MyBobguiBox *my_bobgui_box;
  guint     destroy_count;
} BoxDestroyData;

static void
my_label_destroy (BobguiLabel *label, BoxDestroyData *data)
{
  g_assert_true (MY_IS_BOBGUI_BOX (data->my_bobgui_box));
  /* Make sure the other label is null if it was disposed first */
  g_assert_true (!data->my_bobgui_box->label2 || BOBGUI_IS_LABEL (data->my_bobgui_box->label2));
  data->destroy_count++;
}

static void
my_label2_destroy (BobguiLabel *label2, BoxDestroyData *data)
{
  g_assert_true (MY_IS_BOBGUI_BOX (data->my_bobgui_box));
  /* Make sure the other label is null if it was disposed first */
  g_assert_true (!data->my_bobgui_box->label || BOBGUI_IS_LABEL (data->my_bobgui_box->label));
  data->destroy_count++;
}

static void
test_child_dispose_order (void)
{
  BoxDestroyData data = { 0, };

  /* make sure the type we are trying to register does not exist */
  g_assert_false (g_type_from_name ("MyBobguiBox"));

  /* create the template object */
  data.my_bobgui_box = g_object_ref_sink (g_object_new (MY_TYPE_BOBGUI_BOX, NULL));

  /* Check everything is fine */
  g_assert_true (g_type_from_name ("MyBobguiBox"));
  g_assert_true (MY_IS_BOBGUI_BOX (data.my_bobgui_box));
  g_assert_true (BOBGUI_IS_LABEL (data.my_bobgui_box->label));
  g_assert_true (BOBGUI_IS_LABEL (data.my_bobgui_box->label2));

  /* Check if both labels are destroyed */
  g_signal_connect (data.my_bobgui_box->label, "destroy", G_CALLBACK (my_label_destroy), &data);
  g_signal_connect (data.my_bobgui_box->label2, "destroy", G_CALLBACK (my_label2_destroy), &data);
  g_object_unref (data.my_bobgui_box);
  g_assert_cmpuint (data.destroy_count, ==, 2);
}

#define MY_BOBGUI_BUILDABLE_TEMPLATE "\
<interface>\n\
 <template class=\"MyBobguiBuildable\" parent=\"BobguiWidget\">\n\
  <custom/>\n\
  <custom>\n\
    <custom/>\n\
  </custom>\n\
 </template>\n\
</interface>\n"

#define MY_TYPE_BOBGUI_BUILDABLE (my_bobgui_buildable_get_type ())
G_DECLARE_FINAL_TYPE          (MyBobguiBuildable, my_bobgui_buildable, MY, BOBGUI_BUILDABLE, BobguiWidget)

struct _MyBobguiBuildable
{
  BobguiWidget parent_instance;
};

static void my_bobgui_buildable_buildable_init (BobguiBuildableIface *iface);
static BobguiBuildableIface *parent_buildable_iface;

G_DEFINE_TYPE_WITH_CODE (MyBobguiBuildable, my_bobgui_buildable, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                my_bobgui_buildable_buildable_init));

static void
my_bobgui_buildable_init (MyBobguiBuildable *buildable)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (buildable));
}

static void
my_bobgui_buildable_class_init (MyBobguiBuildableClass *klass)
{
  GBytes *template = g_bytes_new_static (MY_BOBGUI_BUILDABLE_TEMPLATE, strlen (MY_BOBGUI_BUILDABLE_TEMPLATE));
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  bobgui_widget_class_set_template (widget_class, template);

  g_bytes_unref (template);
}

static const BobguiBuildableParser custom_parser = {
  NULL,
  NULL,
  NULL,
  NULL
};

static gboolean
my_bobgui_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                   BobguiBuilder         *builder,
                                   GObject            *child,
                                   const char         *tagname,
                                   BobguiBuildableParser *parser,
                                   gpointer           *parser_data)
{
  if (strcmp (tagname, "custom") == 0) {
    *parser = custom_parser;
    *parser_data = NULL;

    return TRUE;
  }

  return FALSE;
}

static void
my_bobgui_buildable_custom_finished (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *tagname,
                                  gpointer      user_data)
{
  if (strcmp (tagname, "custom") == 0)
    return;

  parent_buildable_iface->custom_finished (buildable, builder, child,
                                           tagname, user_data);
}

static void
my_bobgui_buildable_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = my_bobgui_buildable_custom_tag_start;
  iface->custom_finished = my_bobgui_buildable_custom_finished;
}

static void
test_buildable (void)
{
  MyBobguiBuildable *my_bobgui_buildable;

  /* make sure the type we are trying to register does not exist */
  g_assert_false (g_type_from_name ("MyBobguiBuildable"));

  /* create the template object */
  my_bobgui_buildable = g_object_new (MY_TYPE_BOBGUI_BUILDABLE, NULL);

  /* Check everything is fine */
  g_assert_true (g_type_from_name ("MyBobguiBuildable"));
  g_assert_true (MY_IS_BOBGUI_BUILDABLE (my_bobgui_buildable));

  g_object_ref_sink (my_bobgui_buildable);
  g_object_unref (my_bobgui_buildable);
}


static GFile *
builder_object_get_file_property (BobguiBuilder  *builder, const gchar *object_id)
{
  GObject *obj = bobgui_builder_get_object (builder, object_id);
  GFile *value = NULL;

  g_assert_nonnull (obj);

  g_object_get (obj, "file", &value, NULL);
  g_assert_nonnull (value);

  g_assert_true (g_type_is_a (G_OBJECT_TYPE (value), G_TYPE_FILE));

  return value;
}

static void
test_picture (void)
{
  BobguiBuilder *builder;
  GError *error = NULL;
  GFile *file_path, *file_uri;
  gchar *contents = NULL;
  gchar *path, *uri;

  /* Load from file so builder knows where to read the files from */
  path = g_test_build_filename (G_TEST_DIST, "ui", "picture.ui", NULL);
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_file (builder, path, &error);
  if (error)
    {
      g_print ("ERROR: %s", error->message);
      g_error_free (error);
    }
  g_assert_null (error);

  file_path = builder_object_get_file_property (builder, "relative_path");
  path = g_file_get_path (file_path);
  g_assert_nonnull (path);

  /* Check path is absolute */
  g_assert_true (g_path_is_absolute (path));

  /* Check contents can be loaded */
  g_assert_true (g_file_load_contents (file_path, NULL, &contents, NULL, NULL, NULL));
  g_free (contents);

  file_uri = builder_object_get_file_property (builder, "relative_uri");
  uri = g_file_get_uri (file_uri);
  g_assert_nonnull (uri);

  /* Check uri is absolute */
  g_assert_true (g_str_has_prefix (uri, "file://"));
  g_assert_true (g_path_is_absolute (uri + strlen("file://")));

  g_assert_true (g_file_load_contents (file_uri, NULL, &contents, NULL, NULL, NULL));
  g_free (contents);

  g_object_unref (builder);
  g_free (path);
  g_free (uri);
}

int
main (int argc, char **argv)
{
  /* initialize test program */
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/Builder/Parser", test_parser);
  g_test_add_func ("/Builder/Types", test_types);
  g_test_add_func ("/Builder/Construct-Only Properties", test_construct_only_property);
  g_test_add_func ("/Builder/Children", test_children);
  g_test_add_func ("/Builder/Layout Properties", test_layout_properties);
  g_test_add_func ("/Builder/Object Properties", test_object_properties);
  g_test_add_func ("/Builder/Notebook", test_notebook);
  g_test_add_func ("/Builder/Domain", test_domain);
  g_test_add_func ("/Builder/Signal Autoconnect", test_connect_signals);
  g_test_add_func ("/Builder/Spin Button", test_spin_button);
  g_test_add_func ("/Builder/SizeGroup", test_sizegroup);
  g_test_add_func ("/Builder/ListStore", test_list_store);
  g_test_add_func ("/Builder/TreeStore", test_tree_store);
  g_test_add_func ("/Builder/TreeView Column", test_treeview_column);
  g_test_add_func ("/Builder/IconView", test_icon_view);
  g_test_add_func ("/Builder/ComboBox", test_combo_box);
#if 0
  g_test_add_func ("/Builder/ComboBox Entry", test_combo_box_entry);
#endif
  g_test_add_func ("/Builder/CellView", test_cell_view);
  g_test_add_func ("/Builder/Dialog", test_dialog);
  g_test_add_func ("/Builder/Widget", test_widget);
  g_test_add_func ("/Builder/Value From String", test_value_from_string);
  g_test_add_func ("/Builder/Reference Counting", test_reference_counting);
  g_test_add_func ("/Builder/Window", test_window);
  g_test_add_func ("/Builder/PangoAttributes", test_pango_attributes);
  g_test_add_func ("/Builder/Requires", test_requires);
  g_test_add_func ("/Builder/AddObjects", test_add_objects);
  g_test_add_func ("/Builder/MessageArea", test_message_area);
  g_test_add_func ("/Builder/MessageDialog", test_message_dialog);
  g_test_add_func ("/Builder/GMenu", test_gmenu);
  g_test_add_func ("/Builder/LevelBar", test_level_bar);
  g_test_add_func ("/Builder/Expose Object", test_expose_object);
  g_test_add_func ("/Builder/Template", test_template);
  g_test_add_func ("/Builder/No IDs", test_no_ids);
  g_test_add_func ("/Builder/Property Bindings", test_property_bindings);
  g_test_add_func ("/Builder/anaconda-signal", test_anaconda_signal);
  g_test_add_func ("/Builder/FileFilter", test_file_filter);
  g_test_add_func ("/Builder/Shortcuts", test_shortcuts);
  g_test_add_func ("/Builder/Transforms", test_transforms);
  g_test_add_func ("/Builder/Expressions", test_expressions);
  g_test_add_func ("/Builder/Child Dispose Order", test_child_dispose_order);
  g_test_add_func ("/Builder/Buildable", test_buildable);
  g_test_add_func ("/Builder/Picture", test_picture);

  return g_test_run();
}

