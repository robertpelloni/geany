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

#include <bobgui/bobguiaccessible.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EDITABLE             (bobgui_editable_get_type ())
#define BOBGUI_EDITABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_EDITABLE, BobguiEditable))
#define BOBGUI_IS_EDITABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_EDITABLE))
#define BOBGUI_EDITABLE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_EDITABLE, BobguiEditableInterface))

typedef struct _BobguiEditable          BobguiEditable;         /* Dummy typedef */
typedef struct _BobguiEditableInterface BobguiEditableInterface;

struct _BobguiEditableInterface
{
  GTypeInterface                   base_iface;

  /* signals */
  void (* insert_text)              (BobguiEditable    *editable,
                                     const char     *text,
                                     int             length,
                                     int            *position);
  void (* delete_text)              (BobguiEditable    *editable,
                                     int             start_pos,
                                     int             end_pos);
  void (* changed)                  (BobguiEditable    *editable);

  /* vtable */
  const char * (* get_text)         (BobguiEditable    *editable);
  void     (* do_insert_text)       (BobguiEditable    *editable,
                                     const char     *text,
                                     int             length,
                                     int            *position);
  void     (* do_delete_text)       (BobguiEditable    *editable,
                                     int             start_pos,
                                     int             end_pos);

  gboolean (* get_selection_bounds) (BobguiEditable    *editable,
                                     int            *start_pos,
                                     int            *end_pos);
  void     (* set_selection_bounds) (BobguiEditable    *editable,
                                     int             start_pos,
                                     int             end_pos);
  BobguiEditable * (* get_delegate)    (BobguiEditable    *editable);
};

GDK_AVAILABLE_IN_ALL
GType    bobgui_editable_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
const char * bobgui_editable_get_text         (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_text             (BobguiEditable *editable,
                                            const char  *text);
GDK_AVAILABLE_IN_ALL
char *   bobgui_editable_get_chars            (BobguiEditable *editable,
                                            int          start_pos,
                                            int          end_pos) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_insert_text          (BobguiEditable *editable,
                                            const char  *text,
                                            int          length,
                                            int         *position);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_delete_text          (BobguiEditable *editable,
                                            int          start_pos,
                                            int          end_pos);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_editable_get_selection_bounds (BobguiEditable *editable,
                                            int         *start_pos,
                                            int         *end_pos);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_delete_selection     (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_select_region        (BobguiEditable *editable,
                                            int          start_pos,
                                            int          end_pos);

GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_position         (BobguiEditable *editable,
                                            int          position);
GDK_AVAILABLE_IN_ALL
int      bobgui_editable_get_position         (BobguiEditable *editable);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_editable_get_editable         (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_editable         (BobguiEditable *editable,
                                            gboolean     is_editable);

GDK_AVAILABLE_IN_ALL
float    bobgui_editable_get_alignment        (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_alignment        (BobguiEditable *editable,
                                            float        xalign);

GDK_AVAILABLE_IN_ALL
int      bobgui_editable_get_width_chars      (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_width_chars      (BobguiEditable *editable,
                                            int          n_chars);

GDK_AVAILABLE_IN_ALL
int      bobgui_editable_get_max_width_chars  (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_max_width_chars  (BobguiEditable *editable,
                                            int          n_chars);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_editable_get_enable_undo      (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void     bobgui_editable_set_enable_undo      (BobguiEditable *editable,
                                            gboolean     enable_undo);

/* api for implementations */

/**
 * BobguiEditableProperties:
 * @BOBGUI_EDITABLE_PROP_TEXT: the property id for [property@Bobgui.Editable:text]
 * @BOBGUI_EDITABLE_PROP_CURSOR_POSITION: the property id for [property@Bobgui.Editable:cursor-position]
 * @BOBGUI_EDITABLE_PROP_SELECTION_BOUND: the property id for [property@Bobgui.Editable:selection-bound]
 * @BOBGUI_EDITABLE_PROP_EDITABLE: the property id for [property@Bobgui.Editable:editable]
 * @BOBGUI_EDITABLE_PROP_WIDTH_CHARS: the property id for [property@Bobgui.Editable:width-chars]
 * @BOBGUI_EDITABLE_PROP_MAX_WIDTH_CHARS: the property id for [property@Bobgui.Editable:max-width-chars]
 * @BOBGUI_EDITABLE_PROP_XALIGN: the property id for [property@Bobgui.Editable:xalign]
 * @BOBGUI_EDITABLE_PROP_ENABLE_UNDO: the property id for [property@Bobgui.Editable:enable-undo]
 * @BOBGUI_EDITABLE_NUM_PROPERTIES: the number of properties
 *
 * The identifiers for [iface@Bobgui.Editable] properties.
 *
 * See [func@Bobgui.Editable.install_properties] for details on how to
 * implement the `BobguiEditable` interface.
 */
typedef enum {
  BOBGUI_EDITABLE_PROP_TEXT,
  BOBGUI_EDITABLE_PROP_CURSOR_POSITION,
  BOBGUI_EDITABLE_PROP_SELECTION_BOUND,
  BOBGUI_EDITABLE_PROP_EDITABLE,
  BOBGUI_EDITABLE_PROP_WIDTH_CHARS,
  BOBGUI_EDITABLE_PROP_MAX_WIDTH_CHARS,
  BOBGUI_EDITABLE_PROP_XALIGN,
  BOBGUI_EDITABLE_PROP_ENABLE_UNDO,
  BOBGUI_EDITABLE_NUM_PROPERTIES
} BobguiEditableProperties;

GDK_AVAILABLE_IN_ALL
guint        bobgui_editable_install_properties    (GObjectClass *object_class,
                                                 guint         first_prop);
GDK_AVAILABLE_IN_ALL
BobguiEditable *bobgui_editable_get_delegate          (BobguiEditable *editable);
GDK_AVAILABLE_IN_ALL
void         bobgui_editable_init_delegate         (BobguiEditable  *editable);
GDK_AVAILABLE_IN_ALL
void         bobgui_editable_finish_delegate       (BobguiEditable  *editable);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_editable_delegate_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_editable_delegate_get_property (GObject      *object,
                                                 guint         prop_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);
GDK_AVAILABLE_IN_4_10
gboolean bobgui_editable_delegate_get_accessible_platform_state (BobguiEditable                *editable,
                                                              BobguiAccessiblePlatformState  state);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiEditable, g_object_unref)

G_END_DECLS

