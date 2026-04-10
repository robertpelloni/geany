/*
 * Copyright © 2012 Canonical Limited
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACTIONABLE                                 (bobgui_actionable_get_type ())
#define BOBGUI_ACTIONABLE(inst)                                (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             BOBGUI_TYPE_ACTIONABLE, BobguiActionable))
#define BOBGUI_IS_ACTIONABLE(inst)                             (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             BOBGUI_TYPE_ACTIONABLE))
#define BOBGUI_ACTIONABLE_GET_IFACE(inst)                      (G_TYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             BOBGUI_TYPE_ACTIONABLE, BobguiActionableInterface))

typedef struct _BobguiActionableInterface                      BobguiActionableInterface;
typedef struct _BobguiActionable                               BobguiActionable;

struct _BobguiActionableInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  const char * (* get_action_name)             (BobguiActionable *actionable);
  void          (* set_action_name)             (BobguiActionable *actionable,
                                                 const char    *action_name);
  GVariant *    (* get_action_target_value)     (BobguiActionable *actionable);
  void          (* set_action_target_value)     (BobguiActionable *actionable,
                                                 GVariant      *target_value);
};

GDK_AVAILABLE_IN_ALL
GType                   bobgui_actionable_get_type                         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
const char *            bobgui_actionable_get_action_name                  (BobguiActionable *actionable);
GDK_AVAILABLE_IN_ALL
void                    bobgui_actionable_set_action_name                  (BobguiActionable *actionable,
                                                                         const char    *action_name);

GDK_AVAILABLE_IN_ALL
GVariant *              bobgui_actionable_get_action_target_value          (BobguiActionable *actionable);
GDK_AVAILABLE_IN_ALL
void                    bobgui_actionable_set_action_target_value          (BobguiActionable *actionable,
                                                                         GVariant      *target_value);

GDK_AVAILABLE_IN_ALL
void                    bobgui_actionable_set_action_target                (BobguiActionable *actionable,
                                                                         const char    *format_string,
                                                                         ...);

GDK_AVAILABLE_IN_ALL
void                    bobgui_actionable_set_detailed_action_name         (BobguiActionable *actionable,
                                                                         const char    *detailed_action_name);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiActionable, g_object_unref)

G_END_DECLS

