/* bobguiaccessiblehypertextprivate.h: Private definitions for BobguiAccessibleHypertext
 *
 * SPDX-FileCopyrightText: 2025 Red Hat, Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "bobguiaccessiblehypertext.h"

G_BEGIN_DECLS

unsigned int bobgui_accessible_hypertext_get_n_links (BobguiAccessibleHypertext *self);
BobguiAccessibleHyperlink *
             bobgui_accessible_hypertext_get_link    (BobguiAccessibleHypertext *self,
                                                   unsigned int            index);
unsigned int bobgui_accessible_hypertext_get_link_at (BobguiAccessibleHypertext *self,
                                                   unsigned int            offset);

unsigned int bobgui_accessible_hyperlink_get_index   (BobguiAccessibleHyperlink *self);
const char * bobgui_accessible_hyperlink_get_uri     (BobguiAccessibleHyperlink *self);
void         bobgui_accessible_hyperlink_get_extents (BobguiAccessibleHyperlink *self,
                                                   BobguiAccessibleTextRange *bounds);

G_END_DECLS
