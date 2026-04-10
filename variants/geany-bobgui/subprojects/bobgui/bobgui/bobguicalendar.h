/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BOBGUI Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_CALENDAR                  (bobgui_calendar_get_type ())
#define BOBGUI_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CALENDAR, BobguiCalendar))
#define BOBGUI_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CALENDAR))


typedef struct _BobguiCalendar	       BobguiCalendar;

GDK_AVAILABLE_IN_ALL
GType	   bobgui_calendar_get_type	(void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_calendar_new		(void);

GDK_DEPRECATED_IN_4_20_FOR (bobgui_calendar_set_date)
void          bobgui_calendar_select_day                  (BobguiCalendar *self,
                                                        GDateTime   *date);

GDK_AVAILABLE_IN_ALL
void       bobgui_calendar_mark_day	(BobguiCalendar *calendar,
					 guint	      day);
GDK_AVAILABLE_IN_ALL
void       bobgui_calendar_unmark_day	(BobguiCalendar *calendar,
					 guint	      day);
GDK_AVAILABLE_IN_ALL
void	   bobgui_calendar_clear_marks	(BobguiCalendar *calendar);

GDK_AVAILABLE_IN_ALL
void          bobgui_calendar_set_show_week_numbers       (BobguiCalendar *self,
                                                        gboolean     value);
GDK_AVAILABLE_IN_ALL
gboolean      bobgui_calendar_get_show_week_numbers       (BobguiCalendar *self);
GDK_AVAILABLE_IN_ALL
void          bobgui_calendar_set_show_heading            (BobguiCalendar *self,
                                                        gboolean     value);
GDK_AVAILABLE_IN_ALL
gboolean      bobgui_calendar_get_show_heading            (BobguiCalendar *self);
GDK_AVAILABLE_IN_ALL
void          bobgui_calendar_set_show_day_names          (BobguiCalendar *self,
                                                        gboolean     value);
GDK_AVAILABLE_IN_ALL
gboolean      bobgui_calendar_get_show_day_names          (BobguiCalendar *self);

GDK_AVAILABLE_IN_4_14
void          bobgui_calendar_set_day                     (BobguiCalendar *self,
                                                        int          day);

GDK_AVAILABLE_IN_4_14
int           bobgui_calendar_get_day                     (BobguiCalendar *self);

GDK_AVAILABLE_IN_4_14
void          bobgui_calendar_set_month                   (BobguiCalendar *self,
                                                        int          month);

GDK_AVAILABLE_IN_4_14
int           bobgui_calendar_get_month                   (BobguiCalendar *self);

GDK_AVAILABLE_IN_4_14
void          bobgui_calendar_set_year                    (BobguiCalendar *self,
                                                        int          year);

GDK_AVAILABLE_IN_4_14
int           bobgui_calendar_get_year                    (BobguiCalendar *self);

GDK_AVAILABLE_IN_4_20
void          bobgui_calendar_set_date                    (BobguiCalendar *self,
                                                        GDateTime   *date);
GDK_AVAILABLE_IN_ALL
GDateTime *   bobgui_calendar_get_date                    (BobguiCalendar *self);

GDK_AVAILABLE_IN_ALL
gboolean   bobgui_calendar_get_day_is_marked      (BobguiCalendar    *calendar,
                                                guint           day);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCalendar, g_object_unref)

G_END_DECLS

