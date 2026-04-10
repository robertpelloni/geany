/* Printing/Page Setup
 * #Keywords: BobguiPageSetup
 *
 * BobguiPageSetupUnixDialog can be used if page setup is needed
 * independent of a full printing dialog.
 */

#include <math.h>
#include <bobgui/bobgui.h>
#include <bobgui/bobguiunixprint.h>

static void
done_cb (BobguiDialog *dialog, int response, gpointer data)
{
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

BobguiWidget *
do_pagesetup (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      window = bobgui_page_setup_unix_dialog_new ("Page Setup", BOBGUI_WINDOW (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      g_signal_connect (window, "response", G_CALLBACK (done_cb), NULL);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
