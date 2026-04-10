/* BOBGUI - The Bobgui Framework
 * bobguifontchooser.h - Abstract interface for font file selectors GUIs
 *
 * Copyright (C) 2006, Emmanuele Bassi
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

/**
 * BobguiFontFilterFunc:
 * @family: a `PangoFontFamily`
 * @face: a `PangoFontFace` belonging to @family
 * @data: (closure): user data passed to bobgui_font_chooser_set_filter_func()
 *
 * The type of function that is used for deciding what fonts get
 * shown in a `BobguiFontChooser`.
 *
 * See [method@Bobgui.FontChooser.set_filter_func].
 *
 * Returns: %TRUE if the font should be displayed
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean (*BobguiFontFilterFunc) (const PangoFontFamily *family,
                                       const PangoFontFace   *face,
                                       gpointer               data);

/**
 * BobguiFontChooserLevel:
 * @BOBGUI_FONT_CHOOSER_LEVEL_FAMILY: Allow selecting a font family
 * @BOBGUI_FONT_CHOOSER_LEVEL_STYLE: Allow selecting a specific font face
 * @BOBGUI_FONT_CHOOSER_LEVEL_SIZE: Allow selecting a specific font size
 * @BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS: Allow changing OpenType font variation axes
 * @BOBGUI_FONT_CHOOSER_LEVEL_FEATURES: Allow selecting specific OpenType font features
 *
 * Specifies the granularity of font selection
 * that is desired in a `BobguiFontChooser`.
 *
 * This enumeration may be extended in the future; applications should
 * ignore unknown values.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum {
  BOBGUI_FONT_CHOOSER_LEVEL_FAMILY     = 0,
  BOBGUI_FONT_CHOOSER_LEVEL_STYLE      = 1 << 0,
  BOBGUI_FONT_CHOOSER_LEVEL_SIZE       = 1 << 1,
  BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS = 1 << 2,
  BOBGUI_FONT_CHOOSER_LEVEL_FEATURES   = 1 << 3
} BobguiFontChooserLevel;

#define BOBGUI_TYPE_FONT_CHOOSER			(bobgui_font_chooser_get_type ())
#define BOBGUI_FONT_CHOOSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FONT_CHOOSER, BobguiFontChooser))
#define BOBGUI_IS_FONT_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FONT_CHOOSER))
#define BOBGUI_FONT_CHOOSER_GET_IFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_FONT_CHOOSER, BobguiFontChooserIface))

typedef struct _BobguiFontChooser      BobguiFontChooser; /* dummy */
typedef struct _BobguiFontChooserIface BobguiFontChooserIface;

struct _BobguiFontChooserIface
{
  GTypeInterface base_iface;

  /* Methods */
  PangoFontFamily * (* get_font_family)         (BobguiFontChooser  *fontchooser);
  PangoFontFace *   (* get_font_face)           (BobguiFontChooser  *fontchooser);
  int               (* get_font_size)           (BobguiFontChooser  *fontchooser);

  void              (* set_filter_func)         (BobguiFontChooser   *fontchooser,
                                                 BobguiFontFilterFunc filter,
                                                 gpointer          user_data,
                                                 GDestroyNotify    destroy);

  /* Signals */
  void (* font_activated) (BobguiFontChooser *chooser,
                           const char     *fontname);

  /* More methods */
  void              (* set_font_map)            (BobguiFontChooser   *fontchooser,
                                                 PangoFontMap     *fontmap);
  PangoFontMap *    (* get_font_map)            (BobguiFontChooser   *fontchooser);

  /*< private >*/
  /* Padding; remove in BOBGUI-next */
  gpointer padding[10];
};

GDK_AVAILABLE_IN_ALL
GType            bobgui_font_chooser_get_type                 (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
PangoFontFamily *bobgui_font_chooser_get_font_family          (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
PangoFontFace   *bobgui_font_chooser_get_font_face            (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
int              bobgui_font_chooser_get_font_size            (BobguiFontChooser   *fontchooser);

GDK_DEPRECATED_IN_4_10
PangoFontDescription *
                 bobgui_font_chooser_get_font_desc            (BobguiFontChooser             *fontchooser);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_font_desc            (BobguiFontChooser             *fontchooser,
                                                            const PangoFontDescription *font_desc);

GDK_DEPRECATED_IN_4_10
char *           bobgui_font_chooser_get_font                 (BobguiFontChooser   *fontchooser);

GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_font                 (BobguiFontChooser   *fontchooser,
                                                            const char       *fontname);
GDK_DEPRECATED_IN_4_10
char *           bobgui_font_chooser_get_preview_text         (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_preview_text         (BobguiFontChooser   *fontchooser,
                                                            const char       *text);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_font_chooser_get_show_preview_entry   (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_show_preview_entry   (BobguiFontChooser   *fontchooser,
                                                            gboolean          show_preview_entry);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_filter_func          (BobguiFontChooser   *fontchooser,
                                                            BobguiFontFilterFunc filter,
                                                            gpointer          user_data,
                                                            GDestroyNotify    destroy);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_font_map             (BobguiFontChooser   *fontchooser,
                                                            PangoFontMap     *fontmap);
GDK_DEPRECATED_IN_4_10
PangoFontMap *   bobgui_font_chooser_get_font_map             (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_level                (BobguiFontChooser   *fontchooser,
                                                            BobguiFontChooserLevel level);
GDK_DEPRECATED_IN_4_10
BobguiFontChooserLevel
                 bobgui_font_chooser_get_level                (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
char *           bobgui_font_chooser_get_font_features        (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
char *           bobgui_font_chooser_get_language             (BobguiFontChooser   *fontchooser);
GDK_DEPRECATED_IN_4_10
void             bobgui_font_chooser_set_language             (BobguiFontChooser   *fontchooser,
                                                            const char       *language);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFontChooser, g_object_unref)

G_END_DECLS

