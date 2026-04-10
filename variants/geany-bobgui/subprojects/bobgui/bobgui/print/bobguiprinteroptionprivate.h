/* BOBGUI - The Bobgui Framework
 * bobguiprinteroption.h: printer option
 * Copyright (C) 2006, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/* This is a "semi-private" header; it is meant only for
 * alternate BobguiPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef BOBGUI_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "BobguiPrintBackend is not supported API for general use"
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINTER_OPTION             (bobgui_printer_option_get_type ())
#define BOBGUI_PRINTER_OPTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINTER_OPTION, BobguiPrinterOption))
#define BOBGUI_IS_PRINTER_OPTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINTER_OPTION))

typedef struct _BobguiPrinterOption       BobguiPrinterOption;
typedef struct _BobguiPrinterOptionClass  BobguiPrinterOptionClass;

#define BOBGUI_PRINTER_OPTION_GROUP_IMAGE_QUALITY "ImageQuality"
#define BOBGUI_PRINTER_OPTION_GROUP_FINISHING "Finishing"

typedef enum {
  BOBGUI_PRINTER_OPTION_TYPE_BOOLEAN,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSCODE,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE_REAL,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE_INT,
  BOBGUI_PRINTER_OPTION_TYPE_PICKONE_STRING,
  BOBGUI_PRINTER_OPTION_TYPE_ALTERNATIVE,
  BOBGUI_PRINTER_OPTION_TYPE_STRING,
  BOBGUI_PRINTER_OPTION_TYPE_FILESAVE,
  BOBGUI_PRINTER_OPTION_TYPE_INFO
} BobguiPrinterOptionType;

struct _BobguiPrinterOption
{
  GObject parent_instance;

  char *name;
  char *display_text;
  BobguiPrinterOptionType type;

  char *value;

  int num_choices;
  char **choices;
  char **choices_display;

  gboolean activates_default;

  gboolean has_conflict;
  char *group;
};

struct _BobguiPrinterOptionClass
{
  GObjectClass parent_class;

  void (*changed) (BobguiPrinterOption *option);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType   bobgui_printer_option_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiPrinterOption *bobgui_printer_option_new                    (const char           *name,
							     const char           *display_text,
							     BobguiPrinterOptionType  type);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_set                    (BobguiPrinterOption     *option,
							     const char           *value);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_set_has_conflict       (BobguiPrinterOption     *option,
							     gboolean              has_conflict);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_clear_has_conflict     (BobguiPrinterOption     *option);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_set_boolean            (BobguiPrinterOption     *option,
							     gboolean              value);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_allocate_choices       (BobguiPrinterOption     *option,
							     int                   num);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_choices_from_array     (BobguiPrinterOption     *option,
                                                             int                   num_choices,
                                                             const char           **choices,
                                                             const char           **choices_display);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_printer_option_has_choice             (BobguiPrinterOption     *option,
							    const char           *choice);
GDK_AVAILABLE_IN_ALL
void              bobgui_printer_option_set_activates_default (BobguiPrinterOption     *option,
							    gboolean              activates);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_printer_option_get_activates_default (BobguiPrinterOption     *option);


G_END_DECLS



