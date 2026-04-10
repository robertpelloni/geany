/* bobguitextviewprivate.h: Private header for BobguiTextView
 *
 * Copyright (c) 2016  Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguitextview.h"
#include "bobguitextattributesprivate.h"
#include "bobguitextlayoutprivate.h"
#include "bobguicssnodeprivate.h"

G_BEGIN_DECLS

BobguiCssNode *    bobgui_text_view_get_text_node             (BobguiTextView *text_view);
BobguiCssNode *    bobgui_text_view_get_selection_node        (BobguiTextView *text_view);

BobguiTextAttributes * bobgui_text_view_get_default_attributes (BobguiTextView *text_view);

BobguiEventController *bobgui_text_view_get_key_controller    (BobguiTextView *text_view);

GHashTable *    bobgui_text_view_get_attributes_run        (BobguiTextView *self,
                                                         int          offset,
                                                         gboolean     include_defaults,
                                                         int         *start,
                                                         int         *end);
void            bobgui_text_view_add_default_attributes    (BobguiTextView *view,
                                                         GHashTable  *attributes);

BobguiTextLayout *
bobgui_text_view_get_layout (BobguiTextView *text_view);

G_END_DECLS

