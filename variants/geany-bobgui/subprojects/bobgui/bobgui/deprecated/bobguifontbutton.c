/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved.
 *
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
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
/*
 * Modified by the BOBGUI Team and others 2003.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguifontbutton.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguifontchooser.h"
#include "bobguifontchooserdialog.h"
#include "bobguifontchooserutils.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiseparator.h"
#include "bobguiwidgetprivate.h"

#include <string.h>
#include <stdio.h>


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiFontButton:
 *
 * The `BobguiFontButton` allows to open a font chooser dialog to change
 * the font.
 *
 * <picture>
 *   <source srcset="font-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiFontButton" src="font-button.png">
 * </picture>
 *
 * It is suitable widget for selecting a font in a preference dialog.
 *
 * # CSS nodes
 *
 * ```
 * fontbutton
 * ╰── button.font
 *     ╰── [content]
 * ```
 *
 * `BobguiFontButton` has a single CSS node with name fontbutton which
 * contains a button node with the .font style class.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */

typedef struct _BobguiFontButtonClass   BobguiFontButtonClass;

struct _BobguiFontButton
{
  BobguiWidget parent_instance;

  char          *title;
  char          *fontname;

  guint         use_font : 1;
  guint         use_size : 1;
  guint         show_preview_entry : 1;
  guint         modal    : 1;

  BobguiFontChooserLevel level;

  BobguiWidget     *button;
  BobguiWidget     *font_dialog;
  BobguiWidget     *font_label;
  BobguiWidget     *size_label;
  BobguiWidget     *font_size_box;

  int                   font_size;
  PangoFontDescription *font_desc;
  PangoFontFamily      *font_family;
  PangoFontFace        *font_face;
  PangoFontMap         *font_map;
  char                 *font_features;
  PangoLanguage        *language;
  char                 *preview_text;
  BobguiFontFilterFunc     font_filter;
  gpointer              font_filter_data;
  GDestroyNotify        font_filter_data_destroy;
};

struct _BobguiFontButtonClass
{
  BobguiWidgetClass parent_class;

  void (* font_set) (BobguiFontButton *gfp);
  void (* activate) (BobguiFontButton *self);
};

/* Signals */
enum
{
  FONT_SET,
  ACTIVATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_MODAL,
  PROP_USE_FONT,
  PROP_USE_SIZE
};

/* Prototypes */
static void bobgui_font_button_finalize               (GObject            *object);
static void bobgui_font_button_get_property           (GObject            *object,
                                                    guint               param_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void bobgui_font_button_set_property           (GObject            *object,
                                                    guint               param_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);

static void bobgui_font_button_clicked                 (BobguiButton         *button,
                                                     gpointer           user_data);

/* Dialog response functions */
static void response_cb                             (BobguiDialog         *dialog,
                                                     int                response_id,
                                                     gpointer           data);
static void dialog_destroy                          (BobguiWidget         *widget,
                                                     gpointer           data);

/* Auxiliary functions */
static void bobgui_font_button_label_use_font          (BobguiFontButton     *gfs);
static void bobgui_font_button_update_font_info        (BobguiFontButton     *gfs);

static void        bobgui_font_button_set_font_name (BobguiFontButton *button,
                                                  const char    *fontname);
static const char *bobgui_font_button_get_font_name (BobguiFontButton *button);
static void        bobgui_font_button_set_level     (BobguiFontButton       *font_button,
                                                  BobguiFontChooserLevel  level);
static void        bobgui_font_button_set_language  (BobguiFontButton *button,
                                                  const char    *language);

static guint font_button_signals[LAST_SIGNAL] = { 0 };

static PangoFontFamily * bobgui_font_button_font_chooser_get_font_family (BobguiFontChooser    *chooser);
static PangoFontFace *   bobgui_font_button_font_chooser_get_font_face   (BobguiFontChooser    *chooser);
static int               bobgui_font_button_font_chooser_get_font_size   (BobguiFontChooser    *chooser);
static void              bobgui_font_button_font_chooser_set_filter_func (BobguiFontChooser    *chooser,
                                                                       BobguiFontFilterFunc  filter_func,
                                                                       gpointer           filter_data,
                                                                       GDestroyNotify     data_destroy);
static void              bobgui_font_button_font_chooser_set_font_map    (BobguiFontChooser    *chooser,
                                                                       PangoFontMap      *font_map);
static PangoFontMap *    bobgui_font_button_font_chooser_get_font_map    (BobguiFontChooser    *chooser);


static void
bobgui_font_button_font_chooser_iface_init (BobguiFontChooserIface *iface)
{
  iface->get_font_family = bobgui_font_button_font_chooser_get_font_family;
  iface->get_font_face = bobgui_font_button_font_chooser_get_font_face;
  iface->get_font_size = bobgui_font_button_font_chooser_get_font_size;
  iface->set_filter_func = bobgui_font_button_font_chooser_set_filter_func;
  iface->set_font_map = bobgui_font_button_font_chooser_set_font_map;
  iface->get_font_map = bobgui_font_button_font_chooser_get_font_map;
}

G_DEFINE_TYPE_WITH_CODE (BobguiFontButton, bobgui_font_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_FONT_CHOOSER,
                                                bobgui_font_button_font_chooser_iface_init))

static void
clear_font_data (BobguiFontButton *font_button)
{
  g_clear_object (&font_button->font_family);
  g_clear_object (&font_button->font_face);
  g_clear_pointer (&font_button->font_desc, pango_font_description_free);
  g_clear_pointer (&font_button->fontname, g_free);
  g_clear_pointer (&font_button->font_features, g_free);
}

static void
clear_font_filter_data (BobguiFontButton *font_button)
{
  if (font_button->font_filter_data_destroy)
    font_button->font_filter_data_destroy (font_button->font_filter_data);
  font_button->font_filter = NULL;
  font_button->font_filter_data = NULL;
  font_button->font_filter_data_destroy = NULL;
}

static gboolean
font_description_style_equal (const PangoFontDescription *a,
                              const PangoFontDescription *b)
{
  return (pango_font_description_get_weight (a) == pango_font_description_get_weight (b) &&
          pango_font_description_get_style (a) == pango_font_description_get_style (b) &&
          pango_font_description_get_stretch (a) == pango_font_description_get_stretch (b) &&
          pango_font_description_get_variant (a) == pango_font_description_get_variant (b));
}

static void
bobgui_font_button_update_font_data (BobguiFontButton *font_button)
{
  PangoFontFamily **families;
  PangoFontFace **faces;
  int n_families, n_faces, i;
  const char *family;

  g_assert (font_button->font_desc != NULL);

  font_button->fontname = pango_font_description_to_string (font_button->font_desc);

  family = pango_font_description_get_family (font_button->font_desc);
  if (family == NULL)
    return;

  n_families = 0;
  families = NULL;
  pango_context_list_families (bobgui_widget_get_pango_context (BOBGUI_WIDGET (font_button)),
                               &families, &n_families);
  n_faces = 0;
  faces = NULL;
  for (i = 0; i < n_families; i++)
    {
      const char *name = pango_font_family_get_name (families[i]);

      if (!g_ascii_strcasecmp (name, family))
        {
          font_button->font_family = g_object_ref (families[i]);

          pango_font_family_list_faces (families[i], &faces, &n_faces);
          break;
        }
    }
  g_free (families);

  for (i = 0; i < n_faces; i++)
    {
      PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);

      if (font_description_style_equal (tmp_desc, font_button->font_desc))
        {
          font_button->font_face = g_object_ref (faces[i]);

          pango_font_description_free (tmp_desc);
          break;
        }
      else
        pango_font_description_free (tmp_desc);
    }

  g_free (faces);
}

static char *
bobgui_font_button_get_preview_text (BobguiFontButton *font_button)
{
  if (font_button->font_dialog)
    return bobgui_font_chooser_get_preview_text (BOBGUI_FONT_CHOOSER (font_button->font_dialog));

  return g_strdup (font_button->preview_text);
}

static void
bobgui_font_button_set_preview_text (BobguiFontButton *font_button,
                                  const char    *preview_text)
{
  if (font_button->font_dialog)
    {
      bobgui_font_chooser_set_preview_text (BOBGUI_FONT_CHOOSER (font_button->font_dialog),
                                         preview_text);
      return;
    }

  g_free (font_button->preview_text);
  font_button->preview_text = g_strdup (preview_text);
}


static gboolean
bobgui_font_button_get_show_preview_entry (BobguiFontButton *font_button)
{
  if (font_button->font_dialog)
    return bobgui_font_chooser_get_show_preview_entry (BOBGUI_FONT_CHOOSER (font_button->font_dialog));

  return font_button->show_preview_entry;
}

static void
bobgui_font_button_set_show_preview_entry (BobguiFontButton *font_button,
                                        gboolean       show)
{
  show = show != FALSE;

  if (font_button->show_preview_entry != show)
    {
      font_button->show_preview_entry = show;
      if (font_button->font_dialog)
        bobgui_font_chooser_set_show_preview_entry (BOBGUI_FONT_CHOOSER (font_button->font_dialog), show);
      g_object_notify (G_OBJECT (font_button), "show-preview-entry");
    }
}

static PangoFontFamily *
bobgui_font_button_font_chooser_get_font_family (BobguiFontChooser *chooser)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  return font_button->font_family;
}

static PangoFontFace *
bobgui_font_button_font_chooser_get_font_face (BobguiFontChooser *chooser)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  return font_button->font_face;
}

static int
bobgui_font_button_font_chooser_get_font_size (BobguiFontChooser *chooser)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  return font_button->font_size;
}

static void
bobgui_font_button_font_chooser_set_filter_func (BobguiFontChooser    *chooser,
                                              BobguiFontFilterFunc  filter_func,
                                              gpointer           filter_data,
                                              GDestroyNotify     data_destroy)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  if (font_button->font_dialog)
    {
      bobgui_font_chooser_set_filter_func (BOBGUI_FONT_CHOOSER (font_button->font_dialog),
                                        filter_func,
                                        filter_data,
                                        data_destroy);
      return;
    }

  clear_font_filter_data (font_button);
  font_button->font_filter = filter_func;
  font_button->font_filter_data = filter_data;
  font_button->font_filter_data_destroy = data_destroy;
}

static void
bobgui_font_button_take_font_desc (BobguiFontButton        *font_button,
                                PangoFontDescription *font_desc)
{
  GObject *object = G_OBJECT (font_button);

  if (font_button->font_desc && font_desc &&
      pango_font_description_equal (font_button->font_desc, font_desc))
    {
      pango_font_description_free (font_desc);
      return;
    }

  g_object_freeze_notify (object);

  clear_font_data (font_button);

  if (font_desc)
    font_button->font_desc = font_desc; /* adopted */
  else
    font_button->font_desc = pango_font_description_from_string (_("Sans 12"));

  if (pango_font_description_get_size_is_absolute (font_button->font_desc))
    font_button->font_size = pango_font_description_get_size (font_button->font_desc);
  else
    font_button->font_size = pango_font_description_get_size (font_button->font_desc) / PANGO_SCALE;

  bobgui_font_button_update_font_data (font_button);
  bobgui_font_button_update_font_info (font_button);

  if (font_button->font_dialog)
    bobgui_font_chooser_set_font_desc (BOBGUI_FONT_CHOOSER (font_button->font_dialog),
                                    font_button->font_desc);

  g_object_notify (G_OBJECT (font_button), "font");
  g_object_notify (G_OBJECT (font_button), "font-desc");

  g_object_thaw_notify (object);
}

static const PangoFontDescription *
bobgui_font_button_get_font_desc (BobguiFontButton *font_button)
{
  return font_button->font_desc;
}

static void
bobgui_font_button_font_chooser_set_font_map (BobguiFontChooser *chooser,
                                           PangoFontMap   *font_map)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  if (g_set_object (&font_button->font_map, font_map))
    {
      PangoContext *context;

      if (!font_map)
        font_map = pango_cairo_font_map_get_default ();

      context = bobgui_widget_get_pango_context (font_button->font_label);
      pango_context_set_font_map (context, font_map);
      if (font_button->font_dialog)
        bobgui_font_chooser_set_font_map (BOBGUI_FONT_CHOOSER (font_button->font_dialog), font_map);
    }
}

static PangoFontMap *
bobgui_font_button_font_chooser_get_font_map (BobguiFontChooser *chooser)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (chooser);

  return font_button->font_map;
}

static void
bobgui_font_button_font_chooser_notify (GObject    *object,
                                     GParamSpec *pspec,
                                     gpointer    user_data)
{
  /* We do not forward the notification of the "font" property to the dialog! */
  if (pspec->name == I_("preview-text") ||
      pspec->name == I_("show-preview-entry"))
    g_object_notify_by_pspec (user_data, pspec);
}

static void
bobgui_font_button_activate (BobguiFontButton *self)
{
  bobgui_widget_activate (self->button);
}

static void
bobgui_font_button_unrealize (BobguiWidget *widget)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (widget);

  g_clear_pointer ((BobguiWindow **) &font_button->font_dialog, bobgui_window_destroy);

  BOBGUI_WIDGET_CLASS (bobgui_font_button_parent_class)->unrealize (widget);
}

static void
bobgui_font_button_class_init (BobguiFontButtonClass *klass)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = (GObjectClass *) klass;
  widget_class = (BobguiWidgetClass *) klass;

  gobject_class->finalize = bobgui_font_button_finalize;
  gobject_class->set_property = bobgui_font_button_set_property;
  gobject_class->get_property = bobgui_font_button_get_property;

  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->unrealize = bobgui_font_button_unrealize;

  klass->font_set = NULL;
  klass->activate = bobgui_font_button_activate;

  _bobgui_font_chooser_install_properties (gobject_class);

  /**
   * BobguiFontButton:title:
   *
   * The title of the font chooser dialog.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        _("Pick a Font"),
                                                        BOBGUI_PARAM_READWRITE));

  /**
   * BobguiFontButton:use-font:
   *
   * Whether the buttons label will be drawn in the selected font.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_FONT,
                                   g_param_spec_boolean ("use-font", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontButton:use-size:
   *
   * Whether the buttons label will use the selected font size.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_SIZE,
                                   g_param_spec_boolean ("use-size", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontButton:modal:
   *
   * Whether the font chooser dialog should be modal.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MODAL,
                                   g_param_spec_boolean ("modal", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFontButton::font-set:
   * @widget: the object which received the signal
   *
   * Emitted when the user selects a font.
   *
   * When handling this signal, use [method@Bobgui.FontChooser.get_font]
   * to find out which font was just selected.
   *
   * Note that this signal is only emitted when the user changes the font.
   * If you need to react to programmatic font changes as well, use
   * the notify::font signal.
   */
  font_button_signals[FONT_SET] = g_signal_new (I_("font-set"),
                                                G_TYPE_FROM_CLASS (gobject_class),
                                                G_SIGNAL_RUN_FIRST,
                                                G_STRUCT_OFFSET (BobguiFontButtonClass, font_set),
                                                NULL, NULL,
                                                NULL,
                                                G_TYPE_NONE, 0);

  /**
   * BobguiFontButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the font button is activated.
   *
   * The `::activate` signal on `BobguiFontButton` is an action signal and
   * emitting it causes the button to present its dialog.
   *
   * Since: 4.4
   */
  font_button_signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (gobject_class),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiFontButtonClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, font_button_signals[ACTIVATE]);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("fontbutton"));
}

static void
bobgui_font_button_init (BobguiFontButton *font_button)
{
  BobguiWidget *box;

  font_button->button = bobgui_button_new ();
  g_signal_connect (font_button->button, "clicked", G_CALLBACK (bobgui_font_button_clicked), font_button);
  g_object_bind_property (font_button, "focus-on-click",
                          font_button->button, "focus-on-click",
                          0);
  font_button->font_label = bobgui_label_new (_("Font"));
  bobgui_widget_set_hexpand (font_button->font_label, TRUE);
  font_button->size_label = bobgui_label_new ("14");
  font_button->font_size_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (box), font_button->font_label);

  bobgui_box_append (BOBGUI_BOX (font_button->font_size_box), bobgui_separator_new (BOBGUI_ORIENTATION_VERTICAL));
  bobgui_box_append (BOBGUI_BOX (font_button->font_size_box), font_button->size_label);
  bobgui_box_append (BOBGUI_BOX (box), font_button->font_size_box);

  bobgui_button_set_child (BOBGUI_BUTTON (font_button->button), box);
  bobgui_widget_set_parent (font_button->button, BOBGUI_WIDGET (font_button));

  /* Initialize fields */
  font_button->modal = TRUE;
  font_button->use_font = FALSE;
  font_button->use_size = FALSE;
  font_button->show_preview_entry = TRUE;
  font_button->font_dialog = NULL;
  font_button->font_family = NULL;
  font_button->font_face = NULL;
  font_button->font_size = -1;
  font_button->title = g_strdup (_("Pick a Font"));
  font_button->level = BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                       BOBGUI_FONT_CHOOSER_LEVEL_STYLE |
                       BOBGUI_FONT_CHOOSER_LEVEL_SIZE;
  font_button->language = pango_language_get_default ();

  bobgui_font_button_take_font_desc (font_button, NULL);

  bobgui_widget_add_css_class (font_button->button, "font");
}

static void
bobgui_font_button_finalize (GObject *object)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (object);

  g_free (font_button->title);

  clear_font_data (font_button);
  clear_font_filter_data (font_button);

  g_free (font_button->preview_text);

  bobgui_widget_unparent (font_button->button);

  G_OBJECT_CLASS (bobgui_font_button_parent_class)->finalize (object);
}

static void
bobgui_font_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (object);

  switch (param_id)
    {
    case BOBGUI_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      bobgui_font_button_set_preview_text (font_button, g_value_get_string (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      bobgui_font_button_set_show_preview_entry (font_button, g_value_get_boolean (value));
      break;
    case PROP_TITLE:
      bobgui_font_button_set_title (font_button, g_value_get_string (value));
      break;
    case PROP_MODAL:
      bobgui_font_button_set_modal (font_button, g_value_get_boolean (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_DESC:
      bobgui_font_button_take_font_desc (font_button, g_value_dup_boxed (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LANGUAGE:
      bobgui_font_button_set_language (font_button, g_value_get_string (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LEVEL:
      bobgui_font_button_set_level (font_button, g_value_get_flags (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT:
      bobgui_font_button_set_font_name (font_button, g_value_get_string (value));
      break;
    case PROP_USE_FONT:
      bobgui_font_button_set_use_font (font_button, g_value_get_boolean (value));
      break;
    case PROP_USE_SIZE:
      bobgui_font_button_set_use_size (font_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
bobgui_font_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (object);

  switch (param_id)
    {
    case BOBGUI_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      g_value_set_string (value, bobgui_font_button_get_preview_text (font_button));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      g_value_set_boolean (value, bobgui_font_button_get_show_preview_entry (font_button));
      break;
    case PROP_TITLE:
      g_value_set_string (value, bobgui_font_button_get_title (font_button));
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, bobgui_font_button_get_modal (font_button));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_DESC:
      g_value_set_boxed (value, bobgui_font_button_get_font_desc (font_button));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_FEATURES:
      g_value_set_string (value, font_button->font_features);
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LANGUAGE:
      g_value_set_string (value, pango_language_to_string (font_button->language));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LEVEL:
      g_value_set_flags (value, font_button->level);
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT:
      g_value_set_string (value, bobgui_font_button_get_font_name (font_button));
      break;
    case PROP_USE_FONT:
      g_value_set_boolean (value, bobgui_font_button_get_use_font (font_button));
      break;
    case PROP_USE_SIZE:
      g_value_set_boolean (value, bobgui_font_button_get_use_size (font_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


/**
 * bobgui_font_button_new:
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
BobguiWidget *
bobgui_font_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_FONT_BUTTON, NULL);
}

/**
 * bobgui_font_button_new_with_font:
 * @fontname: Name of font to display in font chooser dialog
 *
 * Creates a new font picker widget showing the given font.
 *
 * Returns: a new font picker widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
BobguiWidget *
bobgui_font_button_new_with_font (const char *fontname)
{
  return g_object_new (BOBGUI_TYPE_FONT_BUTTON, "font", fontname, NULL);
}

/**
 * bobgui_font_button_set_title:
 * @font_button: a `BobguiFontButton`
 * @title: a string containing the font chooser dialog title
 *
 * Sets the title for the font chooser dialog.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
void
bobgui_font_button_set_title (BobguiFontButton *font_button,
                           const char    *title)
{
  char *old_title;
  g_return_if_fail (BOBGUI_IS_FONT_BUTTON (font_button));

  old_title = font_button->title;
  font_button->title = g_strdup (title);
  g_free (old_title);

  if (font_button->font_dialog)
    bobgui_window_set_title (BOBGUI_WINDOW (font_button->font_dialog), font_button->title);

  g_object_notify (G_OBJECT (font_button), "title");
}

/**
 * bobgui_font_button_get_title:
 * @font_button: a `BobguiFontButton`
 *
 * Retrieves the title of the font chooser dialog.
 *
 * Returns: an internal copy of the title string
 *   which must not be freed.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
const char *
bobgui_font_button_get_title (BobguiFontButton *font_button)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_BUTTON (font_button), NULL);

  return font_button->title;
}

/**
 * bobgui_font_button_set_modal:
 * @font_button: a `BobguiFontButton`
 * @modal: %TRUE to make the dialog modal
 *
 * Sets whether the dialog should be modal.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
void
bobgui_font_button_set_modal (BobguiFontButton *font_button,
                           gboolean       modal)
{
  g_return_if_fail (BOBGUI_IS_FONT_BUTTON (font_button));

  if (font_button->modal == modal)
    return;

  font_button->modal = modal;

  if (font_button->font_dialog)
    bobgui_window_set_modal (BOBGUI_WINDOW (font_button->font_dialog), font_button->modal);

  g_object_notify (G_OBJECT (font_button), "modal");
}

/**
 * bobgui_font_button_get_modal:
 * @font_button: a `BobguiFontButton`
 *
 * Gets whether the dialog is modal.
 *
 * Returns: %TRUE if the dialog is modal
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
gboolean
bobgui_font_button_get_modal (BobguiFontButton *font_button)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->modal;
}

/**
 * bobgui_font_button_get_use_font:
 * @font_button: a `BobguiFontButton`
 *
 * Returns whether the selected font is used in the label.
 *
 * Returns: whether the selected font is used in the label.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
gboolean
bobgui_font_button_get_use_font (BobguiFontButton *font_button)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->use_font;
}

/**
 * bobgui_font_button_set_use_font:
 * @font_button: a `BobguiFontButton`
 * @use_font: If %TRUE, font name will be written using font chosen.
 *
 * If @use_font is %TRUE, the font name will be written
 * using the selected font.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
void
bobgui_font_button_set_use_font (BobguiFontButton *font_button,
			      gboolean       use_font)
{
  g_return_if_fail (BOBGUI_IS_FONT_BUTTON (font_button));

  use_font = (use_font != FALSE);

  if (font_button->use_font != use_font)
    {
      font_button->use_font = use_font;

      bobgui_font_button_label_use_font (font_button);

      g_object_notify (G_OBJECT (font_button), "use-font");
    }
}


/**
 * bobgui_font_button_get_use_size:
 * @font_button: a `BobguiFontButton`
 *
 * Returns whether the selected size is used in the label.
 *
 * Returns: whether the selected size is used in the label.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
gboolean
bobgui_font_button_get_use_size (BobguiFontButton *font_button)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->use_size;
}

/**
 * bobgui_font_button_set_use_size:
 * @font_button: a `BobguiFontButton`
 * @use_size: If %TRUE, font name will be written using the
 *   selected size.
 *
 * If @use_size is %TRUE, the font name will be written using
 * the selected size.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialogButton] instead
 */
void
bobgui_font_button_set_use_size (BobguiFontButton *font_button,
                              gboolean       use_size)
{
  g_return_if_fail (BOBGUI_IS_FONT_BUTTON (font_button));

  use_size = (use_size != FALSE);
  if (font_button->use_size != use_size)
    {
      font_button->use_size = use_size;

      bobgui_font_button_label_use_font (font_button);

      g_object_notify (G_OBJECT (font_button), "use-size");
    }
}

static const char *
bobgui_font_button_get_font_name (BobguiFontButton *font_button)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_BUTTON (font_button), NULL);

  return font_button->fontname;
}

static void
bobgui_font_button_set_font_name (BobguiFontButton *font_button,
                               const char     *fontname)
{
  PangoFontDescription *font_desc;

  font_desc = pango_font_description_from_string (fontname);
  bobgui_font_button_take_font_desc (font_button, font_desc);
}

static void
bobgui_font_button_clicked (BobguiButton *button,
                         gpointer   user_data)
{
  BobguiFontChooser *font_dialog;
  BobguiFontButton  *font_button = user_data;

  if (!font_button->font_dialog)
    {
      BobguiWidget *parent;

      parent = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (font_button)));

      font_button->font_dialog = bobgui_font_chooser_dialog_new (font_button->title, NULL);
      bobgui_window_set_hide_on_close (BOBGUI_WINDOW (font_button->font_dialog), TRUE);
      bobgui_window_set_modal (BOBGUI_WINDOW (font_button->font_dialog), font_button->modal);
      bobgui_window_set_display (BOBGUI_WINDOW (font_button->font_dialog), bobgui_widget_get_display (BOBGUI_WIDGET (button)));

      font_dialog = BOBGUI_FONT_CHOOSER (font_button->font_dialog);

      if (font_button->font_map)
        bobgui_font_chooser_set_font_map (font_dialog, font_button->font_map);

      bobgui_font_chooser_set_show_preview_entry (font_dialog, font_button->show_preview_entry);
      bobgui_font_chooser_set_level (BOBGUI_FONT_CHOOSER (font_dialog), font_button->level);
      bobgui_font_chooser_set_language (BOBGUI_FONT_CHOOSER (font_dialog), pango_language_to_string (font_button->language));

      if (font_button->preview_text)
        {
          bobgui_font_chooser_set_preview_text (font_dialog, font_button->preview_text);
          g_free (font_button->preview_text);
          font_button->preview_text = NULL;
        }

      if (font_button->font_filter)
        {
          bobgui_font_chooser_set_filter_func (font_dialog,
                                            font_button->font_filter,
                                            font_button->font_filter_data,
                                            font_button->font_filter_data_destroy);
          font_button->font_filter = NULL;
          font_button->font_filter_data = NULL;
          font_button->font_filter_data_destroy = NULL;
        }

      if (BOBGUI_IS_WINDOW (parent))
        {
          if (BOBGUI_WINDOW (parent) != bobgui_window_get_transient_for (BOBGUI_WINDOW (font_dialog)))
            bobgui_window_set_transient_for (BOBGUI_WINDOW (font_dialog), BOBGUI_WINDOW (parent));

          if (bobgui_window_get_modal (BOBGUI_WINDOW (parent)))
            bobgui_window_set_modal (BOBGUI_WINDOW (font_dialog), TRUE);
        }

      g_signal_connect (font_dialog, "notify",
                        G_CALLBACK (bobgui_font_button_font_chooser_notify), button);

      g_signal_connect (font_dialog, "response",
                        G_CALLBACK (response_cb), font_button);

      g_signal_connect (font_dialog, "destroy",
                        G_CALLBACK (dialog_destroy), font_button);
    }

  if (!bobgui_widget_get_visible (font_button->font_dialog))
    {
      font_dialog = BOBGUI_FONT_CHOOSER (font_button->font_dialog);
      bobgui_font_chooser_set_font_desc (font_dialog, font_button->font_desc);
    }

  bobgui_window_present (BOBGUI_WINDOW (font_button->font_dialog));
}


static void
response_cb (BobguiDialog *dialog,
             int        response_id,
             gpointer   data)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (data);
  BobguiFontChooser *font_chooser;
  GObject *object;

  bobgui_widget_hide (font_button->font_dialog);

  if (response_id != BOBGUI_RESPONSE_OK)
    return;

  font_chooser = BOBGUI_FONT_CHOOSER (font_button->font_dialog);
  object = G_OBJECT (font_chooser);

  g_object_freeze_notify (object);

  clear_font_data (font_button);

  font_button->font_desc = bobgui_font_chooser_get_font_desc (font_chooser);
  if (font_button->font_desc)
    font_button->fontname = pango_font_description_to_string (font_button->font_desc);
  font_button->font_family = bobgui_font_chooser_get_font_family (font_chooser);
  if (font_button->font_family)
    g_object_ref (font_button->font_family);
  font_button->font_face = bobgui_font_chooser_get_font_face (font_chooser);
  if (font_button->font_face)
    g_object_ref (font_button->font_face);
  font_button->font_size = bobgui_font_chooser_get_font_size (font_chooser);
  g_free (font_button->font_features);
  font_button->font_features = bobgui_font_chooser_get_font_features (font_chooser);
  font_button->language = pango_language_from_string (bobgui_font_chooser_get_language (font_chooser));

  /* Set label font */
  bobgui_font_button_update_font_info (font_button);

  g_object_notify (G_OBJECT (font_button), "font");
  g_object_notify (G_OBJECT (font_button), "font-desc");
  g_object_notify (G_OBJECT (font_button), "font-features");

  g_object_thaw_notify (object);

  /* Emit font_set signal */
  g_signal_emit (font_button, font_button_signals[FONT_SET], 0);
}

static void
dialog_destroy (BobguiWidget *widget,
                gpointer   data)
{
  BobguiFontButton *font_button = BOBGUI_FONT_BUTTON (data);

  /* Dialog will get destroyed so reference is not valid now */
  font_button->font_dialog = NULL;
}

static void
bobgui_font_button_label_use_font (BobguiFontButton *font_button)
{
  if (!font_button->use_font)
    bobgui_label_set_attributes (BOBGUI_LABEL (font_button->font_label), NULL);
  else
    {
      PangoFontDescription *desc;
      PangoAttrList *attrs;

      desc = pango_font_description_copy (font_button->font_desc);

      if (!font_button->use_size)
        pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);

      attrs = pango_attr_list_new ();

      /* Prevent font fallback */
      pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));

      /* Force current font and features */
      pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
      if (font_button->font_features)
        pango_attr_list_insert (attrs, pango_attr_font_features_new (font_button->font_features));
      if (font_button->language)
        pango_attr_list_insert (attrs, pango_attr_language_new (font_button->language));

      bobgui_label_set_attributes (BOBGUI_LABEL (font_button->font_label), attrs);

      pango_attr_list_unref (attrs);
      pango_font_description_free (desc);
    }
}

static void
bobgui_font_button_update_font_info (BobguiFontButton *font_button)
{
  const char *fam_name;
  const char *face_name;
  char *family_style;

  if (font_button->font_family)
    fam_name = pango_font_family_get_name (font_button->font_family);
  else
    fam_name = C_("font", "None");
  if (font_button->font_face)
    face_name = pango_font_face_get_face_name (font_button->font_face);
  else
    face_name = "";

  if ((font_button->level & BOBGUI_FONT_CHOOSER_LEVEL_STYLE) != 0)
    family_style = g_strconcat (fam_name, " ", face_name, NULL);
  else
    family_style = g_strdup (fam_name);

  bobgui_label_set_text (BOBGUI_LABEL (font_button->font_label), family_style);
  g_free (family_style);

  if ((font_button->level & BOBGUI_FONT_CHOOSER_LEVEL_SIZE) != 0)
    {
      /* mirror Pango, which doesn't translate this either */
      char *size = g_strdup_printf ("%2.4g%s",
                                     pango_font_description_get_size (font_button->font_desc) / (double)PANGO_SCALE,
                                     pango_font_description_get_size_is_absolute (font_button->font_desc) ? "px" : "");

      bobgui_label_set_text (BOBGUI_LABEL (font_button->size_label), size);

      g_free (size);

      bobgui_widget_show (font_button->font_size_box);
    }
  else
    bobgui_widget_hide (font_button->font_size_box);


  bobgui_font_button_label_use_font (font_button);
}

static void
bobgui_font_button_set_level (BobguiFontButton       *font_button,
                           BobguiFontChooserLevel  level)
{
  if (font_button->level == level)
    return;

  font_button->level = level;

  if (font_button->font_dialog)
    bobgui_font_chooser_set_level (BOBGUI_FONT_CHOOSER (font_button->font_dialog), level);

  bobgui_font_button_update_font_info (font_button);

  g_object_notify (G_OBJECT (font_button), "level");
}

static void
bobgui_font_button_set_language (BobguiFontButton *font_button,
                              const char    *language)
{
  font_button->language = pango_language_from_string (language);

  if (font_button->font_dialog)
    bobgui_font_chooser_set_language (BOBGUI_FONT_CHOOSER (font_button->font_dialog), language);

  g_object_notify (G_OBJECT (font_button), "language");
}
