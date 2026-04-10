/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#define BOBGUI_TYPE_PROGRESS_BAR            (bobgui_progress_bar_get_type ())
#define BOBGUI_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PROGRESS_BAR, BobguiProgressBar))
#define BOBGUI_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PROGRESS_BAR))


typedef struct _BobguiProgressBar              BobguiProgressBar;


GDK_AVAILABLE_IN_ALL
GType      bobgui_progress_bar_get_type             (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_progress_bar_new                  (void);

GDK_AVAILABLE_IN_ALL
void       bobgui_progress_bar_pulse                (BobguiProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
void       bobgui_progress_bar_set_text             (BobguiProgressBar *pbar,
                                                  const char     *text);
GDK_AVAILABLE_IN_ALL
void       bobgui_progress_bar_set_fraction         (BobguiProgressBar *pbar,
                                                  double          fraction);

GDK_AVAILABLE_IN_ALL
void       bobgui_progress_bar_set_pulse_step       (BobguiProgressBar *pbar,
                                                  double          fraction);
GDK_AVAILABLE_IN_ALL
void       bobgui_progress_bar_set_inverted         (BobguiProgressBar *pbar,
                                                  gboolean        inverted);

GDK_AVAILABLE_IN_ALL
const char *      bobgui_progress_bar_get_text       (BobguiProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
double             bobgui_progress_bar_get_fraction   (BobguiProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
double             bobgui_progress_bar_get_pulse_step (BobguiProgressBar *pbar);

GDK_AVAILABLE_IN_ALL
gboolean           bobgui_progress_bar_get_inverted    (BobguiProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
void               bobgui_progress_bar_set_ellipsize (BobguiProgressBar     *pbar,
                                                   PangoEllipsizeMode  mode);
GDK_AVAILABLE_IN_ALL
PangoEllipsizeMode bobgui_progress_bar_get_ellipsize (BobguiProgressBar     *pbar);

GDK_AVAILABLE_IN_ALL
void               bobgui_progress_bar_set_show_text (BobguiProgressBar     *pbar,
                                                   gboolean            show_text);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_progress_bar_get_show_text (BobguiProgressBar     *pbar);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiProgressBar, g_object_unref)

G_END_DECLS

