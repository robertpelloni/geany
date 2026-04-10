/* bobguicombobox.h
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@bobgui.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguitreeview.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COMBO_BOX             (bobgui_combo_box_get_type ())
#define BOBGUI_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COMBO_BOX, BobguiComboBox))
#define BOBGUI_COMBO_BOX_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), BOBGUI_TYPE_COMBO_BOX, BobguiComboBoxClass))
#define BOBGUI_IS_COMBO_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COMBO_BOX))
#define BOBGUI_IS_COMBO_BOX_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), BOBGUI_TYPE_COMBO_BOX))
#define BOBGUI_COMBO_BOX_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), BOBGUI_TYPE_COMBO_BOX, BobguiComboBoxClass))

typedef struct _BobguiComboBox        BobguiComboBox;
typedef struct _BobguiComboBoxClass   BobguiComboBoxClass;

struct _BobguiComboBox
{
  BobguiWidget parent_instance;
};

/**
 * BobguiComboBoxClass:
 * @parent_class: The parent class.
 * @changed: Signal is emitted when the active item is changed.
 * @format_entry_text: Signal which allows you to change how the text
 *    displayed in a combo box’s entry is displayed.
 */
struct _BobguiComboBoxClass
{
  BobguiWidgetClass parent_class;

  /*< public >*/

  /* signals */
  void     (* changed)           (BobguiComboBox *combo_box);
  char    *(* format_entry_text) (BobguiComboBox *combo_box,
                                  const char *path);
  void     (* activate)          (BobguiComboBox *combo_box);

  /*< private >*/

  gpointer padding[7];
};


/* construction */
GDK_AVAILABLE_IN_ALL
GType         bobgui_combo_box_get_type                 (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiWidget    *bobgui_combo_box_new                      (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiWidget    *bobgui_combo_box_new_with_entry           (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiWidget    *bobgui_combo_box_new_with_model           (BobguiTreeModel *model);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiWidget    *bobgui_combo_box_new_with_model_and_entry (BobguiTreeModel *model);

/* get/set active item */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
int           bobgui_combo_box_get_active       (BobguiComboBox     *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_set_active       (BobguiComboBox     *combo_box,
                                              int              index_);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
gboolean      bobgui_combo_box_get_active_iter  (BobguiComboBox     *combo_box,
                                              BobguiTreeIter     *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_set_active_iter  (BobguiComboBox     *combo_box,
                                              BobguiTreeIter     *iter);

/* getters and setters */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_set_model        (BobguiComboBox     *combo_box,
                                              BobguiTreeModel    *model);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiTreeModel *bobgui_combo_box_get_model        (BobguiComboBox     *combo_box);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiTreeViewRowSeparatorFunc bobgui_combo_box_get_row_separator_func (BobguiComboBox                *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void                        bobgui_combo_box_set_row_separator_func (BobguiComboBox                *combo_box,
                                                                  BobguiTreeViewRowSeparatorFunc func,
                                                                  gpointer                    data,
                                                                  GDestroyNotify              destroy);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void               bobgui_combo_box_set_button_sensitivity (BobguiComboBox        *combo_box,
                                                         BobguiSensitivityType  sensitivity);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiSensitivityType bobgui_combo_box_get_button_sensitivity (BobguiComboBox        *combo_box);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
gboolean           bobgui_combo_box_get_has_entry          (BobguiComboBox        *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void               bobgui_combo_box_set_entry_text_column  (BobguiComboBox        *combo_box,
                                                         int                 text_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
int                bobgui_combo_box_get_entry_text_column  (BobguiComboBox        *combo_box);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void               bobgui_combo_box_set_popup_fixed_width  (BobguiComboBox      *combo_box,
                                                         gboolean          fixed);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
gboolean           bobgui_combo_box_get_popup_fixed_width  (BobguiComboBox      *combo_box);

/* programmatic control */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_popup            (BobguiComboBox     *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_popup_for_device (BobguiComboBox     *combo_box,
                                              GdkDevice       *device);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_popdown          (BobguiComboBox     *combo_box);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
int           bobgui_combo_box_get_id_column        (BobguiComboBox *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_set_id_column        (BobguiComboBox *combo_box,
                                                  int          id_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
const char * bobgui_combo_box_get_active_id        (BobguiComboBox *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
gboolean      bobgui_combo_box_set_active_id        (BobguiComboBox *combo_box,
                                                  const char *active_id);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
void          bobgui_combo_box_set_child            (BobguiComboBox *combo_box,
                                                  BobguiWidget   *child);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown)
BobguiWidget *   bobgui_combo_box_get_child            (BobguiComboBox *combo_box);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiComboBox, g_object_unref)

G_END_DECLS

