/* bobguiaccessible.h: Accessible interface
 *
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib-object.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACCESSIBLE (bobgui_accessible_get_type())

GDK_AVAILABLE_IN_4_10
G_DECLARE_INTERFACE (BobguiAccessible, bobgui_accessible, BOBGUI, ACCESSIBLE, GObject)

/**
 * BobguiAccessiblePlatformState:
 * @BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE: whether the accessible can be focused
 * @BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED: whether the accessible has focus
 * @BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE: whether the accessible is active
 *
 * The various platform states which can be queried
 * using [method@Bobgui.Accessible.get_platform_state].
 *
 * Since: 4.10
 */
typedef enum {
  BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE,
  BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED,
  BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE
} BobguiAccessiblePlatformState;

/**
 * BobguiAccessibleInterface:
 * @get_at_context: retrieve the platform-specific accessibility context
 *   for the accessible implementation
 * @get_platform_state: retrieve the accessible state
 *
 * The common interface for accessible objects.
 *
 * Since: 4.10
 */
struct _BobguiAccessibleInterface
{
  GTypeInterface g_iface;

  /**
   * BobguiAccessibleInterface::get_at_context:
   * @self: an accessible object
   *
   * Retrieves the platform-specific accessibility context for the
   * accessible implementation.
   *
   * Returns: (transfer full) (nullable): the accessibility context
   *
   * Since: 4.10
   */
  BobguiATContext * (* get_at_context) (BobguiAccessible *self);

  /**
   * BobguiAccessibleInterface::get_platform_state:
   * @self: an accessible object
   * @state: the state to query
   *
   * Checks if the given @state applies to the accessible object.
   *
   * Returns: true if the @state is set, and false otherwise
   *
   * Since: 4.10
   */
  gboolean (* get_platform_state) (BobguiAccessible              *self,
                                   BobguiAccessiblePlatformState  state);

  /**
   * BobguiAccessibleInterface::get_accessible_parent:
   * @self: an accessible object
   *
   * Retrieves the accessible parent of an accessible object.
   *
   * This virtual function should return `NULL` for top level objects.
   *
   * Returns: (nullable) (transfer full): the accessible parent
   *
   * Since: 4.10
   */
  BobguiAccessible * (* get_accessible_parent) (BobguiAccessible *self);

  /**
   * BobguiaccessibleInterface::get_first_accessible_child:
   * @self: an accessible object
   *
   * Retrieves the first accessible child of an accessible object.
   *
   * Returns: (transfer full) (nullable): an accessible object
   *
   * Since: 4.10
   */
  BobguiAccessible * (* get_first_accessible_child) (BobguiAccessible *self);

  /**
   * BobguiaccessibleInterface::get_next_accessible_sibling:
   * @self: an accessible object
   *
   * Retrieves the next accessible sibling of an accessible object.
   *
   * Returns: (transfer full) (nullable): an accessible object
   *
   * Since: 4.10
   */
  BobguiAccessible * (* get_next_accessible_sibling) (BobguiAccessible *self);

  /**
   * BobguiAccessibleInterface::get_bounds:
   * @self: an accessible object
   * @x: (out): the horizontal coordinate of a rectangle
   * @y: (out): the vertical coordinate of a rectangle
   * @width: (out): the width of a rectangle
   * @height: (out): the height of a rectangle
   *
   * Retrieves the dimensions and position of an accessible object in its
   * parent's coordinate space, if those values can be determined.
   *
   * For top level accessible objects, the X and Y coordinates are always
   * going to be set to zero.
   *
   * Returns: true if the values are value, and false otherwise
   */
  gboolean (* get_bounds) (BobguiAccessible *self,
                           int           *x,
                           int           *y,
                           int           *width,
                           int           *height);

  /**
   * BobguiAccessibleIface::get_accessible_id:
   * @self: an accessible object
   *
   * Retrieves the accessible identifier for the accessible object.
   *
   * Returns: (transfer full) (nullable): the accessible identifier
   *
   * Since: 4.22
   */
  char * (* get_accessible_id) (BobguiAccessible *self);
};

/**
 * BobguiAccessibleList:
 *
 * Wraps a list of references to [iface@Bobgui.Accessible] objects.
 *
 * Since: 4.14
 */
typedef struct _BobguiAccessibleList BobguiAccessibleList;

GDK_AVAILABLE_IN_ALL
BobguiATContext *  bobgui_accessible_get_at_context   (BobguiAccessible *self);

GDK_AVAILABLE_IN_4_10
gboolean bobgui_accessible_get_platform_state (BobguiAccessible              *self,
                                            BobguiAccessiblePlatformState  state);

GDK_AVAILABLE_IN_4_10
BobguiAccessible * bobgui_accessible_get_accessible_parent (BobguiAccessible *self);

GDK_AVAILABLE_IN_4_10
void bobgui_accessible_set_accessible_parent (BobguiAccessible *self,
                                           BobguiAccessible *parent,
                                           BobguiAccessible *next_sibling);

GDK_AVAILABLE_IN_4_10
BobguiAccessible * bobgui_accessible_get_first_accessible_child (BobguiAccessible *self);

GDK_AVAILABLE_IN_4_10
BobguiAccessible * bobgui_accessible_get_next_accessible_sibling (BobguiAccessible *self);
GDK_AVAILABLE_IN_4_10
void bobgui_accessible_update_next_accessible_sibling (BobguiAccessible *self,
                                                    BobguiAccessible *new_sibling);


GDK_AVAILABLE_IN_4_10
gboolean bobgui_accessible_get_bounds (BobguiAccessible *self,
                                    int           *x,
                                    int           *y,
                                    int           *width,
                                    int           *height);

GDK_AVAILABLE_IN_4_22
char * bobgui_accessible_get_accessible_id (BobguiAccessible *self);

GDK_AVAILABLE_IN_ALL
BobguiAccessibleRole bobgui_accessible_get_accessible_role (BobguiAccessible *self);

GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_state (BobguiAccessible      *self,
                                  BobguiAccessibleState  first_state,
                                  ...);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_property (BobguiAccessible         *self,
                                     BobguiAccessibleProperty  first_property,
                                     ...);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_relation (BobguiAccessible         *self,
                                     BobguiAccessibleRelation  first_relation,
                                     ...);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_state_value (BobguiAccessible      *self,
                                        int                 n_states,
                                        BobguiAccessibleState  states[],
                                        const GValue        values[]);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_property_value (BobguiAccessible         *self,
                                           int                    n_properties,
                                           BobguiAccessibleProperty  properties[],
                                           const GValue           values[]);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_update_relation_value (BobguiAccessible         *self,
                                           int                    n_relations,
                                           BobguiAccessibleRelation  relations[],
                                           const GValue           values[]);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_reset_state (BobguiAccessible      *self,
                                 BobguiAccessibleState  state);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_reset_property (BobguiAccessible         *self,
                                    BobguiAccessibleProperty  property);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_reset_relation (BobguiAccessible         *self,
                                    BobguiAccessibleRelation  relation);

GDK_AVAILABLE_IN_ALL
void bobgui_accessible_state_init_value (BobguiAccessibleState  state,
                                      GValue             *value);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_property_init_value (BobguiAccessibleProperty  property,
                                         GValue                *value);
GDK_AVAILABLE_IN_ALL
void bobgui_accessible_relation_init_value (BobguiAccessibleRelation  relation,
                                         GValue                *value);

#define BOBGUI_ACCESSIBLE_LIST (bobgui_accessible_list_get_type())

GDK_AVAILABLE_IN_4_14
GType bobgui_accessible_list_get_type (void);

GDK_AVAILABLE_IN_4_14
GList * bobgui_accessible_list_get_objects (BobguiAccessibleList *accessible_list);

GDK_AVAILABLE_IN_4_14
BobguiAccessibleList * bobgui_accessible_list_new_from_list (GList *list);

GDK_AVAILABLE_IN_4_14
BobguiAccessibleList * bobgui_accessible_list_new_from_array (BobguiAccessible **accessibles,
                                                        gsize           n_accessibles);

GDK_AVAILABLE_IN_4_14
void bobgui_accessible_announce (BobguiAccessible                     *self,
                              const char                        *message,
                              BobguiAccessibleAnnouncementPriority  priority);

GDK_AVAILABLE_IN_4_18
void bobgui_accessible_update_platform_state (BobguiAccessible              *self,
                                           BobguiAccessiblePlatformState  state);

G_END_DECLS
