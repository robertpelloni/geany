/* Spin Buttons
 * #Keywords: BobguiEntry
 *
 * BobguiSpinButton provides convenient ways to input data
 * that can be seen as a value in a range. The examples
 * here show that this does not necessarily mean numeric
 * values, and it can include custom formatting.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>
#include <math.h>
#include <stdlib.h>

G_MODULE_EXPORT int
spinbutton_hex_spin_input (BobguiSpinButton *spin_button,
                           double        *new_val)
{
  const char *buf;
  char *err;
  double res;

  buf = bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button));
  res = strtol (buf, &err, 16);
  *new_val = res;
  if (*err)
    return BOBGUI_INPUT_ERROR;
  else
    return TRUE;
}

G_MODULE_EXPORT int
spinbutton_hex_spin_output (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  char *buf;
  double val;

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  val = bobgui_adjustment_get_value (adjustment);
  if (fabs (val) < 1e-5)
    buf = g_strdup ("0x00");
  else
    buf = g_strdup_printf ("0x%.2X", (int) val);
  if (strcmp (buf, bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
    bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), buf);
  g_free (buf);

  return TRUE;
}

G_MODULE_EXPORT int
spinbutton_time_spin_input (BobguiSpinButton *spin_button,
                            double        *new_val)
{
  const char *text;
  char **str;
  gboolean found = FALSE;
  int hours;
  int minutes;
  char *endh;
  char *endm;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button));
  str = g_strsplit (text, ":", 2);

  if (g_strv_length (str) == 2)
    {
      hours = strtol (str[0], &endh, 10);
      minutes = strtol (str[1], &endm, 10);
      if (!*endh && !*endm &&
          0 <= hours && hours < 24 &&
          0 <= minutes && minutes < 60)
        {
          *new_val = hours * 60 + minutes;
          found = TRUE;
        }
    }

  g_strfreev (str);

  if (!found)
    {
      *new_val = 0.0;
      return BOBGUI_INPUT_ERROR;
    }

  return TRUE;
}

G_MODULE_EXPORT int
spinbutton_time_spin_output (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  char *buf;
  double hours;
  double minutes;

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  hours = bobgui_adjustment_get_value (adjustment) / 60.0;
  minutes = (hours - floor (hours)) * 60.0;
  buf = g_strdup_printf ("%02.0f:%02.0f", floor (hours), floor (minutes + 0.5));
  if (strcmp (buf, bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
    bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), buf);
  g_free (buf);

  return TRUE;
}

static const char *month[12] = {
  "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};

G_MODULE_EXPORT int
spinbutton_month_spin_input (BobguiSpinButton *spin_button,
                             double        *new_val)
{
  int i;
  char *tmp1, *tmp2;
  gboolean found = FALSE;

  for (i = 1; i <= 12; i++)
    {
      tmp1 = g_ascii_strup (month[i - 1], -1);
      tmp2 = g_ascii_strup (bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button)), -1);
      if (strstr (tmp1, tmp2) == tmp1)
        found = TRUE;
      g_free (tmp1);
      g_free (tmp2);
      if (found)
        break;
    }
  if (!found)
    {
      *new_val = 0.0;
      return BOBGUI_INPUT_ERROR;
    }
  *new_val = (double) i;

  return TRUE;
}

G_MODULE_EXPORT int
spinbutton_month_spin_output (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  double value;
  int i;

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  value = bobgui_adjustment_get_value (adjustment);
  for (i = 1; i <= 12; i++)
    if (fabs (value - (double)i) < 1e-5)
      {
        if (strcmp (month[i-1], bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
          bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), month[i-1]);
      }

  return TRUE;
}

static gboolean
value_to_label (GBinding     *binding,
                const GValue *from,
                GValue       *to,
                gpointer      user_data)
{
  g_value_take_string (to, g_strdup_printf ("%g", g_value_get_double (from)));
  return TRUE;
}

BobguiWidget *
do_spinbutton (BobguiWidget *do_widget)
{
  static BobguiWidget *window;

  if (!window)
    {
      BobguiBuilder *builder;
      BobguiBuilderScope *scope;
      BobguiAdjustment *adj;
      BobguiWidget *label;

      scope = bobgui_builder_cscope_new ();
      builder = bobgui_builder_new ();
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_hex_spin_input);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_hex_spin_output);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_time_spin_input);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_time_spin_output);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_month_spin_input);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), spinbutton_month_spin_output);
      bobgui_builder_set_scope (builder, scope);
      bobgui_builder_add_from_resource (builder, "/spinbutton/spinbutton.ui", NULL);
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Spin Buttons");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      adj = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "basic_adjustment"));
      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "basic_label"));
      g_object_bind_property_full (adj, "value",
                                   label, "label",
                                   G_BINDING_SYNC_CREATE,
                                   value_to_label,
                                   NULL,
                                   NULL, NULL);
      adj = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "hex_adjustment"));
      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "hex_label"));
      g_object_bind_property_full (adj, "value",
                                   label, "label",
                                   G_BINDING_SYNC_CREATE,
                                   value_to_label,
                                   NULL,
                                   NULL, NULL);
      adj = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "time_adjustment"));
      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "time_label"));
      g_object_bind_property_full (adj, "value",
                                   label, "label",
                                   G_BINDING_SYNC_CREATE,
                                   value_to_label,
                                   NULL,
                                   NULL, NULL);
      adj = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "month_adjustment"));
      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "month_label"));
      g_object_bind_property_full (adj, "value",
                                   label, "label",
                                   G_BINDING_SYNC_CREATE,
                                   value_to_label,
                                   NULL,
                                   NULL, NULL);

      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
