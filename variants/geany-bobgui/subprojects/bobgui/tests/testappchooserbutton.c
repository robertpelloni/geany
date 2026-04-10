/* testappchooserbutton.c
 * Copyright (C) 2010 Red Hat, Inc.
 * Authors: Cosimo Cecchi
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

#include <stdlib.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define CUSTOM_ITEM "custom-item"

static BobguiWidget *toplevel, *button, *box;
static BobguiWidget *sel_image, *sel_name;

static void
combo_changed_cb (BobguiAppChooserButton *chooser_button,
                  gpointer             user_data)
{
  GAppInfo *app_info;

  app_info = bobgui_app_chooser_get_app_info (BOBGUI_APP_CHOOSER (chooser_button));

  if (app_info == NULL)
    return;

  bobgui_image_set_from_gicon (BOBGUI_IMAGE (sel_image), g_app_info_get_icon (app_info));
  bobgui_label_set_text (BOBGUI_LABEL (sel_name), g_app_info_get_display_name (app_info));

  g_object_unref (app_info);
}

static void
special_item_activated_cb (BobguiAppChooserButton *b,
                           const char *item_name,
                           gpointer user_data)
{
  bobgui_image_set_from_gicon (BOBGUI_IMAGE (sel_image), g_themed_icon_new ("face-smile"));
  bobgui_label_set_text (BOBGUI_LABEL (sel_name), "Special Item");
}

static void
action_cb (BobguiAppChooserButton *b,
           const char *item_name,
           gpointer user_data)
{
  g_print ("Activated custom item %s\n", item_name);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc,
      char **argv)
{
  BobguiWidget *w;
  gboolean done = FALSE;

  bobgui_init ();

  toplevel = bobgui_window_new ();

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);
  bobgui_window_set_child (BOBGUI_WINDOW (toplevel), box);

  button = bobgui_app_chooser_button_new ("image/jpeg");
  bobgui_widget_set_vexpand (button, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), button);

  g_signal_connect (button, "changed",
                    G_CALLBACK (combo_changed_cb), NULL);

  w = bobgui_label_new (NULL);
  bobgui_label_set_markup (BOBGUI_LABEL (w), "<b>Selected app info</b>");
  bobgui_widget_set_vexpand (w, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), w);

  w = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_widget_set_vexpand (w, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), w);

  sel_image = bobgui_image_new ();
  bobgui_widget_set_hexpand (sel_image, TRUE);
  bobgui_box_append (BOBGUI_BOX (w), sel_image);
  sel_name = bobgui_label_new (NULL);
  bobgui_widget_set_hexpand (sel_name, TRUE);
  bobgui_box_append (BOBGUI_BOX (w), sel_name);

  bobgui_app_chooser_button_set_heading (BOBGUI_APP_CHOOSER_BUTTON (button), "Choose one, <i>not</i> two");
  bobgui_app_chooser_button_append_separator (BOBGUI_APP_CHOOSER_BUTTON (button));
  bobgui_app_chooser_button_append_custom_item (BOBGUI_APP_CHOOSER_BUTTON (button),
                                             CUSTOM_ITEM,
                                             "Hey, I'm special!",
                                             g_themed_icon_new ("face-smile"));

  /* this one will trigger a warning, and will not be added */
  bobgui_app_chooser_button_append_custom_item (BOBGUI_APP_CHOOSER_BUTTON (button),
                                             CUSTOM_ITEM,
                                             "Hey, I'm fake!",
                                             g_themed_icon_new ("face-evil"));

  bobgui_app_chooser_button_set_show_dialog_item (BOBGUI_APP_CHOOSER_BUTTON (button),
                                               TRUE);
  bobgui_app_chooser_button_set_show_default_item (BOBGUI_APP_CHOOSER_BUTTON (button),
                                                TRUE);

  /* connect to the detailed signal */
  g_signal_connect (button, "custom-item-activated::" CUSTOM_ITEM,
                    G_CALLBACK (special_item_activated_cb), NULL);

  /* connect to the generic signal too */
  g_signal_connect (button, "custom-item-activated",
                    G_CALLBACK (action_cb), NULL);

  /* test refresh on a combo */
  bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (button));

#if 0
  bobgui_app_chooser_button_set_active_custom_item (BOBGUI_APP_CHOOSER_BUTTON (button),
                                                 CUSTOM_ITEM);
#endif
  bobgui_window_present (BOBGUI_WINDOW (toplevel));

  g_signal_connect (toplevel, "destroy", G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return EXIT_SUCCESS;
}
