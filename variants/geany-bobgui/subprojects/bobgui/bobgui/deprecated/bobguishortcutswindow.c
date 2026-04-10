/* bobguishortcutswindow.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguishortcutswindowprivate.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguigrid.h"
#include "bobguiheaderbar.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguilistbox.h"
#include "bobguimain.h"
#include "bobguimenubutton.h"
#include "bobguipopover.h"
#include "bobguiprivate.h"
#include "bobguiscrolledwindow.h"
#include "bobguisearchbar.h"
#include "bobguisearchentry.h"
#include "bobguishortcutssection.h"
#include "bobguishortcutsgroup.h"
#include "bobguishortcutsshortcutprivate.h"
#include "bobguisizegroup.h"
#include "bobguistack.h"
#include "bobguitogglebutton.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiShortcutsWindow:
 *
 * A `BobguiShortcutsWindow` shows information about the keyboard shortcuts
 * and gestures of an application.
 *
 * The shortcuts can be grouped, and you can have multiple sections in this
 * window, corresponding to the major modes of your application.
 *
 * Additionally, the shortcuts can be filtered by the current view, to avoid
 * showing information that is not relevant in the current application context.
 *
 * The recommended way to construct a `BobguiShortcutsWindow` is with
 * [class@Bobgui.Builder], by using the `<child>` tag to populate a
 * `BobguiShortcutsWindow` with one or more [class@Bobgui.ShortcutsSection] objects,
 * which contain one or more [class@Bobgui.ShortcutsGroup] instances, which, in turn,
 * contain [class@Bobgui.ShortcutsShortcut] instances.
 *
 * If you need to add a section programmatically, use [method@Bobgui.ShortcutsWindow.add_section]
 * instead of [method@Bobgui.Window.set_child], as the shortcuts window manages
 * its children directly.
 *
 * # A simple example:
 *
 * <picture>
 *   <source srcset="gedit-shortcuts-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="A simple example" src="gedit-shortcuts.png">
 * </picture>
 *
 * This example has as single section. As you can see, the shortcut groups
 * are arranged in columns, and spread across several pages if there are too
 * many to find on a single page.
 *
 * The .ui file for this example can be found [here](https://gitlab.gnome.org/GNOME/bobgui/tree/main/demos/bobgui-demo/shortcuts-gedit.ui).
 *
 * # An example with multiple views:
 *
 * <picture>
 *   <source srcset="clocks-shortcuts-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example with multiple views" src="clocks-shortcuts.png">
 * </picture>
 *
 * This example shows a `BobguiShortcutsWindow` that has been configured to show only
 * the shortcuts relevant to the “Stopwatch” view.
 *
 * The .ui file for this example can be found [here](https://gitlab.gnome.org/GNOME/bobgui/tree/main/demos/bobgui-demo/shortcuts-clocks.ui).
 *
 * # An example with multiple sections:
 *
 * <picture>
 *   <source srcset="builder-shortcuts-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example with multiple sections" src="builder-shortcuts.png">
 * </picture>
 *
 * This example shows a `BobguiShortcutsWindow` with two sections, “Editor Shortcuts”
 * and “Terminal Shortcuts”.
 *
 * The .ui file for this example can be found [here](https://gitlab.gnome.org/GNOME/bobgui/tree/main/demos/bobgui-demo/shortcuts-builder.ui).
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.ShortcutsWindow::close]
 * - [signal@Bobgui.ShortcutsWindow::search]
 *
 * # CSS nodes
 *
 * `BobguiShortcutsWindow` has a single CSS node with the name `window` and style
 * class `.shortcuts`.
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */

struct _BobguiShortcutsWindow
{
  BobguiWindow       parent_instance;

  GHashTable     *keywords;
  char           *initial_section;
  char           *last_section_name;
  char           *view_name;
  BobguiSizeGroup   *search_text_group;
  BobguiSizeGroup   *search_image_group;
  GHashTable     *search_items_hash;

  BobguiStack       *stack;
  BobguiStack       *title_stack;
  BobguiMenuButton  *menu_button;
  BobguiSearchBar   *search_bar;
  BobguiSearchEntry *search_entry;
  BobguiHeaderBar   *header_bar;
  BobguiWidget      *main_box;
  BobguiPopover     *popover;
  BobguiListBox     *list_box;
  BobguiBox         *search_gestures;
  BobguiBox         *search_shortcuts;

  BobguiWindow      *window;
};

typedef struct
{
  BobguiWindowClass parent_class;

  void (*close)  (BobguiShortcutsWindow *self);
  void (*search) (BobguiShortcutsWindow *self);
} BobguiShortcutsWindowClass;

typedef struct
{
  BobguiShortcutsWindow *self;
  BobguiBuilder        *builder;
  GQueue            *stack;
  char              *property_name;
  guint              translatable : 1;
} ViewsParserData;

static void bobgui_shortcuts_window_buildable_iface_init (BobguiBuildableIface *iface);


G_DEFINE_TYPE_WITH_CODE (BobguiShortcutsWindow, bobgui_shortcuts_window, BOBGUI_TYPE_WINDOW,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_shortcuts_window_buildable_iface_init))


enum {
  CLOSE,
  SEARCH,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SECTION_NAME,
  PROP_VIEW_NAME,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];
static guint signals[LAST_SIGNAL];


static gboolean
more_than_three_children (BobguiWidget *widget)
{
  BobguiWidget *child;
  int i;

  for (child = bobgui_widget_get_first_child (widget), i = 0;
       child != NULL;
       child = bobgui_widget_get_next_sibling (child), i++)
    {
      if (i == 3)
        return TRUE;
    }

  return FALSE;
}

static void
update_title_stack (BobguiShortcutsWindow *self)
{
  BobguiWidget *visible_child;

  visible_child = bobgui_stack_get_visible_child (self->stack);

  if (BOBGUI_IS_SHORTCUTS_SECTION (visible_child))
    {
      if (more_than_three_children (BOBGUI_WIDGET (self->stack)))
        {
          char *title;

          bobgui_stack_set_visible_child_name (self->title_stack, "sections");
          g_object_get (visible_child, "title", &title, NULL);
          bobgui_menu_button_set_label (self->menu_button, title);
          g_free (title);
        }
      else
        {
          bobgui_stack_set_visible_child_name (self->title_stack, "title");
        }
    }
  else if (visible_child != NULL)
    {
      bobgui_stack_set_visible_child_name (self->title_stack, "search");
    }
}

static void
bobgui_shortcuts_window_add_search_item (BobguiWidget *child, gpointer data)
{
  BobguiShortcutsWindow *self = data;
  BobguiWidget *item;
  char *accelerator = NULL;
  char *title = NULL;
  char *hash_key = NULL;
  GIcon *icon = NULL;
  gboolean icon_set = FALSE;
  gboolean subtitle_set = FALSE;
  BobguiTextDirection direction;
  BobguiShortcutType shortcut_type;
  char *action_name = NULL;
  char *subtitle;
  char *str;
  char *keywords;

  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (child))
    {
      GEnumClass *class;
      GEnumValue *value;

      g_object_get (child,
                    "accelerator", &accelerator,
                    "title", &title,
                    "direction", &direction,
                    "icon-set", &icon_set,
                    "subtitle-set", &subtitle_set,
                    "shortcut-type", &shortcut_type,
                    "action-name", &action_name,
                    NULL);

      class = G_ENUM_CLASS (g_type_class_ref (BOBGUI_TYPE_SHORTCUT_TYPE));
      value = g_enum_get_value (class, shortcut_type);

      hash_key = g_strdup_printf ("%s-%s-%s", title, value->value_nick, accelerator);

      g_type_class_unref (class);

      if (g_hash_table_contains (self->search_items_hash, hash_key))
        {
          g_free (hash_key);
          g_free (title);
          g_free (accelerator);
          return;
        }

      g_hash_table_insert (self->search_items_hash, hash_key, GINT_TO_POINTER (1));

      item = g_object_new (BOBGUI_TYPE_SHORTCUTS_SHORTCUT,
                           "accelerator", accelerator,
                           "title", title,
                           "direction", direction,
                           "shortcut-type", shortcut_type,
                           "accel-size-group", self->search_image_group,
                           "title-size-group", self->search_text_group,
                           "action-name", action_name,
                           NULL);
      if (icon_set)
        {
          g_object_get (child, "icon", &icon, NULL);
          g_object_set (item, "icon", icon, NULL);
          g_clear_object (&icon);
        }
      if (subtitle_set)
        {
          g_object_get (child, "subtitle", &subtitle, NULL);
          g_object_set (item, "subtitle", subtitle, NULL);
          g_free (subtitle);
        }
      str = g_strdup_printf ("%s %s", accelerator, title);
      keywords = g_utf8_strdown (str, -1);

      g_hash_table_insert (self->keywords, item, keywords);
      if (shortcut_type == BOBGUI_SHORTCUT_ACCELERATOR)
        bobgui_box_append (BOBGUI_BOX (self->search_shortcuts), item);
      else
        bobgui_box_append (BOBGUI_BOX (self->search_gestures), item);

      g_free (title);
      g_free (accelerator);
      g_free (str);
      g_free (action_name);
    }
  else
    {
      BobguiWidget *widget;

      for (widget = bobgui_widget_get_first_child (child);
           widget != NULL;
           widget = bobgui_widget_get_next_sibling (widget))
        bobgui_shortcuts_window_add_search_item (widget, self);
    }
}

static void
section_notify_cb (GObject    *section,
                   GParamSpec *pspec,
                   gpointer    data)
{
  BobguiShortcutsWindow *self = data;

  if (strcmp (pspec->name, "section-name") == 0)
    {
      char *name;

      g_object_get (section, "section-name", &name, NULL);
      g_object_set (bobgui_stack_get_page (self->stack, BOBGUI_WIDGET (section)), "name", name, NULL);
      g_free (name);
    }
  else if (strcmp (pspec->name, "title") == 0)
    {
      char *title;
      BobguiWidget *label;

      label = g_object_get_data (section, "bobgui-shortcuts-title");
      g_object_get (section, "title", &title, NULL);
      bobgui_label_set_label (BOBGUI_LABEL (label), title);
      g_free (title);
    }
}

/**
 * bobgui_shortcuts_window_add_section:
 * @self: a `BobguiShortcutsWindow`
 * @section: the `BobguiShortcutsSection` to add
 *
 * Adds a section to the shortcuts window.
 *
 * This is the programmatic equivalent to using [class@Bobgui.Builder] and a
 * `<child>` tag to add the child.
 *
 * Using [method@Bobgui.Window.set_child] is not appropriate as the shortcuts
 * window manages its children internally.
 *
 * Since: 4.14
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */
void
bobgui_shortcuts_window_add_section (BobguiShortcutsWindow  *self,
                                  BobguiShortcutsSection *section)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_WINDOW (self));
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_SECTION (section));
  g_return_if_fail (bobgui_widget_get_parent (BOBGUI_WIDGET (section)) == NULL);

  BobguiListBoxRow *row;
  char *title;
  char *name;
  const char *visible_section;
  BobguiWidget *label;
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (section));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    bobgui_shortcuts_window_add_search_item (child, self);

  g_object_get (section,
                "section-name", &name,
                "title", &title,
                NULL);

  g_signal_connect (section, "notify", G_CALLBACK (section_notify_cb), self);

  if (name == NULL)
    name = g_strdup ("shortcuts");

  bobgui_stack_add_titled (self->stack, BOBGUI_WIDGET (section), name, title);

  visible_section = bobgui_stack_get_visible_child_name (self->stack);
  if (strcmp (visible_section, "internal-search") == 0 ||
      (self->initial_section && strcmp (self->initial_section, visible_section) == 0))
    bobgui_stack_set_visible_child (self->stack, BOBGUI_WIDGET (section));

  row = g_object_new (BOBGUI_TYPE_LIST_BOX_ROW,
                      NULL);
  g_object_set_data (G_OBJECT (row), "bobgui-shortcuts-section", section);
  label = g_object_new (BOBGUI_TYPE_LABEL,
                        "margin-start", 6,
                        "margin-end", 6,
                        "margin-top", 6,
                        "margin-bottom", 6,
                        "label", title,
                        "xalign", 0.5f,
                        NULL);
  g_object_set_data (G_OBJECT (section), "bobgui-shortcuts-title", label);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), BOBGUI_WIDGET (label));
  bobgui_list_box_insert (BOBGUI_LIST_BOX (self->list_box), BOBGUI_WIDGET (row), -1);

  update_title_stack (self);

  g_free (name);
  g_free (title);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_shortcuts_window_buildable_add_child (BobguiBuildable *buildable,
                                          BobguiBuilder   *builder,
                                          GObject      *child,
                                          const char   *type)
{
  if (BOBGUI_IS_SHORTCUTS_SECTION (child))
    bobgui_shortcuts_window_add_section (BOBGUI_SHORTCUTS_WINDOW (buildable),
                                      BOBGUI_SHORTCUTS_SECTION (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_shortcuts_window_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_shortcuts_window_buildable_add_child;
}

static void
bobgui_shortcuts_window_set_view_name (BobguiShortcutsWindow *self,
                                    const char         *view_name)
{
  BobguiWidget *section;

  g_free (self->view_name);
  self->view_name = g_strdup (view_name);

  for (section = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->stack));
       section != NULL;
       section = bobgui_widget_get_next_sibling (section))
    {
      if (BOBGUI_IS_SHORTCUTS_SECTION (section))
        g_object_set (section, "view-name", self->view_name, NULL);
    }
}

static void
bobgui_shortcuts_window_set_section_name (BobguiShortcutsWindow *self,
                                       const char         *section_name)
{
  BobguiWidget *section = NULL;

  g_free (self->initial_section);
  self->initial_section = g_strdup (section_name);

  if (section_name)
    section = bobgui_stack_get_child_by_name (self->stack, section_name);
  if (section)
    bobgui_stack_set_visible_child (self->stack, section);
}

static void
update_accels_cb (BobguiWidget *widget,
                  gpointer   data)
{
  BobguiShortcutsWindow *self = data;

  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (widget))
    bobgui_shortcuts_shortcut_update_accel (BOBGUI_SHORTCUTS_SHORTCUT (widget), self->window);
  else
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (widget));
           child != NULL;
           child = bobgui_widget_get_next_sibling (child ))
        update_accels_cb (child, self);
    }
}

static void
update_accels_for_actions (BobguiShortcutsWindow *self)
{
  if (self->window)
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        update_accels_cb (child, self);
    }
}

void
bobgui_shortcuts_window_set_window (BobguiShortcutsWindow *self,
                                 BobguiWindow          *window)
{
  self->window = window;

  update_accels_for_actions (self);
}

static void
bobgui_shortcuts_window__list_box__row_activated (BobguiShortcutsWindow *self,
                                               BobguiListBoxRow      *row,
                                               BobguiListBox         *list_box)
{
  BobguiWidget *section;

  section = g_object_get_data (G_OBJECT (row), "bobgui-shortcuts-section");
  bobgui_stack_set_visible_child (self->stack, section);
  bobgui_popover_popdown (self->popover);
}

static gboolean
hidden_by_direction (BobguiWidget *widget)
{
  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (widget))
    {
      BobguiTextDirection dir;

      g_object_get (widget, "direction", &dir, NULL);
      if (dir != BOBGUI_TEXT_DIR_NONE &&
          dir != bobgui_widget_get_direction (widget))
        return TRUE;
    }

  return FALSE;
}

static void
bobgui_shortcuts_window__entry__changed (BobguiShortcutsWindow *self,
                                     BobguiSearchEntry      *search_entry)
{
  char *downcase = NULL;
  GHashTableIter iter;
  const char *text;
  const char *last_section_name;
  gpointer key;
  gpointer value;
  gboolean has_result;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (search_entry));

  if (!text || !*text)
    {
      if (self->last_section_name != NULL)
        {
          bobgui_stack_set_visible_child_name (self->stack, self->last_section_name);
          return;

        }
    }

  last_section_name = bobgui_stack_get_visible_child_name (self->stack);

  if (g_strcmp0 (last_section_name, "internal-search") != 0 &&
      g_strcmp0 (last_section_name, "no-search-results") != 0)
    {
      g_free (self->last_section_name);
      self->last_section_name = g_strdup (last_section_name);
    }

  downcase = g_utf8_strdown (text, -1);

  g_hash_table_iter_init (&iter, self->keywords);

  has_result = FALSE;
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      BobguiWidget *widget = key;
      const char *keywords = value;
      gboolean match;

      if (hidden_by_direction (widget))
        match = FALSE;
      else
        match = strstr (keywords, downcase) != NULL;

      bobgui_widget_set_visible (widget, match);
      has_result |= match;
    }

  g_free (downcase);

  if (has_result)
    bobgui_stack_set_visible_child_name (self->stack, "internal-search");
  else
    bobgui_stack_set_visible_child_name (self->stack, "no-search-results");
}

static void
bobgui_shortcuts_window__search_mode__changed (BobguiShortcutsWindow *self)
{
  if (!bobgui_search_bar_get_search_mode (self->search_bar))
    {
      if (self->last_section_name != NULL)
        bobgui_stack_set_visible_child_name (self->stack, self->last_section_name);
    }
}

static void
bobgui_shortcuts_window_close (BobguiShortcutsWindow *self)
{
  bobgui_window_close (BOBGUI_WINDOW (self));
}

static void
bobgui_shortcuts_window_search (BobguiShortcutsWindow *self)
{
  bobgui_search_bar_set_search_mode (self->search_bar, TRUE);
}

static void
bobgui_shortcuts_window_constructed (GObject *object)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)object;

  G_OBJECT_CLASS (bobgui_shortcuts_window_parent_class)->constructed (object);

  if (self->initial_section != NULL)
    bobgui_stack_set_visible_child_name (self->stack, self->initial_section);
}

static void
bobgui_shortcuts_window_finalize (GObject *object)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)object;

  g_clear_pointer (&self->keywords, g_hash_table_unref);
  g_clear_pointer (&self->initial_section, g_free);
  g_clear_pointer (&self->view_name, g_free);
  g_clear_pointer (&self->last_section_name, g_free);
  g_clear_pointer (&self->search_items_hash, g_hash_table_unref);

  g_clear_object (&self->search_image_group);
  g_clear_object (&self->search_text_group);

  G_OBJECT_CLASS (bobgui_shortcuts_window_parent_class)->finalize (object);
}

static void
bobgui_shortcuts_window_dispose (GObject *object)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)object;

  if (self->stack)
    g_signal_handlers_disconnect_by_func (self->stack, G_CALLBACK (update_title_stack), self);

  bobgui_shortcuts_window_set_window (self, NULL);

  self->stack = NULL;
  self->search_bar = NULL;
  self->main_box = NULL;

  G_OBJECT_CLASS (bobgui_shortcuts_window_parent_class)->dispose (object);
}

static void
bobgui_shortcuts_window_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      {
        BobguiWidget *child = bobgui_stack_get_visible_child (self->stack);

        if (child != NULL)
          {
            char *name = NULL;

            g_object_get (bobgui_stack_get_page (self->stack, child),
                                     "name", &name,
                                     NULL);
            g_value_take_string (value, name);
          }
      }
      break;

    case PROP_VIEW_NAME:
      g_value_set_string (value, self->view_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_window_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      bobgui_shortcuts_window_set_section_name (self, g_value_get_string (value));
      break;

    case PROP_VIEW_NAME:
      bobgui_shortcuts_window_set_view_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_window_unmap (BobguiWidget *widget)
{
  BobguiShortcutsWindow *self = (BobguiShortcutsWindow *)widget;

  bobgui_search_bar_set_search_mode (self->search_bar, FALSE);
  bobgui_editable_set_text (BOBGUI_EDITABLE (self->search_entry), "");

  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_window_parent_class)->unmap (widget);
}

static void
bobgui_shortcuts_window_keys_changed (BobguiWindow *window)
{
  BobguiShortcutsWindow *self = BOBGUI_SHORTCUTS_WINDOW (window);

  BOBGUI_WINDOW_CLASS (bobgui_shortcuts_window_parent_class)->keys_changed (window);

  if (self->window != NULL)
    update_accels_for_actions (self);
}

static void
bobgui_shortcuts_window_class_init (BobguiShortcutsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  BobguiWindowClass *window_class = BOBGUI_WINDOW_CLASS (klass);

  object_class->constructed = bobgui_shortcuts_window_constructed;
  object_class->finalize = bobgui_shortcuts_window_finalize;
  object_class->get_property = bobgui_shortcuts_window_get_property;
  object_class->set_property = bobgui_shortcuts_window_set_property;
  object_class->dispose = bobgui_shortcuts_window_dispose;

  widget_class->unmap = bobgui_shortcuts_window_unmap;

  window_class->keys_changed = bobgui_shortcuts_window_keys_changed;

  klass->close = bobgui_shortcuts_window_close;
  klass->search = bobgui_shortcuts_window_search;

  /**
   * BobguiShortcutsWindow:section-name:
   *
   * The name of the section to show.
   *
   * This should be the section-name of one of the `BobguiShortcutsSection`
   * objects that are in this shortcuts window.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_SECTION_NAME] =
    g_param_spec_string ("section-name", NULL, NULL,
                         "internal-search",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsWindow:view-name:
   *
   * The view name by which to filter the contents.
   *
   * This should correspond to the [property@Bobgui.ShortcutsGroup:view]
   * property of some of the [class@Bobgui.ShortcutsGroup] objects that
   * are inside this shortcuts window.
   *
   * Set this to %NULL to show all groups.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_VIEW_NAME] =
    g_param_spec_string ("view-name", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  /**
   * BobguiShortcutsWindow::close:
   *
   * Emitted when the user uses a keybinding to close the window.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is the <kbd>Escape</kbd> key.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  signals[CLOSE] = g_signal_new (I_("close"),
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                 G_STRUCT_OFFSET (BobguiShortcutsWindowClass, close),
                                 NULL, NULL, NULL,
                                 G_TYPE_NONE,
                                 0);

  /**
   * BobguiShortcutsWindow::search:
   *
   * Emitted when the user uses a keybinding to start a search.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Control</kbd>+<kbd>F</kbd>.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  signals[SEARCH] = g_signal_new (I_("search"),
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                 G_STRUCT_OFFSET (BobguiShortcutsWindowClass, search),
                                 NULL, NULL, NULL,
                                 G_TYPE_NONE,
                                 0);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "close",
                                       NULL);

#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_f, GDK_META_MASK,
                                       "search",
                                       NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_f, GDK_CONTROL_MASK,
                                       "search",
                                       NULL);
#endif

  g_type_ensure (BOBGUI_TYPE_SHORTCUTS_GROUP);
  g_type_ensure (BOBGUI_TYPE_SHORTCUTS_SHORTCUT);

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_shortcuts_window_init (BobguiShortcutsWindow *self)
{
  BobguiWidget *search_button;
  BobguiBox *box;
  BobguiWidget *scroller;
  BobguiWidget *label;
  BobguiWidget *empty;
  BobguiWidget *image;
  PangoAttrList *attributes;

  bobgui_window_set_resizable (BOBGUI_WINDOW (self), FALSE);

  self->keywords = g_hash_table_new_full (NULL, NULL, NULL, g_free);
  self->search_items_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  self->search_text_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
  self->search_image_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);

  self->header_bar = BOBGUI_HEADER_BAR (bobgui_header_bar_new ());
  bobgui_window_set_titlebar (BOBGUI_WINDOW (self), BOBGUI_WIDGET (self->header_bar));

  search_button = g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON,
                                "icon-name", "edit-find-symbolic",
                                NULL);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (search_button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Search Shortcuts"),
                                  -1);

  bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (self->header_bar), search_button);

  self->main_box = g_object_new (BOBGUI_TYPE_BOX,
                           "orientation", BOBGUI_ORIENTATION_VERTICAL,
                           NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (self), self->main_box);

  self->search_bar = g_object_new (BOBGUI_TYPE_SEARCH_BAR, NULL);
  g_object_bind_property (self->search_bar, "search-mode-enabled",
                          search_button, "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (self->main_box), BOBGUI_WIDGET (self->search_bar));
  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (self->search_bar),
                                         BOBGUI_WIDGET (self));

  self->stack = g_object_new (BOBGUI_TYPE_STACK,
                              "hexpand", TRUE,
                              "vexpand", TRUE,
                              "hhomogeneous", TRUE,
                              "vhomogeneous", TRUE,
                              "transition-type", BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE,
                              NULL);
  bobgui_box_append (BOBGUI_BOX (self->main_box), BOBGUI_WIDGET (self->stack));

  self->title_stack = g_object_new (BOBGUI_TYPE_STACK,
                                    NULL);
  bobgui_header_bar_set_title_widget (self->header_bar, BOBGUI_WIDGET (self->title_stack));

  /* Translators: This is the window title for the shortcuts window in normal mode */
  label = bobgui_label_new (_("Shortcuts"));
  bobgui_widget_add_css_class (label, "title");
  bobgui_stack_add_named (self->title_stack, label, "title");

  /* Translators: This is the window title for the shortcuts window in search mode */
  label = bobgui_label_new (_("Search Results"));
  bobgui_widget_add_css_class (label, "title");
  bobgui_stack_add_named (self->title_stack, label, "search");

  self->menu_button = g_object_new (BOBGUI_TYPE_MENU_BUTTON,
                                    "focus-on-click", FALSE,
                                    NULL);
  bobgui_widget_add_css_class (BOBGUI_WIDGET (self->menu_button), "flat");
  bobgui_stack_add_named (self->title_stack, BOBGUI_WIDGET (self->menu_button), "sections");

  self->popover = g_object_new (BOBGUI_TYPE_POPOVER,
                                "position", BOBGUI_POS_BOTTOM,
                                NULL);
  bobgui_menu_button_set_popover (self->menu_button, BOBGUI_WIDGET (self->popover));

  self->list_box = g_object_new (BOBGUI_TYPE_LIST_BOX,
                                 "selection-mode", BOBGUI_SELECTION_NONE,
                                 NULL);
  g_signal_connect_object (self->list_box,
                           "row-activated",
                           G_CALLBACK (bobgui_shortcuts_window__list_box__row_activated),
                           self,
                           G_CONNECT_SWAPPED);
  bobgui_popover_set_child (BOBGUI_POPOVER (self->popover), BOBGUI_WIDGET (self->list_box));

  self->search_entry = BOBGUI_SEARCH_ENTRY (bobgui_search_entry_new ());
  bobgui_search_bar_set_child (BOBGUI_SEARCH_BAR (self->search_bar), BOBGUI_WIDGET (self->search_entry));

  g_object_set (self->search_entry,
                /* Translators: This is placeholder text for the search entry in the shortcuts window */
                "placeholder-text", _("Search Shortcuts"),
                "width-chars", 31,
                "max-width-chars", 40,
                NULL);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->search_entry),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Search Shortcuts"),
                                  -1);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (self->search_bar),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, self->search_entry, NULL,
                                  -1);

  g_signal_connect_object (self->search_entry,
                           "search-changed",
                           G_CALLBACK (bobgui_shortcuts_window__entry__changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->search_bar,
                           "notify::search-mode-enabled",
                           G_CALLBACK (bobgui_shortcuts_window__search_mode__changed),
                           self,
                           G_CONNECT_SWAPPED);

  scroller = bobgui_scrolled_window_new ();
  box = g_object_new (BOBGUI_TYPE_BOX,
                      "halign", BOBGUI_ALIGN_CENTER,
                      "orientation", BOBGUI_ORIENTATION_VERTICAL,
                      NULL);
  bobgui_widget_add_css_class (BOBGUI_WIDGET (box), "shortcuts-search-results");
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scroller), BOBGUI_WIDGET (box));
  bobgui_stack_add_named (self->stack, scroller, "internal-search");

  self->search_shortcuts = g_object_new (BOBGUI_TYPE_BOX,
                                         "halign", BOBGUI_ALIGN_CENTER,
                                         "spacing", 6,
                                         "orientation", BOBGUI_ORIENTATION_VERTICAL,
                                         NULL);
  bobgui_box_append (BOBGUI_BOX (box), BOBGUI_WIDGET (self->search_shortcuts));

  self->search_gestures = g_object_new (BOBGUI_TYPE_BOX,
                                        "halign", BOBGUI_ALIGN_CENTER,
                                        "spacing", 6,
                                        "orientation", BOBGUI_ORIENTATION_VERTICAL,
                                        NULL);
  bobgui_box_append (BOBGUI_BOX (box), BOBGUI_WIDGET (self->search_gestures));

  empty = g_object_new (BOBGUI_TYPE_GRID,
                        "row-spacing", 12,
                        "margin-start", 12,
                        "margin-end", 12,
                        "margin-top", 12,
                        "margin-bottom", 12,
                        "hexpand", TRUE,
                        "vexpand", TRUE,
                        "halign", BOBGUI_ALIGN_CENTER,
                        "valign", BOBGUI_ALIGN_CENTER,
                        NULL);
  bobgui_widget_add_css_class (empty, "dim-label");
  image = g_object_new (BOBGUI_TYPE_IMAGE,
                        "icon-name", "edit-find-symbolic",
                        "pixel-size", 72,
                        NULL);
  bobgui_grid_attach (BOBGUI_GRID (empty), image, 0, 0, 1, 1);
  attributes = pango_attr_list_new ();
  pango_attr_list_insert (attributes, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  pango_attr_list_insert (attributes, pango_attr_scale_new (1.44));
  label = g_object_new (BOBGUI_TYPE_LABEL,
                        "label", _("No Results Found"),
                        "attributes", attributes,
                        NULL);
  pango_attr_list_unref (attributes);
  bobgui_grid_attach (BOBGUI_GRID (empty), label, 0, 1, 1, 1);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (image),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  label = g_object_new (BOBGUI_TYPE_LABEL,
                        "label", _("Try a different search"),
                        NULL);
  bobgui_grid_attach (BOBGUI_GRID (empty), label, 0, 2, 1, 1);

  bobgui_stack_add_named (self->stack, empty, "no-search-results");

  g_signal_connect_object (self->stack, "notify::visible-child",
                           G_CALLBACK (update_title_stack), self, G_CONNECT_SWAPPED);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "shortcuts");
}
