
/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <gdk/gdk.h>

#include "bobguiwindow.h"
#include "bobguipointerfocusprivate.h"

G_BEGIN_DECLS

void            _bobgui_window_group_add_grab    (BobguiWindowGroup *window_group,
                                               BobguiWidget      *widget);
void            _bobgui_window_group_remove_grab (BobguiWindowGroup *window_group,
                                               BobguiWidget      *widget);

void            _bobgui_window_unset_focus_and_default (BobguiWindow *window,
                                                     BobguiWidget *widget);

void            _bobgui_window_update_focus_visible (BobguiWindow       *window,
                                                  guint            keyval,
                                                  GdkModifierType  state,
                                                  gboolean         visible);

void            _bobgui_window_set_allocation         (BobguiWindow     *window,
                                                    int            width,
                                                    int            height,
                                                    BobguiAllocation *allocation_out);

typedef void (*BobguiWindowKeysForeachFunc) (BobguiWindow      *window,
                                          guint           keyval,
                                          GdkModifierType modifiers,
                                          gpointer        data);

gboolean bobgui_window_emit_close_request (BobguiWindow *window);

/* --- internal (BobguiAcceleratable) --- */
void            _bobgui_window_schedule_mnemonics_visible (BobguiWindow *window);

void            _bobgui_window_notify_keys_changed (BobguiWindow *window);

void            _bobgui_window_toggle_maximized (BobguiWindow *window);

/* Window groups */

BobguiWindowGroup *_bobgui_window_get_window_group (BobguiWindow *window);

void            _bobgui_window_set_window_group (BobguiWindow      *window,
                                              BobguiWindowGroup *group);


GdkPaintable *    bobgui_window_get_icon_for_size (BobguiWindow *window,
                                                int        size);

/* Exported handles */

typedef void (*BobguiWindowHandleExported)  (BobguiWindow               *window,
                                          const char              *handle,
                                          gpointer                 user_data);

gboolean      bobgui_window_export_handle   (BobguiWindow               *window,
                                          BobguiWindowHandleExported  callback,
                                          gpointer                 user_data);
void          bobgui_window_unexport_handle (BobguiWindow               *window,
                                          const char              *handle);

BobguiWidget *      bobgui_window_lookup_pointer_focus_widget (BobguiWindow        *window,
                                                         GdkDevice        *device,
                                                         GdkEventSequence *sequence);
BobguiWidget *      bobgui_window_lookup_effective_pointer_focus_widget (BobguiWindow        *window,
                                                                   GdkDevice        *device,
                                                                   GdkEventSequence *sequence);
BobguiWidget *      bobgui_window_lookup_pointer_focus_implicit_grab (BobguiWindow        *window,
                                                                GdkDevice        *device,
                                                                GdkEventSequence *sequence);

void             bobgui_window_update_pointer_focus (BobguiWindow        *window,
                                                  GdkDevice        *device,
                                                  GdkEventSequence *sequence,
                                                  BobguiWidget        *target,
                                                  double            x,
                                                  double            y);
void             bobgui_window_set_pointer_focus_grab (BobguiWindow        *window,
                                                    GdkDevice        *device,
                                                    GdkEventSequence *sequence,
                                                    BobguiWidget        *grab_widget);

void             bobgui_window_update_pointer_focus_on_state_change (BobguiWindow *window,
                                                                  BobguiWidget *widget);

void             bobgui_window_maybe_revoke_implicit_grab (BobguiWindow *window,
                                                        GdkDevice *device,
                                                        BobguiWidget *grab_widget);
void             bobgui_window_maybe_update_cursor (BobguiWindow *window,
                                                 BobguiWidget *widget,
                                                 GdkDevice *device);
BobguiWidget *      bobgui_window_pick_popover (BobguiWindow   *window,
                                          double       x,
                                          double       y,
                                          BobguiPickFlags flags);
GdkDevice** bobgui_window_get_foci_on_widget (BobguiWindow *window,
                                           BobguiWidget *widget,
                                           guint     *n_devices);
void bobgui_window_grab_notify (BobguiWindow *window,
                             BobguiWidget *old_grab_widget,
                             BobguiWidget *new_grab_widget,
                             gboolean   from_grab);

G_END_DECLS

