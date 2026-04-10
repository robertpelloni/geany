/* BOBGUI - The Bobgui Framework
 * bobguifontchooser.c - Abstract interface for font file selectors GUIs
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

#include "config.h"

#include "bobguifontchooser.h"
#include "bobguifontchooserprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiFontChooser:
 *
 * `BobguiFontChooser` is an interface that can be implemented by widgets
 * for choosing fonts.
 *
 * In BOBGUI, the main objects that implement this interface are
 * [class@Bobgui.FontChooserWidget], [class@Bobgui.FontChooserDialog] and
 * [class@Bobgui.FontButton].
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */

enum
{
  SIGNAL_FONT_ACTIVATED,
  LAST_SIGNAL
};

static guint chooser_signals[LAST_SIGNAL];

typedef BobguiFontChooserIface BobguiFontChooserInterface;
G_DEFINE_INTERFACE (BobguiFontChooser, bobgui_font_chooser, G_TYPE_OBJECT);

static void
bobgui_font_chooser_default_init (BobguiFontChooserInterface *iface)
{
  /**
   * BobguiFontChooser:font:
   *
   * The font description as a string, e.g. "Sans Italic 12".
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_string ("font", NULL, NULL,
                           BOBGUI_FONT_CHOOSER_DEFAULT_FONT_NAME,
                           BOBGUI_PARAM_READWRITE));

  /**
   * BobguiFontChooser:font-desc:
   *
   * The font description as a `PangoFontDescription`.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_boxed ("font-desc", NULL, NULL,
                          PANGO_TYPE_FONT_DESCRIPTION,
                          BOBGUI_PARAM_READWRITE));

  /**
   * BobguiFontChooser:preview-text:
   *
   * The string with which to preview the font.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_string ("preview-text", NULL, NULL,
                          pango_language_get_sample_string (NULL),
                          BOBGUI_PARAM_READWRITE));

  /**
   * BobguiFontChooser:show-preview-entry:
   *
   * Whether to show an entry to change the preview text.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_boolean ("show-preview-entry", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontChooser:level:
   *
   * The level of granularity to offer for selecting fonts.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_flags ("level", NULL, NULL,
                          BOBGUI_TYPE_FONT_CHOOSER_LEVEL,
                          BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                          BOBGUI_FONT_CHOOSER_LEVEL_STYLE |
                          BOBGUI_FONT_CHOOSER_LEVEL_SIZE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontChooser:font-features:
   *
   * The selected font features.
   *
   * The format of the string is compatible with
   * CSS and with Pango attributes.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_string ("font-features", NULL, NULL,
                          "",
                          BOBGUI_PARAM_READABLE));

  /**
   * BobguiFontChooser:language:
   *
   * The language for which the font features were selected.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  g_object_interface_install_property
     (iface,
      g_param_spec_string ("language", NULL, NULL,
                          "",
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontChooser::font-activated:
   * @self: the object which received the signal
   * @fontname: the font name
   *
   * Emitted when a font is activated.
   *
   * This usually happens when the user double clicks an item,
   * or an item is selected and the user presses one of the keys
   * Space, Shift+Space, Return or Enter.
   *
   * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton] instead
   */
  chooser_signals[SIGNAL_FONT_ACTIVATED] =
    g_signal_new (I_("font-activated"),
                  BOBGUI_TYPE_FONT_CHOOSER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiFontChooserIface, font_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);
}

/**
 * bobgui_font_chooser_get_font_family:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the `PangoFontFamily` representing the selected font family.
 *
 * Font families are a collection of font faces.
 *
 * If the selected font is not installed, returns %NULL.
 *
 * Returns: (nullable) (transfer none): A `PangoFontFamily` representing the
 *   selected font family
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
PangoFontFamily *
bobgui_font_chooser_get_font_family (BobguiFontChooser *fontchooser)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  return BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->get_font_family (fontchooser);
}

/**
 * bobgui_font_chooser_get_font_face:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the `PangoFontFace` representing the selected font group
 * details (i.e. family, slant, weight, width, etc).
 *
 * If the selected font is not installed, returns %NULL.
 *
 * Returns: (nullable) (transfer none): A `PangoFontFace` representing the
 *   selected font group details
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
PangoFontFace *
bobgui_font_chooser_get_font_face (BobguiFontChooser *fontchooser)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  return BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->get_font_face (fontchooser);
}

/**
 * bobgui_font_chooser_get_font_size:
 * @fontchooser: a `BobguiFontChooser`
 *
 * The selected font size.
 *
 * Returns: A n integer representing the selected font size,
 *   or -1 if no font size is selected.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
int
bobgui_font_chooser_get_font_size (BobguiFontChooser *fontchooser)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), -1);

  return BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->get_font_size (fontchooser);
}

/**
 * bobgui_font_chooser_get_font:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the currently-selected font name.
 *
 * Note that this can be a different string than what you set with
 * [method@Bobgui.FontChooser.set_font], as the font chooser widget may
 * normalize font names and thus return a string with a different
 * structure. For example, “Helvetica Italic Bold 12” could be
 * normalized to “Helvetica Bold Italic 12”.
 *
 * Use [method@Pango.FontDescription.equal] if you want to compare two
 * font descriptions.
 *
 * Returns: (nullable) (transfer full): A string with the name
 *   of the current font
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
char *
bobgui_font_chooser_get_font (BobguiFontChooser *fontchooser)
{
  char *fontname;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  g_object_get (fontchooser, "font", &fontname, NULL);


  return fontname;
}

/**
 * bobgui_font_chooser_set_font:
 * @fontchooser: a `BobguiFontChooser`
 * @fontname: a font name like “Helvetica 12” or “Times Bold 18”
 *
 * Sets the currently-selected font.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_font (BobguiFontChooser *fontchooser,
                           const char     *fontname)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));
  g_return_if_fail (fontname != NULL);

  g_object_set (fontchooser, "font", fontname, NULL);
}

/**
 * bobgui_font_chooser_get_font_desc:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the currently-selected font.
 *
 * Note that this can be a different string than what you set with
 * [method@Bobgui.FontChooser.set_font], as the font chooser widget may
 * normalize font names and thus return a string with a different
 * structure. For example, “Helvetica Italic Bold 12” could be
 * normalized to “Helvetica Bold Italic 12”.
 *
 * Use [method@Pango.FontDescription.equal] if you want to compare two
 * font descriptions.
 *
 * Returns: (nullable) (transfer full): A `PangoFontDescription` for the
 *   current font
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
PangoFontDescription *
bobgui_font_chooser_get_font_desc (BobguiFontChooser *fontchooser)
{
  PangoFontDescription *font_desc;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  g_object_get (fontchooser, "font-desc", &font_desc, NULL);

  return font_desc;
}

/**
 * bobgui_font_chooser_set_font_desc:
 * @fontchooser: a `BobguiFontChooser`
 * @font_desc: a `PangoFontDescription`
 *
 * Sets the currently-selected font from @font_desc.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_font_desc (BobguiFontChooser             *fontchooser,
                                const PangoFontDescription *font_desc)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));
  g_return_if_fail (font_desc != NULL);

  g_object_set (fontchooser, "font-desc", font_desc, NULL);
}

/**
 * bobgui_font_chooser_get_preview_text:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the text displayed in the preview area.
 *
 * Returns: (transfer full): the text displayed in the preview area
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
char *
bobgui_font_chooser_get_preview_text (BobguiFontChooser *fontchooser)
{
  char *text;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  g_object_get (fontchooser, "preview-text", &text, NULL);

  return text;
}

/**
 * bobgui_font_chooser_set_preview_text:
 * @fontchooser: a `BobguiFontChooser`
 * @text: (transfer none): the text to display in the preview area
 *
 * Sets the text displayed in the preview area.
 *
 * The @text is used to show how the selected font looks.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_preview_text (BobguiFontChooser *fontchooser,
                                   const char     *text)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));
  g_return_if_fail (text != NULL);

  g_object_set (fontchooser, "preview-text", text, NULL);
}

/**
 * bobgui_font_chooser_get_show_preview_entry:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Returns whether the preview entry is shown or not.
 *
 * Returns: %TRUE if the preview entry is shown or %FALSE if it is hidden.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
gboolean
bobgui_font_chooser_get_show_preview_entry (BobguiFontChooser *fontchooser)
{
  gboolean show;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), FALSE);

  g_object_get (fontchooser, "show-preview-entry", &show, NULL);

  return show;
}

/**
 * bobgui_font_chooser_set_show_preview_entry:
 * @fontchooser: a `BobguiFontChooser`
 * @show_preview_entry: whether to show the editable preview entry or not
 *
 * Shows or hides the editable preview entry.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_show_preview_entry (BobguiFontChooser *fontchooser,
                                         gboolean        show_preview_entry)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));

  show_preview_entry = show_preview_entry != FALSE;
  g_object_set (fontchooser, "show-preview-entry", show_preview_entry, NULL);
}

/**
 * bobgui_font_chooser_set_filter_func:
 * @fontchooser: a `BobguiFontChooser`
 * @filter: (nullable) (scope notified) (closure user_data) (destroy destroy): a `BobguiFontFilterFunc`
 * @user_data: data to pass to @filter
 * @destroy: function to call to free @data when it is no longer needed
 *
 * Adds a filter function that decides which fonts to display
 * in the font chooser.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_filter_func (BobguiFontChooser   *fontchooser,
                                  BobguiFontFilterFunc filter,
                                  gpointer          user_data,
                                  GDestroyNotify    destroy)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));

  BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->set_filter_func (fontchooser,
                                                             filter,
                                                             user_data,
                                                             destroy);
}

void
_bobgui_font_chooser_font_activated (BobguiFontChooser *chooser,
                                  const char     *fontname)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (chooser));

  g_signal_emit (chooser, chooser_signals[SIGNAL_FONT_ACTIVATED], 0, fontname);
}

/**
 * bobgui_font_chooser_set_font_map:
 * @fontchooser: a `BobguiFontChooser`
 * @fontmap: (nullable): a `PangoFontMap`
 *
 * Sets a custom font map to use for this font chooser widget.
 *
 * A custom font map can be used to present application-specific
 * fonts instead of or in addition to the normal system fonts.
 *
 * ```c
 * FcConfig *config;
 * PangoFontMap *fontmap;
 *
 * config = FcInitLoadConfigAndFonts ();
 * FcConfigAppFontAddFile (config, my_app_font_file);
 *
 * fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
 * pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (fontmap), config);
 *
 * bobgui_font_chooser_set_font_map (font_chooser, fontmap);
 * ```
 *
 * Note that other BOBGUI widgets will only be able to use the
 * application-specific font if it is present in the font map they use:
 *
 * ```c
 * context = bobgui_widget_get_pango_context (label);
 * pango_context_set_font_map (context, fontmap);
 * ```
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_font_map (BobguiFontChooser *fontchooser,
                               PangoFontMap   *fontmap)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));
  g_return_if_fail (fontmap == NULL || PANGO_IS_FONT_MAP (fontmap));

  if (BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->set_font_map)
    BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->set_font_map (fontchooser, fontmap);
}

/**
 * bobgui_font_chooser_get_font_map:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the custom font map of this font chooser widget,
 * or %NULL if it does not have one.
 *
 * Returns: (nullable) (transfer full): a `PangoFontMap`
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
PangoFontMap *
bobgui_font_chooser_get_font_map (BobguiFontChooser *fontchooser)
{
  PangoFontMap *fontmap = NULL;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  if (BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->get_font_map)
    fontmap = BOBGUI_FONT_CHOOSER_GET_IFACE (fontchooser)->get_font_map (fontchooser);

  return fontmap;
}

/**
 * bobgui_font_chooser_set_level:
 * @fontchooser: a `BobguiFontChooser`
 * @level: the desired level of granularity
 *
 * Sets the desired level of granularity for selecting fonts.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_level (BobguiFontChooser      *fontchooser,
                            BobguiFontChooserLevel  level)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));

  g_object_set (fontchooser, "level", level, NULL);
}

/**
 * bobgui_font_chooser_get_level:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Returns the current level of granularity for selecting fonts.
 *
 * Returns: the current granularity level
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
BobguiFontChooserLevel
bobgui_font_chooser_get_level (BobguiFontChooser *fontchooser)
{
  BobguiFontChooserLevel level;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), 0);

  g_object_get (fontchooser, "level", &level, NULL);

  return level;
}

/**
 * bobgui_font_chooser_get_font_features:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the currently-selected font features.
 *
 * The format of the returned string is compatible with the
 * [CSS font-feature-settings property](https://www.w3.org/TR/css-fonts-4/#font-rend-desc).
 * It can be passed to [func@Pango.AttrFontFeatures.new].
 *
 * Returns: the currently selected font features
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
char *
bobgui_font_chooser_get_font_features (BobguiFontChooser *fontchooser)
{
  char *text;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  g_object_get (fontchooser, "font-features", &text, NULL);

  return text;
}

/**
 * bobgui_font_chooser_get_language:
 * @fontchooser: a `BobguiFontChooser`
 *
 * Gets the language that is used for font features.
 *
 * Returns: the currently selected language
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
char *
bobgui_font_chooser_get_language (BobguiFontChooser *fontchooser)
{
  char *text;

  g_return_val_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser), NULL);

  g_object_get (fontchooser, "language", &text, NULL);

  return text;
}

/**
 * bobgui_font_chooser_set_language:
 * @fontchooser: a `BobguiFontChooser`
 * @language: a language
 *
 * Sets the language to use for font features.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] and [class@Bobgui.FontDialogButton]
 * instead
 */
void
bobgui_font_chooser_set_language (BobguiFontChooser *fontchooser,
                               const char     *language)
{
  g_return_if_fail (BOBGUI_IS_FONT_CHOOSER (fontchooser));

  g_object_set (fontchooser, "language", language, NULL);
}
