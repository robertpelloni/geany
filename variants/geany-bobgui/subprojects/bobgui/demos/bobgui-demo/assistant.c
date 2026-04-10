/* Assistant
 *
 * Demonstrates a sample multi-step assistant with BobguiAssistant. Assistants
 * are used to divide an operation into several simpler sequential steps,
 * and to guide the user through these steps.
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *progress_bar = NULL;

static gboolean
apply_changes_gradually (gpointer data)
{
  double fraction;

  /* Work, work, work... */
  fraction = bobgui_progress_bar_get_fraction (BOBGUI_PROGRESS_BAR (progress_bar));
  fraction += 0.05;

  if (fraction < 1.0)
    {
      bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (progress_bar), fraction);
      return G_SOURCE_CONTINUE;
    }
  else
    {
      /* Close automatically once changes are fully applied. */
      bobgui_window_destroy (BOBGUI_WINDOW (data));
      return G_SOURCE_REMOVE;
    }
}

static void
on_assistant_apply (BobguiWidget *widget, gpointer data)
{
  /* Start a timer to simulate changes taking a few seconds to apply. */
  g_timeout_add (100, apply_changes_gradually, widget);
}

static void
on_assistant_close_cancel (BobguiWidget *widget, gpointer data)
{
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
on_assistant_prepare (BobguiWidget *widget, BobguiWidget *page, gpointer data)
{
  int current_page, n_pages;
  char *title;

  current_page = bobgui_assistant_get_current_page (BOBGUI_ASSISTANT (widget));
  n_pages = bobgui_assistant_get_n_pages (BOBGUI_ASSISTANT (widget));

  title = g_strdup_printf ("Sample assistant (%d of %d)", current_page + 1, n_pages);
  bobgui_window_set_title (BOBGUI_WINDOW (widget), title);
  g_free (title);

  /* The fourth page (counting from zero) is the progress page.  The
  * user clicked Apply to get here so we tell the assistant to commit,
  * which means the changes up to this point are permanent and cannot
  * be cancelled or revisited. */
  if (current_page == 3)
      bobgui_assistant_commit (BOBGUI_ASSISTANT (widget));
}

static void
on_entry_changed (BobguiWidget *widget, gpointer data)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (data);
  BobguiWidget *current_page;
  int page_number;
  const char *text;

  page_number = bobgui_assistant_get_current_page (assistant);
  current_page = bobgui_assistant_get_nth_page (assistant, page_number);
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (widget));

  if (text && *text)
    bobgui_assistant_set_page_complete (assistant, current_page, TRUE);
  else
    bobgui_assistant_set_page_complete (assistant, current_page, FALSE);
}

static void
create_page1 (BobguiWidget *assistant)
{
  BobguiWidget *box, *label, *entry;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);

  label = bobgui_label_new ("You must fill out this entry to continue:");
  bobgui_box_append (BOBGUI_BOX (box), label);

  entry = bobgui_entry_new ();
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);
  bobgui_entry_set_activates_default (BOBGUI_ENTRY (entry), TRUE);
  bobgui_widget_set_valign (entry, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), entry);
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (on_entry_changed), assistant);

  bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), box);
  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), box, "Page 1");
  bobgui_assistant_set_page_type (BOBGUI_ASSISTANT (assistant), box, BOBGUI_ASSISTANT_PAGE_INTRO);
}

static void
create_page2 (BobguiWidget *assistant)
{
  BobguiWidget *box, *checkbutton;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);

  checkbutton = bobgui_check_button_new_with_label ("This is optional data, you may continue "
                                                 "even if you do not check this");
  bobgui_widget_set_valign (checkbutton, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), checkbutton);

  bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), box);
  bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), box, TRUE);
  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), box, "Page 2");
}

static void
create_page3 (BobguiWidget *assistant)
{
  BobguiWidget *label;

  label = bobgui_label_new ("This is a confirmation page, press 'Apply' to apply changes");

  bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), label);
  bobgui_assistant_set_page_type (BOBGUI_ASSISTANT (assistant), label, BOBGUI_ASSISTANT_PAGE_CONFIRM);
  bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), label, TRUE);
  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), label, "Confirmation");
}

static void
create_page4 (BobguiWidget *assistant)
{
  progress_bar = bobgui_progress_bar_new ();
  bobgui_widget_set_halign (progress_bar, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (progress_bar, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_hexpand (progress_bar, TRUE);
  bobgui_widget_set_margin_start (progress_bar, 40);
  bobgui_widget_set_margin_end (progress_bar, 40);

  bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), progress_bar);
  bobgui_assistant_set_page_type (BOBGUI_ASSISTANT (assistant), progress_bar, BOBGUI_ASSISTANT_PAGE_PROGRESS);
  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), progress_bar, "Applying changes");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (progress_bar),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Applying changes",
                                  -1);

  /* This prevents the assistant window from being
   * closed while we're "busy" applying changes.
   */
  bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), progress_bar, FALSE);
}

BobguiWidget*
do_assistant (BobguiWidget *do_widget)
{
  static BobguiWidget *assistant;

  if (!assistant)
    {
      assistant = bobgui_assistant_new ();

      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), -1, 300);

      bobgui_window_set_display (BOBGUI_WINDOW (assistant),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (assistant), (gpointer *)&assistant);

      create_page1 (assistant);
      create_page2 (assistant);
      create_page3 (assistant);
      create_page4 (assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
                        G_CALLBACK (on_assistant_close_cancel), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
                        G_CALLBACK (on_assistant_close_cancel), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
                        G_CALLBACK (on_assistant_apply), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
                        G_CALLBACK (on_assistant_prepare), NULL);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_set_visible (assistant, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (assistant));

  return assistant;
}
