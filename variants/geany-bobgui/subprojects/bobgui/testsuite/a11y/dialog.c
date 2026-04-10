#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
dialog_role (void)
{
  BobguiWidget *dialog = bobgui_dialog_new ();

  bobgui_test_accessible_assert_role (dialog, BOBGUI_ACCESSIBLE_ROLE_DIALOG);

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
dialog_state (void)
{
  BobguiWidget *dialog = bobgui_dialog_new ();

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  bobgui_test_accessible_assert_state (dialog, BOBGUI_ACCESSIBLE_STATE_HIDDEN, FALSE);

  bobgui_widget_hide (dialog);

  bobgui_test_accessible_assert_state (dialog, BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
dialog_properties (void)
{
  BobguiWidget *dialog = bobgui_dialog_new ();

  bobgui_window_set_modal (BOBGUI_WINDOW (dialog), TRUE);

  bobgui_test_accessible_assert_property (dialog, BOBGUI_ACCESSIBLE_PROPERTY_MODAL, TRUE);
  bobgui_window_set_modal (BOBGUI_WINDOW (dialog), FALSE);

  bobgui_test_accessible_assert_property (dialog, BOBGUI_ACCESSIBLE_PROPERTY_MODAL, FALSE);

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/dialog/role", dialog_role);
  g_test_add_func ("/a11y/dialog/state", dialog_state);
  g_test_add_func ("/a11y/dialog/properties", dialog_properties);

  return g_test_run ();
}
