/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "deprecated/bobguifontchooserdialog.h"
#include "bobguifilter.h"

G_BEGIN_DECLS

void bobgui_font_chooser_dialog_set_filter (BobguiFontChooserDialog *dialog,
                                         BobguiFilter            *filter);

G_END_DECLS

