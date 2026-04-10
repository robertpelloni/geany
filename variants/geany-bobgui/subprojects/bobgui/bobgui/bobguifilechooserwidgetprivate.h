/* bobguifilechooserwidgetprivate.h
 *
 * Copyright (C) 2015 Red Hat
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen
 */

#pragma once

#include <glib.h>
#include "deprecated/bobguifilechooserwidget.h"
#include "bobguiselectionmodel.h"

G_BEGIN_DECLS

void
bobgui_file_chooser_widget_set_save_entry (BobguiFileChooserWidget *chooser,
                                        BobguiWidget            *entry);

gboolean
bobgui_file_chooser_widget_should_respond (BobguiFileChooserWidget *chooser);

void
bobgui_file_chooser_widget_initial_focus  (BobguiFileChooserWidget *chooser);

GSList *
bobgui_file_chooser_widget_get_selected_files (BobguiFileChooserWidget *impl);

BobguiSelectionModel *
bobgui_file_chooser_widget_get_selection_model (BobguiFileChooserWidget *chooser);

G_END_DECLS

