/* BOBGUI - The Bobgui Framework
 * bobguiprinteroptionset.h: printer option set
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
#include "bobguiprinteroptionprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINTER_OPTION_SET             (bobgui_printer_option_set_get_type ())
#define BOBGUI_PRINTER_OPTION_SET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINTER_OPTION_SET, BobguiPrinterOptionSet))
#define BOBGUI_IS_PRINTER_OPTION_SET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINTER_OPTION_SET))

typedef struct _BobguiPrinterOptionSet       BobguiPrinterOptionSet;
typedef struct _BobguiPrinterOptionSetClass  BobguiPrinterOptionSetClass;

struct _BobguiPrinterOptionSet
{
  GObject parent_instance;

  /*< private >*/
  GPtrArray *array;
  GHashTable *hash;
};

struct _BobguiPrinterOptionSetClass
{
  GObjectClass parent_class;

  void (*changed) (BobguiPrinterOptionSet *option);


  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

typedef void (*BobguiPrinterOptionSetFunc) (BobguiPrinterOption  *option,
					 gpointer           user_data);


GDK_AVAILABLE_IN_ALL
GType   bobgui_printer_option_set_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiPrinterOptionSet *bobgui_printer_option_set_new              (void);
GDK_AVAILABLE_IN_ALL
void                 bobgui_printer_option_set_add              (BobguiPrinterOptionSet     *set,
							      BobguiPrinterOption        *option);
GDK_AVAILABLE_IN_ALL
void                 bobgui_printer_option_set_remove           (BobguiPrinterOptionSet     *set,
							      BobguiPrinterOption        *option);
GDK_AVAILABLE_IN_ALL
BobguiPrinterOption *   bobgui_printer_option_set_lookup           (BobguiPrinterOptionSet     *set,
							      const char              *name);
GDK_AVAILABLE_IN_ALL
void                 bobgui_printer_option_set_foreach          (BobguiPrinterOptionSet     *set,
							      BobguiPrinterOptionSetFunc  func,
							      gpointer                 user_data);
GDK_AVAILABLE_IN_ALL
void                 bobgui_printer_option_set_clear_conflicts  (BobguiPrinterOptionSet     *set);
GDK_AVAILABLE_IN_ALL
GList *              bobgui_printer_option_set_get_groups       (BobguiPrinterOptionSet     *set);
GDK_AVAILABLE_IN_ALL
void                 bobgui_printer_option_set_foreach_in_group (BobguiPrinterOptionSet     *set,
							      const char              *group,
							      BobguiPrinterOptionSetFunc  func,
							      gpointer                 user_data);

G_END_DECLS

