/* BOBGUI Demo
 *
 * BOBGUI Demo is a collection of useful examples to demonstrate
 * BOBGUI widgets and features. It is a useful example in itself.
 *
 * You can select examples in the sidebar or search for them by
 * typing a search term. Double-clicking or hitting the “Run” button
 * will run the demo. The source code and other resources used in the
 * demo are shown in this area.
 *
 * You can also use the BOBGUI Inspector, available from the menu on the
 * top right, to poke at the running demos, and see how they are put
 * together.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include <bobgui/bobgui.h>
#include <gmodule.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "demos.h"
#include "fontify.h"

#include "profile_conf.h"

static BobguiWidget *info_view;
static BobguiWidget *source_view;

static char *current_file = NULL;

static BobguiWidget *notebook;
static BobguiSingleSelection *selection;
static BobguiWidget *toplevel;
static BobguiWidget *search_bar;
static BobguiWidget *search_entry;
static char **search_needle;

typedef struct _BobguiDemo BobguiDemo;
struct _BobguiDemo
{
  GObject parent_instance;

  const char *name;
  const char *title;
  const char **keywords;
  const char *filename;
  GDoDemoFunc func;
  GListModel *children_model;
};

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_NAME,
  PROP_TITLE,
  PROP_KEYWORDS,

  N_PROPS
};

# define BOBGUI_TYPE_DEMO (bobgui_demo_get_type ())
G_DECLARE_FINAL_TYPE (BobguiDemo, bobgui_demo, BOBGUI, DEMO, GObject);

G_DEFINE_TYPE (BobguiDemo, bobgui_demo, G_TYPE_OBJECT);
static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_demo_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BobguiDemo *self = BOBGUI_DEMO (object);

  switch (property_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_KEYWORDS:
      g_value_set_boxed (value, self->keywords);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void bobgui_demo_class_init (BobguiDemoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bobgui_demo_get_property;

  properties[PROP_FILENAME] =
    g_param_spec_string ("filename",
                         "filename",
                         "filename",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "name",
                         "name",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "title",
                         "title",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_KEYWORDS] =
    g_param_spec_string ("keywords",
                         "keywords",
                         "keywords",
                         NULL,
                         G_PARAM_READABLE);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void bobgui_demo_init (BobguiDemo *self)
{
}

typedef struct _CallbackData CallbackData;
struct _CallbackData
{
  BobguiTreeModel *model;
  BobguiTreePath *path;
};

static gboolean
bobgui_demo_run (BobguiDemo   *self,
              BobguiWidget *window)
{
  BobguiWidget *result;

  if (!self->func)
    return FALSE;

  result = self->func (window);
  if (result == NULL)
    return FALSE;

  return TRUE;
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  BobguiApplication *app = user_data;
  char *version;
  char *os_name;
  char *os_version;
  GString *s;
  GFile *logo_file;
  BobguiIconPaintable *logo;

  s = g_string_new ("");

  os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
  os_version = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);
  if (os_name && os_version)
    g_string_append_printf (s, "OS\t%s %s\n\n", os_name, os_version);
  g_string_append (s, "System libraries\n");
  g_string_append_printf (s, "\tGLib\t%d.%d.%d\n",
                          glib_major_version,
                          glib_minor_version,
                          glib_micro_version);
  g_string_append_printf (s, "\tPango\t%s\n",
                          pango_version_string ());
  g_string_append_printf (s, "\tBOBGUI \t%d.%d.%d\n",
                          bobgui_get_major_version (),
                          bobgui_get_minor_version (),
                          bobgui_get_micro_version ());
  g_string_append_printf (s, "\nA link can appear here: <http://www.bobgui.org>");

  version = g_strdup_printf ("%s%s%s\nRunning against BOBGUI %d.%d.%d",
                             PACKAGE_VERSION,
                             g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
                             g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "",
                             bobgui_get_major_version (),
                             bobgui_get_minor_version (),
                             bobgui_get_micro_version ());

  logo_file = g_file_new_for_uri ("resource:///org/bobgui/Demo4/icons/scalable/apps/org.bobgui.Demo4.svg");
  logo = bobgui_icon_paintable_new_for_file (logo_file, 64, 1);
  bobgui_show_about_dialog (BOBGUI_WINDOW (bobgui_application_get_active_window (app)),
                         "program-name", g_strcmp0 (PROFILE, "devel") == 0
                                         ? "BOBGUI Demo (Development)"
                                         : "BOBGUI Demo",
                         "version", version,
                         "copyright", "© 1997—2024 The BOBGUI Team",
                         "license-type", BOBGUI_LICENSE_LGPL_2_1,
                         "website", "http://www.bobgui.org",
                         "comments", "Program to demonstrate BOBGUI widgets",
                         "authors", (const char *[]) { "The BOBGUI Team", NULL },
                         "logo", logo,
                         "title", "About BOBGUI Demo",
                         "system-information", s->str,
                         NULL);
  g_object_unref (logo);
  g_object_unref (logo_file);

  g_string_free (s, TRUE);
  g_free (version);
  g_free (os_name);
  g_free (os_version);
}

static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiApplication *app = user_data;
  BobguiWidget *win;
  GList *list, *next;

  list = bobgui_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      bobgui_window_destroy (BOBGUI_WINDOW (win));

      list = next;
    }
}

static void
activate_inspector (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  bobgui_window_set_interactive_debugging (TRUE);
}

static void
activate_run (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       window)
{
  BobguiTreeListRow *row = bobgui_single_selection_get_selected_item (selection);
  BobguiDemo *demo = bobgui_tree_list_row_get_item (row);

  bobgui_demo_run (demo, window);
}

static BobguiWidget *
display_image (const char *format,
               const char *resource,
               BobguiWidget  *label)
{
  BobguiWidget *sw, *image;

  image = bobgui_picture_new_for_resource (resource);
  bobgui_widget_set_halign (image, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (image, BOBGUI_ALIGN_CENTER);
  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), image);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (image),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  return sw;
}

static BobguiWidget *
display_images (const char *format,
                const char *resource_dir,
                BobguiWidget  *label)
{
  char **resources;
  BobguiWidget *grid;
  BobguiWidget *sw;
  BobguiWidget *widget;
  guint i;

  resources = g_resources_enumerate_children (resource_dir, 0, NULL);
  if (resources == NULL)
    return NULL;

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  grid = bobgui_flow_box_new ();
  bobgui_flow_box_set_selection_mode (BOBGUI_FLOW_BOX (grid), BOBGUI_SELECTION_NONE);
  bobgui_widget_set_valign (grid, BOBGUI_ALIGN_START);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), grid);

  for (i = 0; resources[i]; i++)
    {
      char *resource_name;
      BobguiWidget *box;
      BobguiWidget *image_label;

      resource_name = g_strconcat (resource_dir, "/", resources[i], NULL);

      image_label = bobgui_label_new (resources[i]);
      widget = display_image (NULL, resource_name, image_label);
      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_append (BOBGUI_BOX (box), widget);
      bobgui_box_append (BOBGUI_BOX (box), image_label);
      bobgui_flow_box_insert (BOBGUI_FLOW_BOX (grid), box, -1);

      g_free (resource_name);
    }

  g_strfreev (resources);

  bobgui_label_set_label (BOBGUI_LABEL (label), "Images");

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (grid),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  return sw;
}

static BobguiWidget *
display_text (const char *format,
              const char *resource,
              BobguiWidget  *label)
{
  BobguiTextBuffer *buffer;
  BobguiWidget *textview, *sw;
  GBytes *bytes;
  const char *text;
  gsize len;

  bytes = g_resources_lookup_data (resource, 0, NULL);
  g_assert (bytes);
  text = g_bytes_get_data (bytes, &len);
  g_assert (g_utf8_validate (text, len, NULL));

  textview = bobgui_text_view_new ();
  bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (textview), 20);
  bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (textview), 20);
  bobgui_text_view_set_top_margin (BOBGUI_TEXT_VIEW (textview), 20);
  bobgui_text_view_set_bottom_margin (BOBGUI_TEXT_VIEW (textview), 20);
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (textview), FALSE);
  bobgui_text_view_set_cursor_visible (BOBGUI_TEXT_VIEW (textview), FALSE);
  /* Make it a bit nicer for text. */
  bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (textview), BOBGUI_WRAP_WORD);
  bobgui_text_view_set_pixels_above_lines (BOBGUI_TEXT_VIEW (textview), 2);
  bobgui_text_view_set_pixels_below_lines (BOBGUI_TEXT_VIEW (textview), 2);
  bobgui_text_view_set_monospace (BOBGUI_TEXT_VIEW (textview), TRUE);

  buffer = bobgui_text_buffer_new (NULL);
  bobgui_text_buffer_set_text (buffer, text, len);
  g_bytes_unref (bytes);

  if (format)
    fontify (format, buffer);

  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (textview), buffer);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (textview),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), textview);

  return sw;
}

static BobguiWidget *
display_video (const char *format,
               const char *resource,
               BobguiWidget  *label)
{
  BobguiWidget *video;

  video = bobgui_video_new_for_resource (resource);
  bobgui_video_set_loop (BOBGUI_VIDEO (video), TRUE);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (video),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  return video;
}

static BobguiWidget *
display_nothing (const char *resource)
{
  BobguiWidget *widget;
  char *str;

  str = g_strdup_printf ("The contents of the resource at '%s' cannot be displayed", resource);
  widget = bobgui_label_new (str);
  bobgui_label_set_wrap (BOBGUI_LABEL (widget), TRUE);

  g_free (str);

  return widget;
}

static struct {
  const char *extension;
  const char *format;
  BobguiWidget * (* display_func) (const char *format,
                                const char *resource,
                                BobguiWidget  *label);
} display_funcs[] = {
  { ".gif", NULL, display_image },
  { ".jpg", NULL, display_image },
  { ".png", NULL, display_image },
  { ".svg", NULL, display_image },
  { ".c", "c", display_text },
  { ".css", "css", display_text },
  { ".glsl", NULL, display_text },
  { ".h", "c", display_text },
  { ".txt", NULL, display_text },
  { ".ui", "xml", display_text },
  { ".webm", NULL, display_video },
  { "images/", NULL, display_images }
};

static void
add_data_tab (const char *demoname)
{
  char *resource_dir, *resource_name;
  char **resources;
  BobguiWidget *widget, *label;
  guint i, j;

  resource_dir = g_strconcat ("/", demoname, NULL);
  resources = g_resources_enumerate_children (resource_dir, 0, NULL);
  if (resources == NULL)
    {
      g_free (resource_dir);
      return;
    }

  for (i = 0; resources[i]; i++)
    {
      resource_name = g_strconcat (resource_dir, "/", resources[i], NULL);

      for (j = 0; j < G_N_ELEMENTS (display_funcs); j++)
        {
          if (g_str_has_suffix (resource_name, display_funcs[j].extension))
            break;
        }

      label = bobgui_label_new (resources[i]);

      if (j < G_N_ELEMENTS (display_funcs))
        widget = display_funcs[j].display_func (display_funcs[j].format,
                                                resource_name,
                                                label);
      else
        widget = display_nothing (resource_name);

      bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook), widget, label);
      g_object_set (bobgui_notebook_get_page (BOBGUI_NOTEBOOK (notebook), widget),
                    "tab-expand", FALSE,
                    NULL);

      g_free (resource_name);
    }

  g_strfreev (resources);
  g_free (resource_dir);
}

static void
remove_data_tabs (void)
{
  int i;

  for (i = bobgui_notebook_get_n_pages (BOBGUI_NOTEBOOK (notebook)) - 1; i > 1; i--)
    bobgui_notebook_remove_page (BOBGUI_NOTEBOOK (notebook), i);
}

void
load_file (const char *demoname,
           const char *filename)
{
  BobguiTextBuffer *info_buffer, *source_buffer;
  BobguiTextIter start, end;
  char *resource_filename;
  GError *err = NULL;
  int state = 0;
  gboolean in_para = 0;
  char **lines;
  GBytes *bytes;
  int i;

  if (!g_strcmp0 (current_file, filename))
    return;

  remove_data_tabs ();

  add_data_tab (demoname);

  g_free (current_file);
  current_file = g_strdup (filename);

  info_buffer = bobgui_text_buffer_new (NULL);
  bobgui_text_buffer_create_tag (info_buffer, "title",
                              "size", 18 * 1024,
                              "pixels-below-lines", 10,
                              NULL);

  source_buffer = bobgui_text_buffer_new (NULL);

  bobgui_text_buffer_begin_irreversible_action (info_buffer);
  bobgui_text_buffer_begin_irreversible_action (source_buffer);

  resource_filename = g_strconcat ("/sources/", filename, NULL);
  bytes = g_resources_lookup_data (resource_filename, 0, &err);
  g_free (resource_filename);

  if (bytes == NULL)
    {
      g_warning ("Cannot open source for %s: %s", filename, err->message);
      g_error_free (err);
      return;
    }

  lines = g_strsplit (g_bytes_get_data (bytes, NULL), "\n", -1);
  g_bytes_unref (bytes);

  bobgui_text_buffer_get_iter_at_offset (info_buffer, &start, 0);
  for (i = 0; lines[i] != NULL; i++)
    {
      char *p;
      char *q;
      char *r;

      /* Make sure \r is stripped at the end for the poor windows people */
      lines[i] = g_strchomp (lines[i]);

      p = lines[i];
      switch (state)
        {
        case 0:
          /* Reading title */
          while (*p == '/' || *p == '*' || g_ascii_isspace (*p))
            p++;
          r = p;
          while (*r != '\0')
            {
              while (*r != '/' && *r != ':' && *r != '\0')
                r++;
              if (*r == '/')
                {
                  r++;
                  p = r;
                }
              if (r[0] == ':' && r[1] == ':')
                *r = '\0';
            }
          q = p + strlen (p);
          while (q > p && g_ascii_isspace (*(q - 1)))
            q--;


          if (q > p)
            {
              int len_chars = g_utf8_pointer_to_offset (p, q);

              end = start;

              g_assert (strlen (p) >= q - p);
              bobgui_text_buffer_insert (info_buffer, &end, p, q - p);
              start = end;

              bobgui_text_iter_backward_chars (&start, len_chars);
              bobgui_text_buffer_apply_tag_by_name (info_buffer, "title", &start, &end);

              start = end;

              while (*p && *p != '\n') p++;

              state++;
            }
          break;

        case 1:
          /* Reading body of info section */
          while (g_ascii_isspace (*p))
            p++;
          if (*p == '*' && *(p + 1) == '/')
            {
              bobgui_text_buffer_get_iter_at_offset (source_buffer, &start, 0);
              state++;
            }
          else
            {
              int len;

              while (*p == '*' || g_ascii_isspace (*p))
                p++;

              len = strlen (p);
              while (g_ascii_isspace (*(p + len - 1)))
                len--;

              if (*p == '#')
                break;

              if (len > 0)
                {
                  if (in_para)
                    bobgui_text_buffer_insert (info_buffer, &start, " ", 1);

                  g_assert (strlen (p) >= len);
                  bobgui_text_buffer_insert (info_buffer, &start, p, len);
                  in_para = 1;
                }
              else
                {
                  bobgui_text_buffer_insert (info_buffer, &start, "\n", 1);
                  in_para = 0;
                }
            }
          break;

        case 2:
          /* Skipping blank lines */
          while (g_ascii_isspace (*p))
            p++;

          if (!*p)
            break;

          p = lines[i];
          state++;
          G_GNUC_FALLTHROUGH;

        case 3:
          /* Reading program body */
          bobgui_text_buffer_insert (source_buffer, &start, p, -1);
          if (lines[i+1] != NULL)
            bobgui_text_buffer_insert (source_buffer, &start, "\n", 1);
          break;

        default:
          g_assert_not_reached ();
        }
    }

  g_strfreev (lines);

  fontify ("c", source_buffer);

  bobgui_text_buffer_end_irreversible_action (source_buffer);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (source_view), source_buffer);
  g_object_unref (source_buffer);

  bobgui_text_buffer_end_irreversible_action (info_buffer);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (info_view), info_buffer);
  g_object_unref (info_buffer);
}

static void
activate_cb (BobguiWidget *widget,
             guint      position,
             gpointer   window)
{
  GListModel *model = G_LIST_MODEL (bobgui_list_view_get_model (BOBGUI_LIST_VIEW (widget)));
  BobguiTreeListRow *row = g_list_model_get_item (model, position);
  BobguiDemo *demo = bobgui_tree_list_row_get_item (row);

  bobgui_demo_run (demo, window);

  g_object_unref (row);
}

static void
selection_cb (BobguiSingleSelection *sel,
              GParamSpec         *pspec,
              gpointer            user_data)
{
  BobguiTreeListRow *row = bobgui_single_selection_get_selected_item (sel);
  BobguiDemo *demo;
  GAction *action;

  bobgui_widget_set_sensitive (BOBGUI_WIDGET (notebook), !!row);

  if (!row)
    {
      bobgui_window_set_title (BOBGUI_WINDOW (toplevel), "No match");

      return;
    }

  demo = bobgui_tree_list_row_get_item (row);

  if (demo->filename)
    load_file (demo->name, demo->filename);

  action = g_action_map_lookup_action (G_ACTION_MAP (toplevel), "run");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), demo->func != NULL);

  bobgui_window_set_title (BOBGUI_WINDOW (toplevel), demo->title);
}

static gboolean
filter_demo (BobguiDemo *demo)
{
  int i;

  /* Show only if the name matches every needle */
  for (i = 0; search_needle[i]; i++)
    {
      if (!demo->title)
        return FALSE;

      if (g_str_match_string (search_needle[i], demo->title, TRUE))
        continue;

      if (demo->keywords)
        {
          int j;
          gboolean found = FALSE;

          for (j = 0; !found && demo->keywords[j]; j++)
            {
              if (strstr (demo->keywords[j], search_needle[i]))
                found = TRUE;
            }

          if (found)
            continue;
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
demo_filter_by_name (gpointer item,
                     gpointer user_data)
{
  BobguiTreeListRow *row = item;
  GListModel *children;
  BobguiDemo *demo;
  guint i, n;
  BobguiTreeListRow *parent;

  /* Show all items if search is empty */
  if (!search_needle || !search_needle[0] || !*search_needle[0])
    return TRUE;

  g_assert (BOBGUI_IS_TREE_LIST_ROW (row));
  g_assert (BOBGUI_IS_FILTER_LIST_MODEL (user_data));

  /* Show a row if itself of any parent matches */
  for (parent = row; parent; parent = bobgui_tree_list_row_get_parent (parent))
    {
      demo = bobgui_tree_list_row_get_item (parent);
      g_assert (BOBGUI_IS_DEMO (demo));

      if (filter_demo (demo))
        return TRUE;
    }

  /* Show a row if any child matches */
  children = bobgui_tree_list_row_get_children (row);
  if (children)
    {
      n = g_list_model_get_n_items (children);
      for (i = 0; i < n; i++)
        {
          demo = g_list_model_get_item (children, i);
          g_assert (BOBGUI_IS_DEMO (demo));

          if (filter_demo (demo))
            {
              g_object_unref (demo);
              return TRUE;
            }
          g_object_unref (demo);
        }
    }

  return FALSE;
}

static void
demo_search_changed_cb (BobguiSearchEntry *entry,
                        BobguiFilter      *filter)
{
  const char *text;

  g_assert (BOBGUI_IS_SEARCH_ENTRY (entry));
  g_assert (BOBGUI_IS_FILTER (filter));

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  g_clear_pointer (&search_needle, g_strfreev);

  if (text && *text)
    search_needle = g_str_tokenize_and_fold (text, NULL, NULL);

  bobgui_filter_changed (filter, BOBGUI_FILTER_CHANGE_DIFFERENT);
}

static gboolean
demo_can_run (BobguiWidget  *window,
              const char *name)
{
  return TRUE;
}

static GListModel *
create_demo_model (BobguiWidget *window)
{
  GListStore *store = g_list_store_new (BOBGUI_TYPE_DEMO);
  DemoData *demo = bobgui_demos;
  BobguiDemo *d;

  bobgui_widget_realize (window);

  d = BOBGUI_DEMO (g_object_new (BOBGUI_TYPE_DEMO, NULL));
  d->name = "main";
  d->title = "BOBGUI Demo";
  d->keywords = NULL;
  d->filename = "main.c";
  d->func = NULL;

  g_list_store_append (store, d);

  while (demo->title)
    {
       DemoData *children = demo->children;

      if (demo_can_run (window, demo->name))
        {
          d = BOBGUI_DEMO (g_object_new (BOBGUI_TYPE_DEMO, NULL));

          d->name = demo->name;
          d->title = demo->title;
          d->keywords = demo->keywords;
          d->filename = demo->filename;
          d->func = demo->func;

          g_list_store_append (store, d);
        }

      if (children)
        {
          d->children_model = G_LIST_MODEL (g_list_store_new (BOBGUI_TYPE_DEMO));

          while (children->title)
            {
              if (demo_can_run (window, children->name))
                {
                  BobguiDemo *child = BOBGUI_DEMO (g_object_new (BOBGUI_TYPE_DEMO, NULL));

                  child->name = children->name;
                  child->title = children->title;
                  child->keywords = children->keywords;
                  child->filename = children->filename;
                  child->func = children->func;

                  g_list_store_append (G_LIST_STORE (d->children_model), child);
                }

              children++;
            }
        }

      demo++;
    }

  return G_LIST_MODEL (store);
}

static GListModel *
get_child_model (gpointer item,
                 gpointer user_data)
{
  BobguiDemo *demo = item;

  if (demo->children_model)
    return g_object_ref (G_LIST_MODEL (demo->children_model));

  return NULL;
}

static void
clear_search (BobguiSearchBar *bar)
{
  if (!bobgui_search_bar_get_search_mode (bar))
    {
      BobguiWidget *entry = bobgui_search_bar_get_child (BOBGUI_SEARCH_BAR (bar));
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "");
    }
}

static void
search_results_update (GObject    *filter_model,
                       GParamSpec *pspec,
                       BobguiEntry   *entry)
{
  gsize n_items = g_list_model_get_n_items (G_LIST_MODEL (filter_model));

  if (strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))) > 0)
    {
      char *text;

      if (n_items > 0)
        text = g_strdup_printf (ngettext ("%ld search result", "%ld search results", (long) n_items), (long) n_items);
      else
        text = g_strdup (_("No search results"));

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, text,
                                      -1);

      g_free (text);
    }
  else
    {
      bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (entry), BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION);
    }
}

static gboolean
save_state_cb (BobguiApplicationWindow *window,
               GVariantDict         *state)
{
  guint selected;
  int current;

  g_variant_dict_insert_value (state, "search-bar-visible", g_variant_new_boolean (bobgui_search_bar_get_search_mode (BOBGUI_SEARCH_BAR (search_bar))));

  g_variant_dict_insert_value (state, "search-string", g_variant_new_string (bobgui_editable_get_text (BOBGUI_EDITABLE (search_entry))));

  selected = bobgui_single_selection_get_selected (selection);
  g_variant_dict_insert_value (state, "selected-item", g_variant_new_uint32 (selected));

  current = bobgui_notebook_get_current_page (BOBGUI_NOTEBOOK (notebook));
  g_variant_dict_insert_value (state, "current-page", g_variant_new_int32 (current));

  return TRUE;
}

static gboolean
select_item (gpointer data)
{
  bobgui_single_selection_set_selected (selection, GPOINTER_TO_UINT (data));
  return G_SOURCE_REMOVE;
}

static gboolean
select_page (gpointer data)
{
  bobgui_notebook_set_current_page (BOBGUI_NOTEBOOK (notebook), GPOINTER_TO_INT (data));
  return G_SOURCE_REMOVE;
}

static void
restore_state (BobguiApplicationWindow *window,
               BobguiRestoreReason      reason,
               GVariant             *state)
{
  gboolean visible;
  guint selected;
  const char *string;
  int current;

  if (!state)
    return;

  if (reason != BOBGUI_RESTORE_REASON_RESTORE)
    return;

  g_variant_lookup (state, "search-bar-visible", "b", &visible);
  bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (search_bar), visible);

  g_variant_lookup (state, "search-string", "&s", &string);
  if (string)
    {
      bobgui_editable_set_text (BOBGUI_EDITABLE (search_entry), string);
      bobgui_editable_set_position (BOBGUI_EDITABLE (search_entry), -1);
    }

  /* Wait for the search string filtering to take effect before
   * applying the selected-item
   */
  g_variant_lookup (state, "selected-item", "u", &selected);
  g_timeout_add (160, select_item, GUINT_TO_POINTER (selected));

  /* Wait for the selected-item to update the notebook before
   * applying the current-page
   */
  g_variant_lookup (state, "current-page", "i", &current);
  g_timeout_add (320, select_page , GINT_TO_POINTER (current));
}

static BobguiWindow *
create_window (BobguiApplication *app)
{
  BobguiBuilder *builder;
  GListModel *listmodel;
  BobguiTreeListModel *treemodel;
  BobguiWidget *window, *listview;
  BobguiFilterListModel *filter_model;
  BobguiFilter *filter;
  GSimpleAction *action;

  builder = bobgui_builder_new_from_resource ("/ui/main.ui");

  window = (BobguiWidget *) bobgui_builder_get_object (builder, "window");

  g_signal_connect (window, "save-state", G_CALLBACK (save_state_cb), NULL);

  bobgui_application_add_window (app, BOBGUI_WINDOW (window));

  if (g_strcmp0 (PROFILE, "devel") == 0)
    bobgui_widget_add_css_class (window, "devel");

  action = g_simple_action_new ("run", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (activate_run), window);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  g_clear_pointer (&current_file, g_free);
  notebook = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "notebook"));

  info_view = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "info-textview"));
  source_view = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "source-textview"));
  toplevel = BOBGUI_WIDGET (window);
  listview = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "listview"));
  g_signal_connect (listview, "activate", G_CALLBACK (activate_cb), window);
  search_bar = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "searchbar"));
  g_signal_connect (search_bar, "notify::search-mode-enabled", G_CALLBACK (clear_search), NULL);

  listmodel = create_demo_model (window);
  treemodel = bobgui_tree_list_model_new (G_LIST_MODEL (listmodel),
                                       FALSE,
                                       TRUE,
                                       get_child_model,
                                       NULL,
                                       NULL);
  filter_model = bobgui_filter_list_model_new (G_LIST_MODEL (treemodel), NULL);
  filter = BOBGUI_FILTER (bobgui_custom_filter_new (demo_filter_by_name, filter_model, NULL));
  bobgui_filter_list_model_set_filter (filter_model, filter);
  g_object_unref (filter);

  search_entry = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "search-entry"));
  g_signal_connect (search_entry, "search-changed", G_CALLBACK (demo_search_changed_cb), filter);
  g_signal_connect (filter_model, "notify::n-items", G_CALLBACK (search_results_update), search_entry);

  selection = bobgui_single_selection_new (G_LIST_MODEL (filter_model));
  g_signal_connect (selection, "notify::selected-item", G_CALLBACK (selection_cb), NULL);
  bobgui_list_view_set_model (BOBGUI_LIST_VIEW (listview), BOBGUI_SELECTION_MODEL (selection));

  selection_cb (selection, NULL, NULL);
  g_object_unref (selection);

  g_object_unref (builder);

  return BOBGUI_WINDOW (window);
}

static void
restore_window (BobguiApplication   *app,
                BobguiRestoreReason  reason,
                GVariant         *state)
{
  BobguiWindow *window;

  window = create_window (app);
  restore_state (BOBGUI_APPLICATION_WINDOW (window), reason, state);
}

static void
activate (GApplication *gapp)
{
  BobguiApplication *app = BOBGUI_APPLICATION (gapp);
  GList *list;
  BobguiWindow *window;

  list = bobgui_application_get_windows (app);
  if (list)
    window = list->data;
  else
    window = create_window (app);

  bobgui_window_present (window);
}

static gboolean
auto_quit (gpointer data)
{
  g_application_quit (G_APPLICATION (data));
  return G_SOURCE_REMOVE;
}

static void
list_demos (void)
{
  DemoData *d, *c;

  d = bobgui_demos;

  while (d->title)
    {
      c = d->children;
      if (d->name)
        g_print ("%s\n", d->name);
      d++;
      while (c && c->title)
        {
          if (c->name)
            g_print ("%s\n", c->name);
          c++;
        }
    }
}

static int
command_line (GApplication            *app,
              GApplicationCommandLine *cmdline)
{
  GVariantDict *options;
  const char *name = NULL;
  gboolean autoquit = FALSE;
  gboolean list = FALSE;
  DemoData *d, *c;
  GDoDemoFunc func = 0;
  BobguiWidget *window, *demo;

  options = g_application_command_line_get_options_dict (cmdline);
  g_variant_dict_lookup (options, "run", "&s", &name);
  g_variant_dict_lookup (options, "autoquit", "b", &autoquit);
  g_variant_dict_lookup (options, "list", "b", &list);

  if (autoquit)
    g_timeout_add_seconds (1, auto_quit, app);

  if (!name && !list)
    {
      g_application_activate (app);
      return 0;
    }

  create_window (BOBGUI_APPLICATION (app));

  if (list)
    {
      list_demos ();
      g_application_quit (app);
      return 0;
    }

  window = bobgui_application_get_windows (BOBGUI_APPLICATION (app))->data;

  if (name == NULL)
    goto out;

  d = bobgui_demos;

  while (d->title)
    {
      c = d->children;
      if (g_strcmp0 (d->name, name) == 0)
        {
          func = d->func;
          goto out;
        }
      d++;
      while (c && c->title)
        {
          if (g_strcmp0 (c->name, name) == 0)
            {
              func = c->func;
              goto out;
            }
          c++;
        }
    }

out:
  if (func)
    {
      demo = (func) (window);

      bobgui_window_set_transient_for (BOBGUI_WINDOW (demo), BOBGUI_WINDOW (window));
      bobgui_widget_set_visible (window, FALSE);

      g_signal_connect_swapped (G_OBJECT (demo), "destroy", G_CALLBACK (g_application_quit), app);
    }

  return 0;
}

G_MODULE_EXPORT
int
main (int argc, char **argv)
{
  BobguiApplication *app;
  static GActionEntry app_entries[] = {
    { "about", activate_about, NULL, NULL, NULL },
    { "quit", activate_quit, NULL, NULL, NULL },
    { "inspector", activate_inspector, NULL, NULL, NULL },
  };
  struct {
    const char *action_and_target;
    const char *accelerators[2];
  } accels[] = {
    { "app.about", { "F1", NULL } },
    { "app.quit", { "<Control>q", NULL } },
  };
  int i;
  char version[80];

  bobgui_init ();

  app = bobgui_application_new ("org.bobgui.Demo4", G_APPLICATION_NON_UNIQUE|G_APPLICATION_HANDLES_COMMAND_LINE);
  g_object_set (app, "support-save", TRUE, NULL);

  g_snprintf (version, sizeof (version), "%s%s%s\n",
              PACKAGE_VERSION,
              g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
              g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "");

  g_application_set_version (G_APPLICATION (app), version);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  for (i = 0; i < G_N_ELEMENTS (accels); i++)
    bobgui_application_set_accels_for_action (app, accels[i].action_and_target, accels[i].accelerators);

  g_application_add_main_option (G_APPLICATION (app), "run", 0, 0, G_OPTION_ARG_STRING, "Run an example", "EXAMPLE");
  g_application_add_main_option (G_APPLICATION (app), "list", 0, 0, G_OPTION_ARG_NONE, "List examples", NULL);
  g_application_add_main_option (G_APPLICATION (app), "autoquit", 0, 0, G_OPTION_ARG_NONE, "Quit after a delay", NULL);

  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "restore-window", G_CALLBACK (restore_window), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return 0;
}
