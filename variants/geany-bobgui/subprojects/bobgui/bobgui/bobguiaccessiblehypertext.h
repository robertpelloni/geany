/* bobguiaccessiblehypertext.h: Interface for accessible objects containing links
 *
 * SPDX-FileCopyrightText: 2025  Red Hat, Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiaccessible.h>
#include <bobgui/bobguiaccessibletext.h>
#include <graphene.h>

G_BEGIN_DECLS

#define BOBGUI_ACCESSIBLE_HYPERLINK_TYPE (bobgui_accessible_hyperlink_get_type ())

GDK_AVAILABLE_IN_4_22
G_DECLARE_FINAL_TYPE (BobguiAccessibleHyperlink, bobgui_accessible_hyperlink, BOBGUI, ACCESSIBLE_HYPERLINK, GObject);

#define BOBGUI_TYPE_ACCESSIBLE_HYPERTEXT (bobgui_accessible_hypertext_get_type ())

/**
 * BobguiAccessibleHypertext:
 *
 * An interface for accessible objects containing links.
 *
 * The `BobguiAccessibleHypertext` interfaces is meant to be implemented by accessible
 * objects that contain links. Those links don't necessarily have to be part
 * of text, they can be associated with images and other things.
 *
 * Since: 4.22
 */
GDK_AVAILABLE_IN_4_22
G_DECLARE_INTERFACE (BobguiAccessibleHypertext, bobgui_accessible_hypertext, BOBGUI, ACCESSIBLE_HYPERTEXT, BobguiAccessible)

/**
 * BobguiAccessibleHypertextInterface:
 *
 * The interface vtable for accessible objects containing links.
 *
 * Since: 4.22
 */
struct _BobguiAccessibleHypertextInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /**
   * BobguiAccessibleHypertextInterface::get_n_links:
   * @self: the accessible object
   *
   * Retrieve the number of links in the accessible object.
   *
   * Returns: the number of links
   *
   * Since: 4.22
   */
  unsigned int (* get_n_links) (BobguiAccessibleHypertext *self);

  /**
   * BobguiAccessibleHypertextInterface::get_link:
   * @self: the accessible object
   * @index: the index of the link
   *
   * Retrieve the n-th link in the accessible object.
   *
   * @index must be smaller than the number of links.
   *
   * Returns: (transfer none): the link
   *
   * Since: 4.22
   */
  BobguiAccessibleHyperlink *
               (* get_link) (BobguiAccessibleHypertext *self,
                             unsigned int            index);

  /**
   * BobguiAccessibleTextInterface::get_link_at:
   * @self: the accessible object
   * @offset: the character offset
   *
   * Retrieves the index of the link at the given character offset.
   *
   * Note that this method will return `G_MAXUINT` if the object
   * does not contain text.
   *
   * Returns: index of the link at the given character offset, or
   *   `G_MAXUINT` if there is no link
   *
   * Since: 4.22
   */
  unsigned int (* get_link_at) (BobguiAccessibleHypertext *self,
                                unsigned int            offset);
};

GDK_AVAILABLE_IN_4_22
BobguiAccessibleHyperlink * bobgui_accessible_hyperlink_new (BobguiAccessibleHypertext *parent,
                                                       unsigned int            index,
                                                       const char             *uri,
                                                       BobguiAccessibleTextRange *bounds);

GDK_AVAILABLE_IN_4_22
void bobgui_accessible_hyperlink_set_platform_state (BobguiAccessibleHyperlink     *self,
                                                  BobguiAccessiblePlatformState  state,
                                                  gboolean                    enabled);

G_END_DECLS
