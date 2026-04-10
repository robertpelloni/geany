/* BobguiPrinterOptionWidget 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <bobgui/bobgui.h>
#include "bobguiprinteroptionprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINTER_OPTION_WIDGET                  (bobgui_printer_option_widget_get_type ())
#define BOBGUI_PRINTER_OPTION_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINTER_OPTION_WIDGET, BobguiPrinterOptionWidget))
#define BOBGUI_PRINTER_OPTION_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINTER_OPTION_WIDGET, BobguiPrinterOptionWidgetClass))
#define BOBGUI_IS_PRINTER_OPTION_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINTER_OPTION_WIDGET))
#define BOBGUI_IS_PRINTER_OPTION_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINTER_OPTION_WIDGET))
#define BOBGUI_PRINTER_OPTION_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINTER_OPTION_WIDGET, BobguiPrinterOptionWidgetClass))


typedef struct _BobguiPrinterOptionWidget         BobguiPrinterOptionWidget;
typedef struct _BobguiPrinterOptionWidgetClass    BobguiPrinterOptionWidgetClass;
typedef struct BobguiPrinterOptionWidgetPrivate   BobguiPrinterOptionWidgetPrivate;

struct _BobguiPrinterOptionWidget
{
  BobguiBox parent_instance;

  BobguiPrinterOptionWidgetPrivate *priv;
};

struct _BobguiPrinterOptionWidgetClass
{
  BobguiBoxClass parent_class;

  void (*changed) (BobguiPrinterOptionWidget *widget);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType	     bobgui_printer_option_widget_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_printer_option_widget_new                (BobguiPrinterOption       *source);
GDK_AVAILABLE_IN_ALL
void         bobgui_printer_option_widget_set_source         (BobguiPrinterOptionWidget *setting,
		 					   BobguiPrinterOption       *source);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_printer_option_widget_has_external_label (BobguiPrinterOptionWidget *setting);
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_printer_option_widget_get_external_label (BobguiPrinterOptionWidget *setting);
GDK_AVAILABLE_IN_ALL
const char *bobgui_printer_option_widget_get_value          (BobguiPrinterOptionWidget *setting);

G_END_DECLS

