/*
 * Copyright (C) 2010 Openismus GmbH
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum {
  SIMPLE_ITEMS = 0,
  FOCUS_ITEMS,
  WRAPPY_ITEMS,
  IMAGE_ITEMS,
  BUTTON_ITEMS
};

#define INITIAL_HALIGN          BOBGUI_ALIGN_FILL
#define INITIAL_VALIGN          BOBGUI_ALIGN_START
#define INITIAL_MINIMUM_LENGTH  3
#define INITIAL_MAXIMUM_LENGTH  6
#define INITIAL_CSPACING        2
#define INITIAL_RSPACING        2
#define N_ITEMS 1000

static BobguiFlowBox    *the_flowbox       = NULL;
static int            items_type       = SIMPLE_ITEMS;

static void
populate_flowbox_simple (BobguiFlowBox *flowbox)
{
  BobguiWidget *widget, *frame;
  int i;

  for (i = 0; i < N_ITEMS; i++)
    {
      char *text = g_strdup_printf ("Item %02d", i);

      widget = bobgui_label_new (text);
      frame  = bobgui_frame_new (NULL);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), widget);

      g_object_set_data_full (G_OBJECT (frame), "id", (gpointer)g_strdup (text), g_free);
      bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), frame, -1);

      g_free (text);
    }
}

static void
populate_flowbox_focus (BobguiFlowBox *flowbox)
{
  BobguiWidget *widget, *frame, *box;
  int i;
  gboolean sensitive;

  for (i = 0; i < 200; i++)
    {
      sensitive = TRUE;
      frame = bobgui_frame_new (NULL);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

      widget = bobgui_label_new ("Label");
      bobgui_box_append (BOBGUI_BOX (box), widget);

      switch (i % 4)
        {
        case 0:
          widget = bobgui_entry_new ();
          break;
        case 1:
          widget = bobgui_button_new_with_label ("Button");
          break;
        case 2:
          widget = bobgui_label_new ("bla");
          break;
        case 3:
          widget = bobgui_label_new ("bla");
          sensitive = FALSE;
          break;
        default:
          g_assert_not_reached ();
        }

      bobgui_box_append (BOBGUI_BOX (box), widget);

      if (i % 5 == 0)
        bobgui_box_append (BOBGUI_BOX (box), bobgui_switch_new ());

      bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), frame, -1);
      if (!sensitive)
        bobgui_widget_set_sensitive (bobgui_widget_get_parent (frame), FALSE);
    }
}

static void
populate_flowbox_buttons (BobguiFlowBox *flowbox)
{
  BobguiWidget *widget;
  int i;

  for (i = 0; i < 50; i++)
    {
      widget = bobgui_button_new_with_label ("Button");
      bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), widget, -1);
      widget = bobgui_widget_get_parent (widget);
      bobgui_widget_set_can_focus (widget, FALSE);
    }
}

static void
populate_flowbox_wrappy (BobguiFlowBox *flowbox)
{
  BobguiWidget *widget, *frame;
  int i;

  const char *strings[] = {
    "These are", "some wrappy label", "texts", "of various", "lengths.",
    "They should always be", "shown", "consecutively. Except it's",
    "hard to say", "where exactly the", "label", "will wrap", "and where exactly",
    "the actual", "container", "will wrap.", "This label is really really really long !",
    "Let's add some more", "labels to the",
    "mix. Just to", "make sure we", "got something to work", "with here."
  };

  for (i = 0; i < G_N_ELEMENTS (strings); i++)
    {
      widget = bobgui_label_new (strings[i]);
      frame  = bobgui_frame_new (NULL);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), widget);

      bobgui_label_set_wrap (BOBGUI_LABEL (widget), TRUE);
      bobgui_label_set_wrap_mode (BOBGUI_LABEL (widget), PANGO_WRAP_WORD);
      bobgui_label_set_width_chars (BOBGUI_LABEL (widget), 10);
      g_object_set_data_full (G_OBJECT (frame), "id", (gpointer)g_strdup (strings[i]), g_free);

      bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), frame, -1);
    }
}

static void
populate_flowbox_images (BobguiFlowBox *flowbox)
{
  BobguiWidget *widget, *image, *label;
  int i;

  for (i = 0; i < N_ITEMS; i++)
    {
      char *text = g_strdup_printf ("Item %02d", i);

      widget = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
      bobgui_widget_set_hexpand (widget, TRUE);

      image = bobgui_image_new_from_icon_name ("face-wink");
      bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
      bobgui_widget_set_hexpand (image, TRUE);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 256);

      label = bobgui_label_new (text);

      bobgui_box_append (BOBGUI_BOX (widget), image);
      bobgui_box_append (BOBGUI_BOX (widget), label);

      g_object_set_data_full (G_OBJECT (widget), "id", (gpointer)g_strdup (text), g_free);
      bobgui_box_append (BOBGUI_BOX (flowbox), widget);

      g_free (text);
    }
}

static void
populate_items (BobguiFlowBox *flowbox)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (flowbox))))
    bobgui_flow_box_remove (flowbox, child);

  if (items_type == SIMPLE_ITEMS)
    populate_flowbox_simple (flowbox);
  else if (items_type == FOCUS_ITEMS)
    populate_flowbox_focus (flowbox);
  else if (items_type == WRAPPY_ITEMS)
    populate_flowbox_wrappy (flowbox);
  else if (items_type == IMAGE_ITEMS)
    populate_flowbox_images (flowbox);
  else if (items_type == BUTTON_ITEMS)
    populate_flowbox_buttons (flowbox);
}

static void
horizontal_alignment_changed (BobguiComboBox   *box,
                              BobguiFlowBox    *flowbox)
{
  BobguiAlign alignment = bobgui_combo_box_get_active (box);

  bobgui_widget_set_halign (BOBGUI_WIDGET (flowbox), alignment);
}

static void
vertical_alignment_changed (BobguiComboBox   *box,
                            BobguiFlowBox    *flowbox)
{
  BobguiAlign alignment = bobgui_combo_box_get_active (box);

  bobgui_widget_set_valign (BOBGUI_WIDGET (flowbox), alignment);
}

static void
orientation_changed (BobguiComboBox   *box,
                     BobguiFlowBox *flowbox)
{
  BobguiOrientation orientation = bobgui_combo_box_get_active (box);

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (flowbox), orientation);
}

static void
selection_mode_changed (BobguiComboBox *box,
                        BobguiFlowBox  *flowbox)
{
  BobguiSelectionMode mode = bobgui_combo_box_get_active (box);

  bobgui_flow_box_set_selection_mode (flowbox, mode);
}

static void
line_length_changed (BobguiSpinButton *spin,
                     BobguiFlowBox *flowbox)
{
  int length = bobgui_spin_button_get_value_as_int (spin);

  bobgui_flow_box_set_min_children_per_line (flowbox, length);
}

static void
max_line_length_changed (BobguiSpinButton *spin,
                         BobguiFlowBox *flowbox)
{
  int length = bobgui_spin_button_get_value_as_int (spin);

  bobgui_flow_box_set_max_children_per_line (flowbox, length);
}

static void
spacing_changed (BobguiSpinButton *button,
                 gpointer       data)
{
  BobguiOrientation orientation = GPOINTER_TO_INT (data);
  int            state = bobgui_spin_button_get_value_as_int (button);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    bobgui_flow_box_set_column_spacing (the_flowbox, state);
  else
    bobgui_flow_box_set_row_spacing (the_flowbox, state);
}

static void
items_changed (BobguiComboBox   *box,
               BobguiFlowBox *flowbox)
{
  items_type = bobgui_combo_box_get_active (box);

  populate_items (flowbox);
}

static void
homogeneous_toggled (BobguiCheckButton *button,
                     BobguiFlowBox     *flowbox)
{
  gboolean state = bobgui_check_button_get_active (button);

  bobgui_flow_box_set_homogeneous (flowbox, state);
}

static void
on_child_activated (BobguiFlowBox *self,
                    BobguiWidget  *child)
{
  const char *id;
  id = g_object_get_data (G_OBJECT (bobgui_flow_box_child_get_child (BOBGUI_FLOW_BOX_CHILD (child))), "id");
  g_message ("Child activated %p: %s", child, id);
}

static G_GNUC_UNUSED void
selection_foreach (BobguiFlowBox      *self,
                   BobguiFlowBoxChild *child_info,
                   gpointer         data)
{
  const char *id;
  BobguiWidget *child;

  child = bobgui_flow_box_child_get_child (child_info);
  id = g_object_get_data (G_OBJECT (child), "id");
  g_message ("Child selected %p: %s", child, id);
}

static void
on_selected_children_changed (BobguiFlowBox *self)
{
  g_message ("Selection changed");
  //bobgui_flow_box_selected_foreach (self, selection_foreach, NULL);
}

static gboolean
filter_func (BobguiFlowBoxChild *child, gpointer user_data)
{
  int index;

  index = bobgui_flow_box_child_get_index (child);

  return (index % 3) == 0;
}

static void
filter_toggled (BobguiCheckButton *button,
                BobguiFlowBox     *flowbox)
{
  gboolean state = bobgui_check_button_get_active (button);

  if (state)
    bobgui_flow_box_set_filter_func (flowbox, filter_func, NULL, NULL);
  else
    bobgui_flow_box_set_filter_func (flowbox, NULL, NULL, NULL);
}

static int
sort_func (BobguiFlowBoxChild *a,
           BobguiFlowBoxChild *b,
           gpointer         data)
{
  char *ida, *idb;

  ida = (char *)g_object_get_data (G_OBJECT (bobgui_flow_box_child_get_child (a)), "id");
  idb = (char *)g_object_get_data (G_OBJECT (bobgui_flow_box_child_get_child (b)), "id");
  return g_strcmp0 (ida, idb);
}

static void
sort_toggled (BobguiCheckButton *button,
              BobguiFlowBox     *flowbox)
{
  gboolean state = bobgui_check_button_get_active (button);

  if (state)
    bobgui_flow_box_set_sort_func (flowbox, sort_func, NULL, NULL);
  else
    bobgui_flow_box_set_sort_func (flowbox, NULL, NULL, NULL);
}

static BobguiWidget *
create_window (void)
{
  BobguiWidget *window, *hbox, *vbox, *flowbox_cntl, *items_cntl;
  BobguiWidget *flowbox, *widget, *expander, *swindow;

  window = bobgui_window_new ();
  hbox   = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  vbox   = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

  bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  swindow = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (swindow, TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow),
                                  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);

  bobgui_box_append (BOBGUI_BOX (hbox), swindow);

  flowbox = bobgui_flow_box_new ();
  bobgui_widget_set_halign (flowbox, BOBGUI_ALIGN_END);
  the_flowbox = (BobguiFlowBox *)flowbox;
  bobgui_widget_set_halign (flowbox, INITIAL_HALIGN);
  bobgui_widget_set_valign (flowbox, INITIAL_VALIGN);
  bobgui_flow_box_set_column_spacing (BOBGUI_FLOW_BOX (flowbox), INITIAL_CSPACING);
  bobgui_flow_box_set_row_spacing (BOBGUI_FLOW_BOX (flowbox), INITIAL_RSPACING);
  bobgui_flow_box_set_min_children_per_line (BOBGUI_FLOW_BOX (flowbox), INITIAL_MINIMUM_LENGTH);
  bobgui_flow_box_set_max_children_per_line (BOBGUI_FLOW_BOX (flowbox), INITIAL_MAXIMUM_LENGTH);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), flowbox);

  bobgui_flow_box_set_hadjustment (BOBGUI_FLOW_BOX (flowbox),
                                bobgui_scrolled_window_get_hadjustment (BOBGUI_SCROLLED_WINDOW (swindow)));
  bobgui_flow_box_set_vadjustment (BOBGUI_FLOW_BOX (flowbox),
                                bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (swindow)));

  g_signal_connect (flowbox, "child-activated", G_CALLBACK (on_child_activated), NULL);
  g_signal_connect (flowbox, "selected-children-changed", G_CALLBACK (on_selected_children_changed), NULL);

  /* Add Flowbox test control frame */
  expander = bobgui_expander_new ("Flow Box controls");
  bobgui_expander_set_expanded (BOBGUI_EXPANDER (expander), TRUE);
  flowbox_cntl = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_expander_set_child (BOBGUI_EXPANDER (expander), flowbox_cntl);
  bobgui_box_append (BOBGUI_BOX (vbox), expander);

  widget = bobgui_check_button_new_with_label ("Homogeneous");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);

  bobgui_widget_set_tooltip_text (widget, "Set whether the items should be displayed at the same size");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (homogeneous_toggled), flowbox);

  widget = bobgui_check_button_new_with_label ("Activate on single click");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);
  g_object_bind_property (widget, "active",
                          flowbox, "activate-on-single-click",
                          G_BINDING_SYNC_CREATE);
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  /* Add alignment controls */
  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Fill");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Start");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "End");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Center");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), INITIAL_HALIGN);

  bobgui_widget_set_tooltip_text (widget, "Set the horizontal alignment policy");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (horizontal_alignment_changed), flowbox);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Fill");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Start");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "End");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Center");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), INITIAL_VALIGN);

  bobgui_widget_set_tooltip_text (widget, "Set the vertical alignment policy");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (vertical_alignment_changed), flowbox);

  /* Add Orientation control */
  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Horizontal");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Vertical");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);

  bobgui_widget_set_tooltip_text (widget, "Set the flowbox orientation");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), flowbox);

  /* Add selection mode control */
  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "None");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Single");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Browse");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Multiple");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 1);

  bobgui_widget_set_tooltip_text (widget, "Set the selection mode");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (selection_mode_changed), flowbox);

  /* Add minimum line length in items control */
  widget = bobgui_spin_button_new_with_range (1, 10, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), INITIAL_MINIMUM_LENGTH);

  bobgui_widget_set_tooltip_text (widget, "Set the minimum amount of items per line before wrapping");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (line_length_changed), flowbox);
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (line_length_changed), flowbox);

  /* Add natural line length in items control */
  widget = bobgui_spin_button_new_with_range (1, 10, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), INITIAL_MAXIMUM_LENGTH);

  bobgui_widget_set_tooltip_text (widget, "Set the natural amount of items per line ");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (max_line_length_changed), flowbox);
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (max_line_length_changed), flowbox);

  /* Add horizontal/vertical spacing controls */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("H Spacing");
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (0, 30, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), INITIAL_CSPACING);

  bobgui_widget_set_tooltip_text (widget, "Set the horizontal spacing between children");
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (BOBGUI_ORIENTATION_HORIZONTAL));
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (BOBGUI_ORIENTATION_HORIZONTAL));

  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), hbox);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("V Spacing");
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (0, 30, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), INITIAL_RSPACING);

  bobgui_widget_set_tooltip_text (widget, "Set the vertical spacing between children");
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (BOBGUI_ORIENTATION_VERTICAL));
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (BOBGUI_ORIENTATION_VERTICAL));

  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), hbox);

  /* filtering and sorting */

  widget = bobgui_check_button_new_with_label ("Filter");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);

  bobgui_widget_set_tooltip_text (widget, "Set whether some items should be filtered out");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (filter_toggled), flowbox);

  widget = bobgui_check_button_new_with_label ("Sort");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);

  bobgui_widget_set_tooltip_text (widget, "Set whether items should be sorted");
  bobgui_box_append (BOBGUI_BOX (flowbox_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (sort_toggled), flowbox);


  /* Add test items control frame */
  expander = bobgui_expander_new ("Test item controls");
  bobgui_expander_set_expanded (BOBGUI_EXPANDER (expander), TRUE);
  items_cntl = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_expander_set_child (BOBGUI_EXPANDER (expander), items_cntl);
  bobgui_box_append (BOBGUI_BOX (vbox), expander);

  /* Add Items control */
  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Simple");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Focus");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Wrappy");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Images");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Buttons");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);

  bobgui_widget_set_tooltip_text (widget, "Set the item set to use");
  bobgui_box_append (BOBGUI_BOX (items_cntl), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (items_changed), flowbox);

  populate_items (BOBGUI_FLOW_BOX (flowbox));

  /* This line was added only for the convenience of reproducing
   * a height-for-width inside BobguiScrolledWindow bug (bug 629778).
   *   -Tristan
   */
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 390, -1);

  return window;
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
main (int argc, char *argv[])
{
  BobguiWidget *window;
  gboolean done = FALSE;

  bobgui_init ();

  window = create_window ();

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
