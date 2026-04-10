/* Error States
 *
 * BobguiLabel and BobguiEntry can indicate errors if you set the .error
 * style class on them.
 *
 * This examples shows how this can be used in a dialog for input validation.
 *
 * It also shows how pass callbacks and objects to BobguiBuilder with
 * BobguiBuilderScope and bobgui_builder_expose_object().
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

static void
validate_more_details (BobguiEntry   *entry,
                       GParamSpec *pspec,
                       BobguiEntry   *details)
{
  if (strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))) > 0 &&
      strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (details))) == 0)
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), "Must have details first");
      bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "error");
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (entry),
                                   BOBGUI_ACCESSIBLE_STATE_INVALID, BOBGUI_ACCESSIBLE_INVALID_TRUE,
                                   -1);
    }
  else
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), "");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (entry), "error");
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (entry), BOBGUI_ACCESSIBLE_STATE_INVALID);
    }
}

static gboolean
mode_switch_state_set (BobguiSwitch *sw,
                       gboolean   state,
                       BobguiWidget *scale)
{
  BobguiWidget *label;

  label = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (sw), "error_label"));

  if (!state ||
      (bobgui_range_get_value (BOBGUI_RANGE (scale)) > 50))
    {
      bobgui_widget_set_visible (label, FALSE);
      bobgui_switch_set_state (sw, state);
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (sw), BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE);
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (sw), BOBGUI_ACCESSIBLE_STATE_INVALID);
    }
  else
    {
      bobgui_widget_set_visible (label, TRUE);
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (sw),
                                      BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE, label, NULL,
                                      -1);
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (sw),
                                   BOBGUI_ACCESSIBLE_STATE_INVALID, BOBGUI_ACCESSIBLE_INVALID_TRUE,
                                   -1);
    }

  return TRUE;
}

static void
level_scale_value_changed (BobguiRange *range,
                           BobguiWidget *sw)
{
  BobguiWidget *label;

  label = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (sw), "error_label"));

  if (bobgui_switch_get_active (BOBGUI_SWITCH (sw)) &&
      !bobgui_switch_get_state (BOBGUI_SWITCH (sw)) &&
      (bobgui_range_get_value (range) > 50))
    {
      bobgui_widget_set_visible (label, FALSE);
      bobgui_switch_set_state (BOBGUI_SWITCH (sw), TRUE);
    }
  else if (bobgui_switch_get_state (BOBGUI_SWITCH (sw)) &&
          (bobgui_range_get_value (range) <= 50))
    {
      bobgui_switch_set_state (BOBGUI_SWITCH (sw), FALSE);
    }

  bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (sw), BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE);
  bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (sw), BOBGUI_ACCESSIBLE_STATE_INVALID);
}

BobguiWidget *
do_errorstates (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *toplevel;
      BobguiBuilder *builder;
      BobguiBuilderScope *scope;
      BobguiWidget *sw, *label;

      toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (do_widget));

      scope = bobgui_builder_cscope_new ();
      bobgui_builder_cscope_add_callback (scope, validate_more_details);
      bobgui_builder_cscope_add_callback (scope, mode_switch_state_set);
      bobgui_builder_cscope_add_callback (scope, level_scale_value_changed);
      builder = bobgui_builder_new ();
      bobgui_builder_set_scope (builder, scope);
      bobgui_builder_expose_object (builder, "toplevel", G_OBJECT (toplevel));
      bobgui_builder_add_from_resource (builder, "/errorstates/errorstates.ui", NULL);

      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "dialog"));

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      sw = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "mode_switch"));
      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "error_label"));
      g_object_set_data (G_OBJECT (sw), "error_label", label);

      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
