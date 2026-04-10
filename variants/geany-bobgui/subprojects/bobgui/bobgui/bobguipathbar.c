/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* bobguipathbar.c
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
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

#include "config.h"

#include "bobguipathbarprivate.h"

#include <string.h>

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguidragsource.h"
#include "bobguiimage.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiscrolledwindow.h"
#include "bobguisettings.h"
#include "bobguitogglebutton.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguidragsource.h"
#include "bobguiprivate.h"

struct _BobguiPathBar
{
  BobguiWidget parent_instance;

  BobguiWidget *buttons_box;
  BobguiWidget *scrolled_window;

  GFile *root_file;
  GFile *home_file;
  GFile *desktop_file;

  /* List of running GCancellable.  When we cancel one, we remove it from this list.
   * The pathbar cancels all outstanding cancellables when it is disposed.
   *
   * In code that queues async I/O operations:
   *
   *   - Obtain a cancellable from the async I/O APIs, and call add_cancellable().
   *
   * To cancel a cancellable:
   *
   *   - Call cancel_cancellable().
   *
   * In async I/O callbacks:
   *
   *   - Check right away if g_cancellable_is_cancelled():  if true, just
   *     g_object_unref() the cancellable and return early (also free your
   *     closure data if you have one).
   *
   *   - If it was not cancelled, call cancellable_async_done().  This will
   *     unref the cancellable and unqueue it from the pathbar's outstanding
   *     cancellables.  Do your normal work to process the async result and free
   *     your closure data if you have one.
   */
  GList *cancellables;

  GCancellable *get_info_cancellable;

  GIcon *root_icon;
  GIcon *home_icon;
  GIcon *desktop_icon;

  GList *button_list;
  GList *fake_root;
};

typedef struct _BobguiPathBarClass BobguiPathBarClass;

struct _BobguiPathBarClass
{
  BobguiWidgetClass parent_class;

  void (* path_clicked) (BobguiPathBar  *path_bar,
                         GFile       *file,
                         GFile       *child_file,
                         gboolean     child_is_hidden);
};

enum {
  PATH_CLICKED,
  LAST_SIGNAL
};

typedef enum {
  NORMAL_BUTTON,
  ROOT_BUTTON,
  HOME_BUTTON,
  DESKTOP_BUTTON,
  RECENT_BUTTON,
} ButtonType;

#define BUTTON_DATA(x) ((ButtonData *)(x))

static guint path_bar_signals [LAST_SIGNAL] = { 0 };

typedef struct _ButtonData ButtonData;

struct _ButtonData
{
  BobguiWidget *button;
  ButtonType type;
  char *dir_name;
  GFile *file;
  BobguiWidget *image;
  BobguiWidget *label;
  GCancellable *cancellable;
  guint ignore_changes : 1;
  guint file_is_hidden : 1;
  GMount *mount;
};
/* This macro is used to check if a button can be used as a fake root.
 * All buttons in front of a fake root are automatically hidden when in a
 * directory below a fake root and replaced with the "<" arrow button.
 */
#define BUTTON_IS_FAKE_ROOT(button) ((button)->type == HOME_BUTTON || (button)->type == RECENT_BUTTON)

G_DEFINE_TYPE (BobguiPathBar, bobgui_path_bar, BOBGUI_TYPE_WIDGET)

static void bobgui_path_bar_finalize                 (GObject          *object);
static void bobgui_path_bar_dispose                  (GObject          *object);
static void bobgui_path_bar_update_button_appearance (BobguiPathBar       *path_bar,
						   ButtonData       *button_data,
						   gboolean          current_dir);

static void
add_cancellable (BobguiPathBar   *path_bar,
                 GCancellable *cancellable)
{
  g_assert (g_list_find (path_bar->cancellables, cancellable) == NULL);
  path_bar->cancellables = g_list_prepend (path_bar->cancellables, cancellable);
}

static void
drop_node_for_cancellable (BobguiPathBar *path_bar,
                           GCancellable *cancellable)
{
  GList *node;

  node = g_list_find (path_bar->cancellables, cancellable);
  g_assert (node != NULL);
  node->data = NULL;
  path_bar->cancellables = g_list_delete_link (path_bar->cancellables, node);
}

static void
cancel_cancellable (BobguiPathBar   *path_bar,
                    GCancellable *cancellable)
{
  drop_node_for_cancellable (path_bar, cancellable);
  g_cancellable_cancel (cancellable);
}

static void
cancellable_async_done (BobguiPathBar   *path_bar,
                        GCancellable *cancellable)
{
  drop_node_for_cancellable (path_bar, cancellable);
  g_object_unref (cancellable);
}

static void
cancel_all_cancellables (BobguiPathBar *path_bar)
{
  while (path_bar->cancellables)
    {
      GCancellable *cancellable = path_bar->cancellables->data;
      cancel_cancellable (path_bar, cancellable);
    }
}

static void
bobgui_path_bar_init (BobguiPathBar *path_bar)
{
  const char *home;

  path_bar->scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (path_bar->scrolled_window),
                                  BOBGUI_POLICY_EXTERNAL, BOBGUI_POLICY_NEVER);
  bobgui_widget_set_hexpand (path_bar->scrolled_window, TRUE);
  bobgui_widget_set_parent (path_bar->scrolled_window, BOBGUI_WIDGET (path_bar));

  path_bar->buttons_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (path_bar->buttons_box, "linked");
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (path_bar->scrolled_window),
                                 path_bar->buttons_box);


  path_bar->get_info_cancellable = NULL;
  path_bar->cancellables = NULL;

  home = g_get_home_dir ();
  if (home != NULL)
    {
      const char *desktop;

      path_bar->home_file = g_file_new_for_path (home);
      /* FIXME: Need file system backend specific way of getting the
       * Desktop path.
       */
      desktop = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
      if (desktop != NULL)
        path_bar->desktop_file = g_file_new_for_path (desktop);
      else
        path_bar->desktop_file = NULL;
    }
  else
    {
      path_bar->home_file = NULL;
      path_bar->desktop_file = NULL;
    }
  path_bar->root_file = g_file_new_for_path ("/");
}

static void
bobgui_path_bar_class_init (BobguiPathBarClass *path_bar_class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = (GObjectClass *) path_bar_class;
  widget_class = (BobguiWidgetClass *) path_bar_class;

  gobject_class->finalize = bobgui_path_bar_finalize;
  gobject_class->dispose = bobgui_path_bar_dispose;

  path_bar_signals [PATH_CLICKED] =
    g_signal_new (I_("path-clicked"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiPathBarClass, path_clicked),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_OBJECT_BOOLEAN,
                  G_TYPE_NONE, 3,
                  G_TYPE_FILE, G_TYPE_FILE, G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (path_bar_signals[PATH_CLICKED],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _bobgui_marshal_VOID__OBJECT_OBJECT_BOOLEANv);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "pathbar");
}

static void
bobgui_path_bar_finalize (GObject *object)
{
  BobguiPathBar *path_bar = BOBGUI_PATH_BAR (object);

  cancel_all_cancellables (path_bar);

  g_list_free (path_bar->button_list);
  g_clear_object (&path_bar->root_file);
  g_clear_object (&path_bar->home_file);
  g_clear_object (&path_bar->desktop_file);

  g_clear_object (&path_bar->root_icon);
  g_clear_object (&path_bar->home_icon);
  g_clear_object (&path_bar->desktop_icon);

  G_OBJECT_CLASS (bobgui_path_bar_parent_class)->finalize (object);
}

static void
bobgui_path_bar_dispose (GObject *object)
{
  BobguiPathBar *path_bar = BOBGUI_PATH_BAR (object);

  g_clear_pointer (&path_bar->scrolled_window, bobgui_widget_unparent);

  path_bar->get_info_cancellable = NULL;
  cancel_all_cancellables (path_bar);

  G_OBJECT_CLASS (bobgui_path_bar_parent_class)->dispose (object);
}

static void
bobgui_path_bar_clear_buttons (BobguiPathBar *path_bar)
{
  BobguiWidget *w;

  while ((w = bobgui_widget_get_first_child (path_bar->buttons_box)) != NULL)
    bobgui_box_remove (BOBGUI_BOX (path_bar->buttons_box), w);

  path_bar->fake_root = NULL;
}

static void
update_visibility_up_to_next_root (BobguiPathBar *path_bar,
                                   GList      *start_from_button)
{
  gboolean fake_root_found = FALSE;
  GList *l;

  for (l = start_from_button; l; l = l->next)
    {
      BobguiWidget *button = BUTTON_DATA (l->data)->button;

      bobgui_widget_set_visible (button, !fake_root_found);

      fake_root_found |= BUTTON_IS_FAKE_ROOT (BUTTON_DATA (l->data));
    }
}

static void
button_clicked_cb (BobguiWidget *button,
		   gpointer   data)
{
  BobguiPathBar *path_bar;
  ButtonData *button_data;
  GList *button_list;
  gboolean child_is_hidden;
  GFile *child_file;

  button_data = BUTTON_DATA (data);
  if (button_data->ignore_changes)
    return;

  path_bar = BOBGUI_PATH_BAR (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_PATH_BAR));

  button_list = g_list_find (path_bar->button_list, button_data);
  g_assert (button_list != NULL);

  g_signal_handlers_block_by_func (button,
				   G_CALLBACK (button_clicked_cb), data);
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);
  g_signal_handlers_unblock_by_func (button,
				     G_CALLBACK (button_clicked_cb), data);

  update_visibility_up_to_next_root (path_bar, button_list);

  if (button_list->prev)
    {
      ButtonData *child_data;

      child_data = BUTTON_DATA (button_list->prev->data);
      child_file = child_data->file;
      child_is_hidden = child_data->file_is_hidden;
    }
  else
    {
      child_file = NULL;
      child_is_hidden = FALSE;
    }

  g_signal_emit (path_bar, path_bar_signals [PATH_CLICKED], 0,
		 button_data->file, child_file, child_is_hidden);
}

struct SetButtonImageData
{
  BobguiPathBar *path_bar;
  ButtonData *button_data;
};

static void
set_button_image_get_info_cb (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  GFile *file = G_FILE (source);
  struct SetButtonImageData *data = user_data;
  GFileInfo *info;
  GIcon *icon;

  info = g_file_query_info_finish (file, result, NULL);
  if (!info)
    goto out;

  g_assert (BOBGUI_IS_PATH_BAR (data->path_bar));
  g_assert (G_OBJECT (data->path_bar)->ref_count > 0);

  cancellable_async_done (data->path_bar, data->button_data->cancellable);
  data->button_data->cancellable = NULL;

  icon = g_file_info_get_symbolic_icon (info);
  bobgui_image_set_from_gicon (BOBGUI_IMAGE (data->button_data->image), icon);

  switch (data->button_data->type)
    {
      case HOME_BUTTON:
        g_set_object (&data->path_bar->home_icon, icon);
        break;

      case DESKTOP_BUTTON:
        g_set_object (&data->path_bar->desktop_icon, icon);
        break;

      case RECENT_BUTTON:
      case NORMAL_BUTTON:
      case ROOT_BUTTON:
      default:
        break;
    };

  g_object_unref (info);

out:
  g_free (data);
}

static void
set_button_image (BobguiPathBar *path_bar,
                  ButtonData *button_data)
{
  struct SetButtonImageData *data;
  GIcon *root_icon = NULL;

  switch (button_data->type)
    {
    case ROOT_BUTTON:
      if (!button_data->mount && path_bar->root_icon != NULL &&
          g_file_is_native (button_data->file))
        {
          bobgui_image_set_from_gicon (BOBGUI_IMAGE (button_data->image), path_bar->root_icon);
          break;
        }

      if (!button_data->mount && g_file_is_native (button_data->file))
        root_icon = path_bar->root_icon = g_object_ref (g_themed_icon_new ("drive-harddisk-symbolic"));
      else if (button_data->mount)
        root_icon = g_mount_get_symbolic_icon (button_data->mount);
      else
        root_icon = NULL;

      bobgui_image_set_from_gicon (BOBGUI_IMAGE (button_data->image), root_icon);

      g_clear_object (&root_icon);

      break;

    case HOME_BUTTON:
      if (path_bar->home_icon != NULL)
        {
          bobgui_image_set_from_gicon (BOBGUI_IMAGE (button_data->image), path_bar->home_icon);
	  break;
	}

      data = g_new0 (struct SetButtonImageData, 1);
      data->path_bar = path_bar;
      data->button_data = button_data;

      if (button_data->cancellable)
        {
          cancel_cancellable (path_bar, button_data->cancellable);
          g_clear_object (&button_data->cancellable);
        }

      button_data->cancellable = g_cancellable_new ();
      g_file_query_info_async (path_bar->home_file,
                               "standard::symbolic-icon",
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               button_data->cancellable,
                               set_button_image_get_info_cb,
                               data);
      add_cancellable (path_bar, button_data->cancellable);
      break;

    case DESKTOP_BUTTON:
      if (path_bar->desktop_icon != NULL)
        {
          bobgui_image_set_from_gicon (BOBGUI_IMAGE (button_data->image), path_bar->desktop_icon);
	  break;
	}

      data = g_new0 (struct SetButtonImageData, 1);
      data->path_bar = path_bar;
      data->button_data = button_data;

      if (button_data->cancellable)
        {
          cancel_cancellable (path_bar, button_data->cancellable);
          g_clear_object (&button_data->cancellable);
        }

      button_data->cancellable = g_cancellable_new ();
      g_file_query_info_async (path_bar->desktop_file,
                               "standard::symbolic-icon",
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               button_data->cancellable,
                               set_button_image_get_info_cb,
                               data);
      add_cancellable (path_bar, button_data->cancellable);
      break;

    case RECENT_BUTTON:
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (button_data->image),
                                    "document-open-recent-symbolic");
      break;

    case NORMAL_BUTTON:
    default:
      break;
    }
}

static void
button_data_free (ButtonData *button_data)
{
  g_clear_object (&button_data->file);
  g_clear_object (&button_data->mount);
  g_free (button_data->dir_name);
  g_free (button_data);
}

static const char *
get_dir_name (ButtonData *button_data)
{
  return button_data->dir_name;
}

static void
bobgui_path_bar_update_button_appearance (BobguiPathBar *path_bar,
				       ButtonData *button_data,
				       gboolean    current_dir)
{
  const char *dir_name = get_dir_name (button_data);

  bobgui_widget_remove_css_class (button_data->button, "text-button");
  bobgui_widget_remove_css_class (button_data->button, "image-button");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button_data->button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, dir_name,
                                  -1);

  if (button_data->label != NULL)
    {
      bobgui_label_set_text (BOBGUI_LABEL (button_data->label), dir_name);
      if (button_data->image == NULL)
        bobgui_widget_add_css_class (button_data->button, "text-button");
    }

  if (button_data->image != NULL)
    {
      set_button_image (path_bar, button_data);
      if (button_data->label == NULL)
        bobgui_widget_add_css_class (button_data->button, "image-button");
    }

  if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button_data->button)) != current_dir)
    {
      button_data->ignore_changes = TRUE;
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button_data->button), current_dir);
      button_data->ignore_changes = FALSE;
    }
}

static gboolean
file_is_recent_uri (GFile *file)
{
  GFile *recent;
  gboolean same;

  recent = g_file_new_for_uri ("recent:///");
  same = g_file_equal (file, recent);
  g_object_unref (recent);

  return same;
}

static ButtonType
find_button_type (BobguiPathBar *path_bar,
                  GFile *file,
                  GFile *root_file)
{
  if (root_file != NULL && g_file_equal (file, root_file))
    return ROOT_BUTTON;
  if (path_bar->home_file != NULL &&
      g_file_equal (file, path_bar->home_file))
    return HOME_BUTTON;
  if (path_bar->desktop_file != NULL &&
      g_file_equal (file, path_bar->desktop_file))
    return DESKTOP_BUTTON;
  if (file_is_recent_uri (file))
    return RECENT_BUTTON;

 return NORMAL_BUTTON;
}

static ButtonData *
make_directory_button (BobguiPathBar  *path_bar,
		       const char  *dir_name,
		       GFile       *file,
		       GMount      *mount,
		       GFile       *root_file,
		       gboolean     current_dir,
		       gboolean     file_is_hidden)
{
  BobguiWidget *child = NULL;
  ButtonData *button_data;
  GdkContentProvider *content;
  BobguiDragSource *source;

  file_is_hidden = !! file_is_hidden;
  /* Is it a special button? */
  button_data = g_new0 (ButtonData, 1);
  button_data->type = find_button_type (path_bar, file, root_file);
  button_data->button = bobgui_toggle_button_new ();
  bobgui_widget_set_focus_on_click (button_data->button, FALSE);

  switch (button_data->type)
    {
    case ROOT_BUTTON:
      if (mount)
        {
          button_data->dir_name = g_mount_get_name (mount);
          button_data->mount = g_object_ref (mount);
        }
      else
        {
          button_data->image = bobgui_image_new ();
          child = button_data->image;
          button_data->label = NULL;
          break;
        }
      G_GNUC_FALLTHROUGH;
    case HOME_BUTTON:
    case DESKTOP_BUTTON:
    case RECENT_BUTTON:
      button_data->image = bobgui_image_new ();
      button_data->label = bobgui_label_new (NULL);
      child = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (child), button_data->image);
      bobgui_box_append (BOBGUI_BOX (child), button_data->label);
      break;
    case NORMAL_BUTTON:
    default:
      button_data->label = bobgui_label_new (NULL);
      child = button_data->label;
      button_data->image = NULL;
    }

  if (button_data->dir_name == NULL)
    button_data->dir_name = g_strdup (dir_name);
  button_data->file = g_object_ref (file);
  button_data->file_is_hidden = file_is_hidden;

  bobgui_button_set_child (BOBGUI_BUTTON (button_data->button), child);

  bobgui_path_bar_update_button_appearance (path_bar, button_data, current_dir);

  g_signal_connect (button_data->button, "clicked",
		    G_CALLBACK (button_clicked_cb),
		    button_data);
  g_object_weak_ref (G_OBJECT (button_data->button),
		     (GWeakNotify) button_data_free, button_data);

  source = bobgui_drag_source_new ();
  content = gdk_content_provider_new_typed (G_TYPE_FILE, button_data->file);
  bobgui_drag_source_set_content (source, content);
  g_object_unref (content);
  bobgui_widget_add_controller (button_data->button, BOBGUI_EVENT_CONTROLLER (source));

  return button_data;
}

static gboolean
bobgui_path_bar_check_parent_path (BobguiPathBar         *path_bar,
				GFile              *file)
{
  GList *list;
  GList *current_path = NULL;
  gboolean need_new_fake_root = FALSE;

  for (list = path_bar->button_list; list; list = list->next)
    {
      ButtonData *button_data;

      button_data = list->data;
      if (g_file_equal (file, button_data->file))
	{
	  current_path = list;
	  break;
	}
      if (list == path_bar->fake_root)
	need_new_fake_root = TRUE;
    }

  if (current_path)
    {
      if (need_new_fake_root)
	{
	  path_bar->fake_root = NULL;
	  for (list = current_path; list; list = list->next)
	    {
	      ButtonData *button_data;

	      button_data = list->data;
	      if (BUTTON_IS_FAKE_ROOT (button_data))
		{
		  path_bar->fake_root = list;
		  break;
		}
	    }
	}

      for (list = path_bar->button_list; list; list = list->next)
	{
	  bobgui_path_bar_update_button_appearance (path_bar,
						 BUTTON_DATA (list->data),
						 (list == current_path) ? TRUE : FALSE);
	}

      if (!bobgui_widget_get_child_visible (BUTTON_DATA (current_path->data)->button))
	{
	  bobgui_widget_queue_resize (BOBGUI_WIDGET (path_bar));
	}

      return TRUE;
    }
  return FALSE;
}


struct SetFileInfo
{
  GFile *file;
  GMount *mount;
  GFile *root_file;
  GFile *parent_file;
  BobguiPathBar *path_bar;
  GList *new_buttons;
  GList *fake_root;
  GCancellable *cancellable;
  gboolean first_directory;
};

static void
bobgui_path_bar_set_file_finish (struct SetFileInfo *info,
                              gboolean            result)
{
  if (result)
    {
      GList *l;

      bobgui_path_bar_clear_buttons (info->path_bar);
      g_list_free (info->path_bar->button_list);
      info->path_bar->button_list = g_list_reverse (info->new_buttons);
      info->path_bar->fake_root = info->fake_root;

      for (l = info->path_bar->button_list; l; l = l->next)
	{
	  BobguiWidget *button = BUTTON_DATA (l->data)->button;
          bobgui_box_prepend (BOBGUI_BOX (info->path_bar->buttons_box), button);
	}

      update_visibility_up_to_next_root (info->path_bar,
                                         info->path_bar->button_list);
    }
  else
    {
      GList *l;

      for (l = info->new_buttons; l; l = l->next)
        {
          ButtonData *button_data = BUTTON_DATA (l->data);

          bobgui_box_remove (BOBGUI_BOX (info->path_bar->buttons_box), button_data->button);
        }

      g_list_free (info->new_buttons);
    }

  if (info->file)
    g_object_unref (info->file);
  if (info->parent_file)
    g_object_unref (info->parent_file);
  if (info->root_file)
    g_object_unref (info->root_file);
  if (info->mount)
    g_object_unref (info->mount);

  g_free (info);
}


static void
bobgui_path_bar_get_info_callback (GObject      *source,
                                GAsyncResult *result,
                                gpointer      data)
{
  GFile *file = G_FILE (source);
  struct SetFileInfo *file_info = data;
  GFileInfo *info;
  ButtonData *button_data;
  const char *display_name;
  gboolean has_is_hidden, has_is_backup;
  gboolean is_hidden;

  info = g_file_query_info_finish (file, result, NULL);
  if (!info)
    {
      bobgui_path_bar_set_file_finish (file_info, FALSE);
      return;
    }

  g_assert (BOBGUI_IS_PATH_BAR (file_info->path_bar));
  g_assert (G_OBJECT (file_info->path_bar)->ref_count > 0);

  cancellable_async_done (file_info->path_bar, file_info->cancellable);
  if (file_info->path_bar->get_info_cancellable == file_info->cancellable)
    file_info->path_bar->get_info_cancellable = NULL;
  file_info->cancellable = NULL;

  display_name = g_file_info_get_display_name (info);
  has_is_hidden = g_file_info_has_attribute (info, "standard::is-hidden");
  has_is_backup = g_file_info_has_attribute (info, "standard::is-backup");
  is_hidden = (has_is_hidden && g_file_info_get_is_hidden (info)) ||
    (has_is_backup && g_file_info_get_is_backup (info));

  button_data = make_directory_button (file_info->path_bar, display_name,
                                       file_info->file,
                                       file_info->mount,
                                       file_info->root_file,
                                       file_info->first_directory, is_hidden);
  g_clear_object (&file_info->file);
  g_object_unref (info);

  file_info->new_buttons = g_list_prepend (file_info->new_buttons, button_data);

  if (BUTTON_IS_FAKE_ROOT (button_data))
    file_info->fake_root = file_info->new_buttons;

  /* We have assigned the info for the innermost button, i.e. the deepest directory.
   * Now, go on to fetch the info for this directory's parent.
   */

  file_info->file = file_info->parent_file;
  file_info->first_directory = FALSE;

  if (!file_info->file)
    {
      /* No parent?  Okay, we are done. */
      bobgui_path_bar_set_file_finish (file_info, TRUE);
      return;
    }

  if (g_file_equal (file_info->file, file_info->root_file))
    file_info->parent_file = NULL;
  else
    file_info->parent_file = g_file_get_parent (file_info->file);

  /* Recurse asynchronously */
  file_info->cancellable = g_cancellable_new ();
  file_info->path_bar->get_info_cancellable = file_info->cancellable;
  g_file_query_info_async (file_info->file,
                           "standard::display-name,"
                           "standard::is-hidden,"
                           "standard::is-backup",
                           G_FILE_QUERY_INFO_NONE,
                           G_PRIORITY_DEFAULT,
                           file_info->cancellable,
                           bobgui_path_bar_get_info_callback,
                           file_info);
  add_cancellable (file_info->path_bar, file_info->cancellable);
}

static void
bobgui_path_bar_get_mount_callback (GObject      *source,
				 GAsyncResult *result,
				 gpointer      data)
{
  GFile *file = G_FILE (source);
  struct SetFileInfo *file_info = data;
  GError *error = NULL;

  file_info->mount = g_file_find_enclosing_mount_finish (file, result,
							 &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      bobgui_path_bar_set_file_finish (file_info, FALSE);
      g_clear_error (&error);
      return;
    }
  g_clear_error (&error);

  if (file_info->mount)
    file_info->root_file = g_mount_get_root (file_info->mount);

  g_assert (BOBGUI_IS_PATH_BAR (file_info->path_bar));
  g_assert (G_OBJECT (file_info->path_bar)->ref_count > 0);

  if (file_info->root_file == NULL)
    file_info->root_file = g_object_ref (file_info->path_bar->root_file);

  if (g_file_equal (file_info->file, file_info->root_file))
    file_info->parent_file = NULL;
  else
    file_info->parent_file = g_file_get_parent (file_info->file);

  cancellable_async_done (file_info->path_bar, file_info->cancellable);
  if (file_info->path_bar->get_info_cancellable == file_info->cancellable)
    file_info->path_bar->get_info_cancellable = NULL;
  file_info->cancellable = NULL;

  /* Recurse asynchronously */
  file_info->cancellable = g_cancellable_new ();
  file_info->path_bar->get_info_cancellable = file_info->cancellable;
  g_file_query_info_async (file_info->file,
                           "standard::display-name,"
                           "standard::is-hidden,"
                           "standard::is-backup",
                           G_FILE_QUERY_INFO_NONE,
                           G_PRIORITY_DEFAULT,
                           file_info->cancellable,
                           bobgui_path_bar_get_info_callback,
                           file_info);
  add_cancellable (file_info->path_bar, file_info->cancellable);
}

void
_bobgui_path_bar_set_file (BobguiPathBar *path_bar,
                        GFile      *file,
                        gboolean    keep_trail)
{
  struct SetFileInfo *info;

  g_return_if_fail (BOBGUI_IS_PATH_BAR (path_bar));
  g_return_if_fail (G_IS_FILE (file));

  /* Check whether the new path is already present in the pathbar as buttons.
   * This could be a parent directory or a previous selected subdirectory.
   */
  if (keep_trail && bobgui_path_bar_check_parent_path (path_bar, file))
    return;

  info = g_new0 (struct SetFileInfo, 1);
  info->file = g_object_ref (file);
  info->path_bar = path_bar;
  info->first_directory = TRUE;

  if (path_bar->get_info_cancellable)
    cancel_cancellable (path_bar, path_bar->get_info_cancellable);

  info->cancellable = g_cancellable_new ();
  path_bar->get_info_cancellable = info->cancellable;

  if (g_file_is_native (info->file))
    {
      info->root_file = g_object_ref (path_bar->root_file);
      info->parent_file = g_file_get_parent (info->file);

      g_file_query_info_async (info->file,
                               "standard::display-name,"
                               "standard::is-hidden,"
                               "standard::is-backup",
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               info->cancellable,
                               bobgui_path_bar_get_info_callback,
                               info);
    }
  else
    {
      g_file_find_enclosing_mount_async (info->file,
                                         G_PRIORITY_DEFAULT,
                                         info->cancellable,
                                         bobgui_path_bar_get_mount_callback,
                                         info);
    }
  add_cancellable (path_bar, info->cancellable);
}

/**
 * _bobgui_path_bar_up:
 * @path_bar: a `BobguiPathBar`
 *
 * If the selected button in the pathbar is not the furthest button “up” (in the
 * root direction), act as if the user clicked on the next button up.
 **/
void
_bobgui_path_bar_up (BobguiPathBar *path_bar)
{
  GList *l;

  for (l = path_bar->button_list; l; l = l->next)
    {
      BobguiWidget *button = BUTTON_DATA (l->data)->button;
      if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button)))
	{
	  if (l->next)
	    {
	      BobguiWidget *next_button = BUTTON_DATA (l->next->data)->button;
	      button_clicked_cb (next_button, l->next->data);
	    }
	  break;
	}
    }
}

/**
 * _bobgui_path_bar_down:
 * @path_bar: a `BobguiPathBar`
 *
 * If the selected button in the pathbar is not the furthest button “down” (in the
 * leaf direction), act as if the user clicked on the next button down.
 **/
void
_bobgui_path_bar_down (BobguiPathBar *path_bar)
{
  GList *l;

  for (l = path_bar->button_list; l; l = l->next)
    {
      BobguiWidget *button = BUTTON_DATA (l->data)->button;
      if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button)))
	{
	  if (l->prev)
	    {
	      BobguiWidget *prev_button = BUTTON_DATA (l->prev->data)->button;
	      button_clicked_cb (prev_button, l->prev->data);
	    }
	  break;
	}
    }
}
