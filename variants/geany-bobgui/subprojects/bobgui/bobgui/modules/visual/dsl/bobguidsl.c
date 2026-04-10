#include "bobguidsl.h"
#include <stdarg.h>

BobguiWidget *
bobgui_dsl_box (BobguiOrientation orientation, ...)
{
  va_list args;
  va_start (args, orientation);
  BobguiBox *box = BOBGUI_BOX (bobgui_box_new (orientation, 0));
  BobguiWidget *child;
  while ((child = va_arg (args, BobguiWidget *)) != NULL)
    bobgui_box_append (box, child);
  va_end (args);
  return BOBGUI_WIDGET (box);
}

BobguiWidget *
bobgui_dsl_button (const char *label, GCallback callback)
{
  BobguiWidget *button = bobgui_button_new_with_label (label);
  if (callback)
    g_signal_connect (button, "clicked", callback, NULL);
  return button;
}

BobguiWidget *
bobgui_dsl_label (const char *text)
{
  return bobgui_label_new (text);
}

BobguiWidget *
bobgui_dsl_set_padding (BobguiWidget *widget, int padding)
{
  bobgui_widget_set_margin_start (widget, padding);
  bobgui_widget_set_margin_end (widget, padding);
  bobgui_widget_set_margin_top (widget, padding);
  bobgui_widget_set_margin_bottom (widget, padding);
  return widget;
}
