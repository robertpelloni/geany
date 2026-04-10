/* templates.c
 * Copyright (C) 2013 Openismus GmbH
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
 *
 * Authors: Tristan Van Berkom <tristanvb@openismus.com>
 */
#include <bobgui/bobgui.h>

#ifdef HAVE_UNIX_PRINT_WIDGETS
#  include <bobgui/bobguiunixprint.h>
#endif

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static gboolean
main_loop_quit_cb (gpointer data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);

  return FALSE;
}

static void
show_and_wait (BobguiWidget *widget)
{
  gboolean done = FALSE;

  g_timeout_add (500, main_loop_quit_cb, &done);
  bobgui_widget_set_visible (widget, TRUE);
  while (!done)
    g_main_context_iteration (NULL, FALSE);
}

static void
test_dialog_basic (void)
{
  BobguiWidget *dialog;

  dialog = bobgui_dialog_new ();
  g_assert_true (BOBGUI_IS_DIALOG (dialog));
  g_assert_nonnull (bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog)));

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
test_dialog_override_property (void)
{
  BobguiWidget *dialog;

  dialog = g_object_new (BOBGUI_TYPE_DIALOG,
                         "use-header-bar", 1,
                         NULL);
  g_assert_true (BOBGUI_IS_DIALOG (dialog));

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
test_message_dialog_basic (void)
{
  BobguiWidget *dialog;

  dialog = bobgui_message_dialog_new (NULL, 0,
                                   BOBGUI_MESSAGE_INFO,
                                   BOBGUI_BUTTONS_CLOSE,
                                   "Do it hard !");
  g_assert_true (BOBGUI_IS_DIALOG (dialog));
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
test_about_dialog_basic (void)
{
  BobguiWidget *dialog;

  dialog = bobgui_about_dialog_new ();
  g_assert_true (BOBGUI_IS_ABOUT_DIALOG (dialog));
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
test_about_dialog_show (void)
{
  BobguiWidget *dialog;

  dialog = bobgui_about_dialog_new ();
  g_assert_true (BOBGUI_IS_ABOUT_DIALOG (dialog));
  show_and_wait (dialog);
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
test_info_bar_basic (void)
{
  BobguiWidget *infobar;

  infobar = bobgui_info_bar_new ();
  g_assert_true (BOBGUI_IS_INFO_BAR (infobar));
  g_object_unref (g_object_ref_sink (infobar));
}

static void
test_lock_button_basic (void)
{
  BobguiWidget *button;
  GPermission *permission;

  permission = g_simple_permission_new (TRUE);
  button = bobgui_lock_button_new (permission);
  g_assert_true (BOBGUI_IS_LOCK_BUTTON (button));
  g_object_unref (g_object_ref_sink (button));
  g_object_unref (permission);
}

static void
test_assistant_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_assistant_new ();
  g_assert_true (BOBGUI_IS_ASSISTANT (widget));
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_assistant_show (void)
{
  BobguiWidget *widget;

  widget = bobgui_assistant_new ();
  g_assert_true (BOBGUI_IS_ASSISTANT (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_scale_button_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_scale_button_new (0, 100, 10, NULL);
  g_assert_true (BOBGUI_IS_SCALE_BUTTON (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_volume_button_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_volume_button_new ();
  g_assert_true (BOBGUI_IS_VOLUME_BUTTON (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_statusbar_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_statusbar_new ();
  g_assert_true (BOBGUI_IS_STATUSBAR (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_search_bar_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_search_bar_new ();
  g_assert_true (BOBGUI_IS_SEARCH_BAR (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_action_bar_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_action_bar_new ();
  g_assert_true (BOBGUI_IS_ACTION_BAR (widget));
  g_object_unref (g_object_ref_sink (widget));
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_app_chooser_widget_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_app_chooser_widget_new (NULL);
  g_assert_true (BOBGUI_IS_APP_CHOOSER_WIDGET (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_app_chooser_dialog_basic (void)
{
  BobguiWidget *widget;
  gboolean done = FALSE;

  widget = bobgui_app_chooser_dialog_new_for_content_type (NULL, 0, "text/plain");
  g_assert_true (BOBGUI_IS_APP_CHOOSER_DIALOG (widget));

  /* BobguiAppChooserDialog bug, if destroyed before spinning 
   * the main context then app_chooser_online_get_default_ready_cb()
   * will be eventually called and segfault.
   */
  g_timeout_add (500, main_loop_quit_cb, &done);
  while (!done)
    g_main_context_iteration (NULL,  TRUE);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_color_chooser_dialog_basic (void)
{
  BobguiWidget *widget;

  /* This test also tests the internal BobguiColorEditor widget */
  widget = bobgui_color_chooser_dialog_new (NULL, NULL);
  g_assert_true (BOBGUI_IS_COLOR_CHOOSER_DIALOG (widget));
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_color_chooser_dialog_show (void)
{
  BobguiWidget *widget;

  /* This test also tests the internal BobguiColorEditor widget */
  widget = bobgui_color_chooser_dialog_new (NULL, NULL);
  g_assert_true (BOBGUI_IS_COLOR_CHOOSER_DIALOG (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

/* Avoid warnings from GVFS-RemoteVolumeMonitor */
static gboolean
ignore_gvfs_warning (const char *log_domain,
                     GLogLevelFlags log_level,
                     const char *message,
                     gpointer user_data)
{
  if (g_strcmp0 (log_domain, "GVFS-RemoteVolumeMonitor") == 0)
    return FALSE;

  return TRUE;
}

static void
test_file_chooser_widget_basic (void)
{
  BobguiWidget *widget;
  gboolean done = FALSE;

  /* This test also tests the internal BobguiPathBar widget */
  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = bobgui_file_chooser_widget_new (BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  g_assert_true (BOBGUI_IS_FILE_CHOOSER_WIDGET (widget));

  /* XXX BUG:
   *
   * Spin the mainloop for a bit, this allows the file operations
   * to complete, BobguiFileChooserWidget has a bug where it leaks
   * BobguiTreeRowReferences to the internal shortcuts_model
   *
   * Since we assert all automated children are finalized we
   * can catch this
   */
  g_timeout_add (100, main_loop_quit_cb, &done);
  while (!done)
    g_main_context_iteration (NULL,  TRUE);

  g_object_unref (g_object_ref_sink (widget));
}

static void
test_file_chooser_dialog_basic (void)
{
  BobguiWidget *widget;
  gboolean done;

  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = bobgui_file_chooser_dialog_new ("The Dialog", NULL,
                                        BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        "_OK", BOBGUI_RESPONSE_OK,
                                        NULL);

  g_assert_true (BOBGUI_IS_FILE_CHOOSER_DIALOG (widget));
  done = FALSE;
  g_timeout_add (100, main_loop_quit_cb, &done);
  while (!done)
    g_main_context_iteration (NULL,  TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_file_chooser_dialog_show (void)
{
  BobguiWidget *widget;

  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = bobgui_file_chooser_dialog_new ("The Dialog", NULL,
                                        BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        "_OK", BOBGUI_RESPONSE_OK,
                                        NULL);

  g_assert_true (BOBGUI_IS_FILE_CHOOSER_DIALOG (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_font_button_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_font_button_new ();
  g_assert_true (BOBGUI_IS_FONT_BUTTON (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_font_chooser_widget_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_font_chooser_widget_new ();
  g_assert_true (BOBGUI_IS_FONT_CHOOSER_WIDGET (widget));
  g_object_unref (g_object_ref_sink (widget));
}

static void
test_font_chooser_dialog_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_font_chooser_dialog_new ("Choose a font !", NULL);
  g_assert_true (BOBGUI_IS_FONT_CHOOSER_DIALOG (widget));
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_font_chooser_dialog_show (void)
{
  BobguiWidget *widget;

  widget = bobgui_font_chooser_dialog_new ("Choose a font !", NULL);
  g_assert_true (BOBGUI_IS_FONT_CHOOSER_DIALOG (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

#ifdef HAVE_UNIX_PRINT_WIDGETS
static void
test_page_setup_unix_dialog_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_page_setup_unix_dialog_new ("Setup your Page !", NULL);
  g_assert_true (BOBGUI_IS_PAGE_SETUP_UNIX_DIALOG (widget));
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_page_setup_unix_dialog_show (void)
{
  BobguiWidget *widget;

  widget = bobgui_page_setup_unix_dialog_new ("Setup your Page !", NULL);
  g_assert_true (BOBGUI_IS_PAGE_SETUP_UNIX_DIALOG (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_print_unix_dialog_basic (void)
{
  BobguiWidget *widget;

  widget = bobgui_print_unix_dialog_new ("Go Print !", NULL);
  g_assert_true (BOBGUI_IS_PRINT_UNIX_DIALOG (widget));
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_print_unix_dialog_show (void)
{
  BobguiWidget *widget;

  widget = bobgui_print_unix_dialog_new ("Go Print !", NULL);
  g_assert_true (BOBGUI_IS_PRINT_UNIX_DIALOG (widget));
  show_and_wait (widget);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}
#endif

int
main (int argc, char **argv)
{
  /* These must be set before bobgui_test_init */
  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);

  /* initialize test program */
  bobgui_test_init (&argc, &argv);

  /* This environment variable cooperates with widget dispose()
   * to assert that all automated compoenents are properly finalized
   * when a given composite widget is destroyed.
   */
  g_assert_true (g_setenv ("BOBGUI_WIDGET_ASSERT_COMPONENTS", "1", TRUE));

  g_test_add_func ("/template/BobguiDialog/basic", test_dialog_basic);
  g_test_add_func ("/template/BobguiDialog/OverrideProperty", test_dialog_override_property);
  g_test_add_func ("/template/BobguiMessageDialog/basic", test_message_dialog_basic);
  g_test_add_func ("/template/BobguiAboutDialog/basic", test_about_dialog_basic);
  g_test_add_func ("/template/BobguiAboutDialog/show", test_about_dialog_show);
  g_test_add_func ("/template/BobguiInfoBar/basic", test_info_bar_basic);
  g_test_add_func ("/template/BobguiLockButton/basic", test_lock_button_basic);
  g_test_add_func ("/template/BobguiAssistant/basic", test_assistant_basic);
  g_test_add_func ("/template/BobguiAssistant/show", test_assistant_show);
  g_test_add_func ("/template/BobguiScaleButton/basic", test_scale_button_basic);
  g_test_add_func ("/template/BobguiVolumeButton/basic", test_volume_button_basic);
  g_test_add_func ("/template/BobguiStatusBar/basic", test_statusbar_basic);
  g_test_add_func ("/template/BobguiSearchBar/basic", test_search_bar_basic);
  g_test_add_func ("/template/BobguiActionBar/basic", test_action_bar_basic);
  g_test_add_func ("/template/BobguiAppChooserWidget/basic", test_app_chooser_widget_basic);
  g_test_add_func ("/template/BobguiAppChooserDialog/basic", test_app_chooser_dialog_basic);
  g_test_add_func ("/template/BobguiColorChooserDialog/basic", test_color_chooser_dialog_basic);
  g_test_add_func ("/template/BobguiColorChooserDialog/show", test_color_chooser_dialog_show);
  g_test_add_func ("/template/BobguiFileChooserWidget/basic", test_file_chooser_widget_basic);
  g_test_add_func ("/template/BobguiFileChooserDialog/basic", test_file_chooser_dialog_basic);
  g_test_add_func ("/template/BobguiFileChooserDialog/show", test_file_chooser_dialog_show);
  g_test_add_func ("/template/BobguiFontButton/basic", test_font_button_basic);
  g_test_add_func ("/template/BobguiFontChooserWidget/basic", test_font_chooser_widget_basic);
  g_test_add_func ("/template/BobguiFontChooserDialog/basic", test_font_chooser_dialog_basic);
  g_test_add_func ("/template/BobguiFontChooserDialog/show", test_font_chooser_dialog_show);

#ifdef HAVE_UNIX_PRINT_WIDGETS
  g_test_add_func ("/template/BobguiPageSetupUnixDialog/basic", test_page_setup_unix_dialog_basic);
  g_test_add_func ("/template/BobguiPageSetupUnixDialog/show", test_page_setup_unix_dialog_show);
  g_test_add_func ("/template/BobguiPrintUnixDialog/basic", test_print_unix_dialog_basic);
  g_test_add_func ("/template/BobguiPrintUnixDialog/show", test_print_unix_dialog_show);
#endif

  return g_test_run();
}
