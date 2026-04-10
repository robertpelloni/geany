/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguifontdialogbutton.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguiseparator.h"
#include "bobguibutton.h"
#include "bobguilabel.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguitypebuiltins.h"

static void     activated      (BobguiFontDialogButton *self);
static void     button_clicked (BobguiFontDialogButton *self);
static void     update_button_sensitivity
                               (BobguiFontDialogButton *self);

/**
 * BobguiFontDialogButton:
 *
 * Opens a font chooser dialog to select a font.
 *
 * <picture>
 *   <source srcset="font-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiFontDialogButton" src="font-button.png">
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
 * `BobguiFontDialogButton` has a single CSS node with name fontbutton which
 * contains a button node with the .font style class.
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiFontDialogButton
{
  BobguiWidget parent_instance;

  BobguiWidget *button;
  BobguiWidget *font_label;
  BobguiWidget *size_label;
  BobguiWidget *font_size_box;

  BobguiFontLevel level;

  guint use_font : 1;
  guint use_size : 1;

  BobguiFontDialog *dialog;
  GCancellable *cancellable;
  PangoFontDescription *font_desc;
  char *font_features;
  PangoLanguage *language;

  PangoFontFamily *font_family;
  PangoFontFace *font_face;
};

/* Properties */
enum
{
  PROP_DIALOG = 1,
  PROP_LEVEL,
  PROP_FONT_DESC,
  PROP_FONT_FEATURES,
  PROP_LANGUAGE,
  PROP_USE_FONT,
  PROP_USE_SIZE,
  NUM_PROPERTIES
};

/* Signals */
enum
{
  SIGNAL_ACTIVATE = 1,
  NUM_SIGNALS
};

static GParamSpec *properties[NUM_PROPERTIES];

static unsigned int font_dialog_button_signals[NUM_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiFontDialogButton, bobgui_font_dialog_button, BOBGUI_TYPE_WIDGET)

static void
bobgui_font_dialog_button_init (BobguiFontDialogButton *self)
{
  BobguiWidget *box;
  PangoFontDescription *font_desc;

  g_signal_connect_swapped (self, "activate", G_CALLBACK (activated), self);

  self->button = bobgui_button_new ();
  g_signal_connect_swapped (self->button, "clicked", G_CALLBACK (button_clicked), self);
  self->font_label = bobgui_label_new (_("Font"));
  bobgui_widget_set_hexpand (self->font_label, TRUE);
  self->size_label = bobgui_label_new ("14");
  self->font_size_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (box), self->font_label);

  bobgui_box_append (BOBGUI_BOX (self->font_size_box), bobgui_separator_new (BOBGUI_ORIENTATION_VERTICAL));
  bobgui_box_append (BOBGUI_BOX (self->font_size_box), self->size_label);
  bobgui_box_append (BOBGUI_BOX (box), self->font_size_box);

  bobgui_button_set_child (BOBGUI_BUTTON (self->button), box);
  bobgui_widget_set_parent (self->button, BOBGUI_WIDGET (self));

  self->level = BOBGUI_FONT_LEVEL_FONT;

  self->use_font = FALSE;
  self->use_size = FALSE;

  font_desc = pango_font_description_from_string ("Sans 12");
  bobgui_font_dialog_button_set_font_desc (self, font_desc);
  pango_font_description_free (font_desc);

  bobgui_widget_add_css_class (self->button, "font");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                  -1);
}

static void
bobgui_font_dialog_button_unroot (BobguiWidget *widget)
{
  BobguiFontDialogButton *self = BOBGUI_FONT_DIALOG_BUTTON (widget);

  if (self->cancellable)
    {
      g_cancellable_cancel (self->cancellable);
      g_clear_object (&self->cancellable);
      update_button_sensitivity (self);
    }

  BOBGUI_WIDGET_CLASS (bobgui_font_dialog_button_parent_class)->unroot (widget);
}

static void
bobgui_font_dialog_button_set_property (GObject      *object,
                                     unsigned int  param_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiFontDialogButton *self = BOBGUI_FONT_DIALOG_BUTTON (object);

  switch (param_id)
    {
    case PROP_DIALOG:
      bobgui_font_dialog_button_set_dialog (self, g_value_get_object (value));
      break;

    case PROP_LEVEL:
      bobgui_font_dialog_button_set_level (self, g_value_get_enum (value));
      break;

    case PROP_FONT_DESC:
      bobgui_font_dialog_button_set_font_desc (self, g_value_get_boxed (value));
      break;

    case PROP_FONT_FEATURES:
      bobgui_font_dialog_button_set_font_features (self, g_value_get_string (value));
      break;

    case PROP_LANGUAGE:
      bobgui_font_dialog_button_set_language (self, g_value_get_boxed (value));
      break;

    case PROP_USE_FONT:
      bobgui_font_dialog_button_set_use_font (self, g_value_get_boolean (value));
      break;

    case PROP_USE_SIZE:
      bobgui_font_dialog_button_set_use_size (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_font_dialog_button_get_property (GObject      *object,
                                     unsigned int  param_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  BobguiFontDialogButton *self = BOBGUI_FONT_DIALOG_BUTTON (object);

  switch (param_id)
    {
    case PROP_DIALOG:
      g_value_set_object (value, self->dialog);
      break;

    case PROP_LEVEL:
      g_value_set_enum (value, self->level);
      break;

    case PROP_FONT_DESC:
      g_value_set_boxed (value, self->font_desc);
      break;

    case PROP_FONT_FEATURES:
      g_value_set_string (value, self->font_features);
      break;

    case PROP_LANGUAGE:
      g_value_set_boxed (value, self->language);
      break;

    case PROP_USE_FONT:
      g_value_set_boolean (value, self->use_font);
      break;

    case PROP_USE_SIZE:
      g_value_set_boolean (value, self->use_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_font_dialog_button_dispose (GObject *object)
{
  BobguiFontDialogButton *self = BOBGUI_FONT_DIALOG_BUTTON (object);

  g_clear_pointer (&self->button, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_font_dialog_button_parent_class)->dispose (object);
}

static void
bobgui_font_dialog_button_finalize (GObject *object)
{
  BobguiFontDialogButton *self = BOBGUI_FONT_DIALOG_BUTTON (object);

  g_assert (self->cancellable == NULL);
  g_clear_object (&self->dialog);
  pango_font_description_free (self->font_desc);
  g_clear_object (&self->font_family);
  g_clear_object (&self->font_face);
  g_free (self->font_features);

  G_OBJECT_CLASS (bobgui_font_dialog_button_parent_class)->finalize (object);
}

static void
bobgui_font_dialog_button_class_init (BobguiFontDialogButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->get_property = bobgui_font_dialog_button_get_property;
  object_class->set_property = bobgui_font_dialog_button_set_property;
  object_class->dispose = bobgui_font_dialog_button_dispose;
  object_class->finalize = bobgui_font_dialog_button_finalize;

  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->unroot = bobgui_font_dialog_button_unroot;

  /**
   * BobguiFontDialogButton:dialog:
   *
   * The `BobguiFontDialog` that contains parameters for
   * the font chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_DIALOG] =
      g_param_spec_object ("dialog", NULL, NULL,
                           BOBGUI_TYPE_FONT_DIALOG,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:level:
   *
   * The level of detail for the font chooser dialog.
   */
  properties[PROP_LEVEL] =
      g_param_spec_enum ("level", NULL, NULL,
                         BOBGUI_TYPE_FONT_LEVEL,
                         BOBGUI_FONT_LEVEL_FONT,
                         BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:font-desc:
   *
   * The selected font.
   *
   * This property can be set to give the button its initial
   * font, and it will be updated to reflect the users choice
   * in the font chooser dialog.
   *
   * Listen to `notify::font-desc` to get informed about changes
   * to the buttons font.
   *
   * Since: 4.10
   */
  properties[PROP_FONT_DESC] =
      g_param_spec_boxed ("font-desc", NULL, NULL,
                          PANGO_TYPE_FONT_DESCRIPTION,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:font-features:
   *
   * The selected font features.
   *
   * This property will be updated to reflect the users choice
   * in the font chooser dialog.
   *
   * Listen to `notify::font-features` to get informed about changes
   * to the buttons font features.
   *
   * Since: 4.10
   */
  properties[PROP_FONT_FEATURES] =
      g_param_spec_string ("font-features", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:language:
   *
   * The selected language for font features.
   *
   * This property will be updated to reflect the users choice
   * in the font chooser dialog.
   *
   * Listen to `notify::language` to get informed about changes
   * to the buttons language.
   *
   * Since: 4.10
   */
  properties[PROP_LANGUAGE] =
      g_param_spec_boxed ("language", NULL, NULL,
                          PANGO_TYPE_LANGUAGE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:use-font:
   *
   * Whether the buttons label will be drawn in the selected font.
   */
  properties[PROP_USE_FONT] =
      g_param_spec_boolean ("use-font", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialogButton:use-size:
   *
   * Whether the buttons label will use the selected font size.
   */
  properties[PROP_USE_SIZE] =
      g_param_spec_boolean ("use-size", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  /**
   * BobguiFontDialogButton::activate:
   * @widget: The object which received the signal
   *
   * Emitted when the font dialog button is activated.
   *
   * The `::activate` signal on `BobguiFontDialogButton` is an action signal
   * and emitting it causes the button to pop up its dialog.
   *
   * Since: 4.14
   */
  font_dialog_button_signals[SIGNAL_ACTIVATE] =
    g_signal_new (I_ ("activate"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, font_dialog_button_signals[SIGNAL_ACTIVATE]);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "fontbutton");
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

/* }}} */
/* {{{ Private API, callbacks */

static void
update_button_sensitivity (BobguiFontDialogButton *self)
{
  if (self->button)
    bobgui_widget_set_sensitive (self->button,
                              self->dialog != NULL && self->cancellable == NULL);
}

static void
family_chosen (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  BobguiFontDialog *dialog = BOBGUI_FONT_DIALOG (source);
  BobguiFontDialogButton *self = data;
  PangoFontFamily *family;

  family = bobgui_font_dialog_choose_family_finish (dialog, result, NULL);
  if (family)
    {
      PangoFontDescription *desc;

      desc = pango_font_description_new ();
      pango_font_description_set_family (desc, pango_font_family_get_name (family));

      bobgui_font_dialog_button_set_font_desc (self, desc);

      pango_font_description_free (desc);
      g_object_unref (family);
    }

  g_clear_object (&self->cancellable);
  update_button_sensitivity (self);
}

static void
face_chosen (GObject      *source,
             GAsyncResult *result,
             gpointer      data)
{
  BobguiFontDialog *dialog = BOBGUI_FONT_DIALOG (source);
  BobguiFontDialogButton *self = data;
  PangoFontFace *face;

  face = bobgui_font_dialog_choose_face_finish (dialog, result, NULL);
  if (face)
    {
      PangoFontDescription *desc;

      desc = pango_font_face_describe (face);

      bobgui_font_dialog_button_set_font_desc (self, desc);

      pango_font_description_free (desc);
      g_object_unref (face);
    }

  g_clear_object (&self->cancellable);
  update_button_sensitivity (self);
}

static void
font_chosen (GObject      *source,
             GAsyncResult *result,
             gpointer      data)
{
  BobguiFontDialog *dialog = BOBGUI_FONT_DIALOG (source);
  BobguiFontDialogButton *self = data;
  PangoFontDescription *desc;

  desc = bobgui_font_dialog_choose_font_finish (dialog, result, NULL);
  if (desc)
    {
      bobgui_font_dialog_button_set_font_desc (self, desc);
      pango_font_description_free (desc);
    }

  g_clear_object (&self->cancellable);
  update_button_sensitivity (self);
}

static void
font_and_features_chosen (GObject      *source,
                          GAsyncResult *result,
                          gpointer      data)
{
  BobguiFontDialog *dialog = BOBGUI_FONT_DIALOG (source);
  BobguiFontDialogButton *self = data;
  PangoFontDescription *desc;
  char *features;
  PangoLanguage *language;

  if (bobgui_font_dialog_choose_font_and_features_finish (dialog, result,
                                                       &desc, &features, &language,
                                                       NULL))
    {
      bobgui_font_dialog_button_set_font_desc (self, desc);
      bobgui_font_dialog_button_set_font_features (self, features);
      bobgui_font_dialog_button_set_language (self, language);

      pango_font_description_free (desc);
      g_free (features);
    }

  g_clear_object (&self->cancellable);
  update_button_sensitivity (self);
}

static void
activated (BobguiFontDialogButton *self)
{
  bobgui_widget_activate (self->button);
}

static void
button_clicked (BobguiFontDialogButton *self)
{
  BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (self));
  BobguiWindow *parent = NULL;

  g_assert (self->cancellable == NULL);
  self->cancellable = g_cancellable_new ();

  update_button_sensitivity (self);

  if (BOBGUI_IS_WINDOW (root))
    parent = BOBGUI_WINDOW (root);

  switch (self->level)
    {
    case BOBGUI_FONT_LEVEL_FAMILY:
      bobgui_font_dialog_choose_family (self->dialog, parent, self->font_family,
                                     self->cancellable, family_chosen, self);
      break;

    case BOBGUI_FONT_LEVEL_FACE:
      bobgui_font_dialog_choose_face (self->dialog, parent, self->font_face,
                                   self->cancellable, face_chosen, self);
      break;

    case BOBGUI_FONT_LEVEL_FONT:
      bobgui_font_dialog_choose_font (self->dialog, parent, self->font_desc,
                                   self->cancellable, font_chosen, self);
      break;

    case BOBGUI_FONT_LEVEL_FEATURES:
      bobgui_font_dialog_choose_font_and_features (self->dialog, parent, self->font_desc,
                                                self->cancellable, font_and_features_chosen, self);
      break;

    default:
      g_assert_not_reached ();
    }
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
update_font_data (BobguiFontDialogButton *self)
{
  PangoFontMap *fontmap = NULL;
  const char *family_name;

  g_assert (self->font_desc != NULL);

  g_clear_object (&self->font_family);
  g_clear_object (&self->font_face);

  family_name = pango_font_description_get_family (self->font_desc);
  if (family_name == NULL)
    return;

  if (self->dialog)
    fontmap = bobgui_font_dialog_get_font_map (self->dialog);
  if (!fontmap)
    fontmap = pango_cairo_font_map_get_default ();

  for (unsigned int i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (fontmap)); i++)
    {
      PangoFontFamily *family = g_list_model_get_item (G_LIST_MODEL (fontmap), i);
      const char *name = pango_font_family_get_name (family);
      g_object_unref (family);

      if (g_ascii_strcasecmp (name, family_name) == 0)
        {
          g_set_object (&self->font_family, family);
          break;
        }
    }

  if (self->font_family == NULL)
    return;

  for (unsigned i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->font_family)); i++)
    {
      PangoFontFace *face = g_list_model_get_item (G_LIST_MODEL (self->font_family), i);
      PangoFontDescription *tmp_desc = pango_font_face_describe (face);
      g_object_unref (face);

      if (font_description_style_equal (tmp_desc, self->font_desc))
        {
          g_set_object (&self->font_face, face);
          pango_font_description_free (tmp_desc);
          break;
        }
      else
        pango_font_description_free (tmp_desc);
    }
}

static void
update_font_info (BobguiFontDialogButton *self)
{
  const char *fam_name;
  const char *face_name;
  char *family_style;
  char *size;

  if (self->font_family)
    fam_name = pango_font_family_get_name (self->font_family);
  else
    fam_name = C_("font", "None");
  if (self->font_face)
    face_name = pango_font_face_get_face_name (self->font_face);
  else
    face_name = "";

  if (self->level == BOBGUI_FONT_LEVEL_FAMILY)
    family_style = g_strdup (fam_name);
  else
    family_style = g_strconcat (fam_name, " ", face_name, NULL);

  bobgui_label_set_text (BOBGUI_LABEL (self->font_label), family_style);
  g_free (family_style);

  if (self->level >= BOBGUI_FONT_LEVEL_FONT)
    {
      /* mirror Pango, which doesn't translate this either */
      size = g_strdup_printf ("%2.4g%s",
                              pango_font_description_get_size (self->font_desc) / (double)PANGO_SCALE,
                              pango_font_description_get_size_is_absolute (self->font_desc) ? "px" : "");

      bobgui_label_set_text (BOBGUI_LABEL (self->size_label), size);
      g_free (size);
    }

  bobgui_widget_set_visible (self->font_size_box, self->level >= BOBGUI_FONT_LEVEL_FONT);
}

static void
apply_use_font (BobguiFontDialogButton *self)
{
  if (!self->use_font)
    bobgui_label_set_attributes (BOBGUI_LABEL (self->font_label), NULL);
  else
    {
      PangoFontDescription *desc;
      PangoAttrList *attrs;
      PangoLanguage *language;

      desc = pango_font_description_copy (self->font_desc);

      if (!self->use_size)
        pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);

      attrs = pango_attr_list_new ();

      /* Prevent font fallback */
      pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));

      /* Force current font and features */
      pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
      if (self->font_features)
        pango_attr_list_insert (attrs, pango_attr_font_features_new (self->font_features));
      if (self->language)
        language = self->language;
      else if (self->dialog)
        language = bobgui_font_dialog_get_language (self->dialog);
      else
        language = NULL;
      if (language)
        pango_attr_list_insert (attrs, pango_attr_language_new (language));

      bobgui_label_set_attributes (BOBGUI_LABEL (self->font_label), attrs);

      pango_attr_list_unref (attrs);
      pango_font_description_free (desc);
    }
}

/* }}} */
/* {{{ Constructor */

/**
 * bobgui_font_dialog_button_new:
 * @dialog: (nullable) (transfer full): the `BobguiFontDialog` to use
 *
 * Creates a new `BobguiFontDialogButton` with the
 * given `BobguiFontDialog`.
 *
 * You can pass `NULL` to this function and set a `BobguiFontDialog`
 * later. The button will be insensitive until that happens.
 *
 * Returns: the new `BobguiFontDialogButton`
 *
 * Since: 4.10
 */
BobguiWidget *
bobgui_font_dialog_button_new (BobguiFontDialog *dialog)
{
  BobguiWidget *self;

  g_return_val_if_fail (dialog == NULL || BOBGUI_IS_FONT_DIALOG (dialog), NULL);

  self = g_object_new (BOBGUI_TYPE_FONT_DIALOG_BUTTON,
                       "dialog", dialog,
                       NULL);

  g_clear_object (&dialog);

  return self;
}

/* }}} */
/* {{{ Getters and setters */

/**
 * bobgui_font_dialog_button_set_dialog:
 * @self: a `BobguiFontDialogButton`
 * @dialog: the new `BobguiFontDialog`
 *
 * Sets a `BobguiFontDialog` object to use for
 * creating the font chooser dialog that is
 * presented when the user clicks the button.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_dialog (BobguiFontDialogButton *self,
                                   BobguiFontDialog       *dialog)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));
  g_return_if_fail (dialog == NULL || BOBGUI_IS_FONT_DIALOG (dialog));

  if (!g_set_object (&self->dialog, dialog))
    return;

  update_button_sensitivity (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DIALOG]);
}

/**
 * bobgui_font_dialog_button_get_dialog:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns the `BobguiFontDialog` of @self.
 *
 * Returns: (nullable) (transfer none): the `BobguiFontDialog`
 *
 * Since: 4.10
 */
BobguiFontDialog *
bobgui_font_dialog_button_get_dialog (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), NULL);

  return self->dialog;
}

/**
 * bobgui_font_dialog_button_get_level:
 * @self: a `BobguiFontDialogButton
 *
 * Returns the level of detail at which this dialog
 * lets the user select fonts.
 *
 * Returns: the level of detail
 *
 * Since: 4.10
 */
BobguiFontLevel
bobgui_font_dialog_button_get_level (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), BOBGUI_FONT_LEVEL_FONT);

  return self->level;
}

/**
 * bobgui_font_dialog_button_set_level:
 * @self: a `BobguiFontDialogButton`
 * @level: the level of detail
 *
 * Sets the level of detail at which this dialog
 * lets the user select fonts.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_level (BobguiFontDialogButton *self,
                                  BobguiFontLevel         level)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));

  if (self->level == level)
    return;

  self->level = level;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LEVEL]);
}

/**
 * bobgui_font_dialog_button_set_font_desc:
 * @self: a `BobguiFontDialogButton`
 * @font_desc: the new font
 *
 * Sets the font of the button.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_font_desc (BobguiFontDialogButton        *self,
                                      const PangoFontDescription *font_desc)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));
  g_return_if_fail (font_desc != NULL);

  if (self->font_desc == font_desc ||
      (self->font_desc && font_desc &&
       pango_font_description_equal (self->font_desc, font_desc)))
    return;

  if (self->font_desc)
    pango_font_description_free (self->font_desc);

  self->font_desc = pango_font_description_copy (font_desc);

  update_font_data (self);
  update_font_info (self);
  apply_use_font (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT_DESC]);
}

/**
 * bobgui_font_dialog_button_get_font_desc:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns the font of the button.
 *
 * This function is what should be used to obtain
 * the font that was chosen by the user. To get
 * informed about changes, listen to "notify::font-desc".
 *
 * Returns: (transfer none) (nullable): the font
 *
 * Since: 4.10
 */
PangoFontDescription *
bobgui_font_dialog_button_get_font_desc (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), NULL);

  return self->font_desc;
}

/**
 * bobgui_font_dialog_button_set_font_features:
 * @self: a `BobguiFontDialogButton`
 * @font_features: (nullable): the font features
 *
 * Sets the font features of the button.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_font_features (BobguiFontDialogButton *self,
                                          const char          *font_features)
{
  char *new_features;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));

  if (g_strcmp0 (self->font_features, font_features) == 0)
    return;

  new_features = g_strdup (font_features);
  g_free (self->font_features);
  self->font_features = new_features;

  apply_use_font (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT_FEATURES]);
}

/**
 * bobgui_font_dialog_button_get_font_features:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns the font features of the button.
 *
 * This function is what should be used to obtain the font features
 * that were chosen by the user. To get informed about changes, listen
 * to "notify::font-features".
 *
 * Note that the button will only let users choose font features
 * if [property@Bobgui.FontDialogButton:level] is set to
 * `BOBGUI_FONT_LEVEL_FEATURES`.
 *
 * Returns: (transfer none) (nullable): the font features
 *
 * Since: 4.10
 */
const char *
bobgui_font_dialog_button_get_font_features (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), NULL);

  return self->font_features;
}

/**
 * bobgui_font_dialog_button_set_language:
 * @self: a `BobguiFontDialogButton`
 * @language: (nullable): the new language
 *
 * Sets the language to use for font features.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_language (BobguiFontDialogButton *self,
                                     PangoLanguage       *language)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));

  if (self->language == language)
    return;

  self->language = language;

  apply_use_font (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LANGUAGE]);
}

/**
 * bobgui_font_dialog_button_get_language:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns the language that is used for font features.
 *
 * Returns: (nullable): the language
 *
 * Since: 4.10
 */
PangoLanguage *
bobgui_font_dialog_button_get_language (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), NULL);

  return self->language;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
/**
 * bobgui_font_dialog_button_set_use_font:
 * @self: a `BobguiFontDialogButton`
 * @use_font: If `TRUE`, font name will be written using
 *   the chosen font
 *
 * If @use_font is `TRUE`, the font name will be written
 * using the selected font.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_use_font (BobguiFontDialogButton *self,
                                     gboolean             use_font)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));

  if (self->use_font == use_font)
    return;

  self->use_font = use_font;

  apply_use_font (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USE_FONT]);
}

/**
 * bobgui_font_dialog_button_get_use_font:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns whether the selected font is used in the label.
 *
 * Returns: whether the selected font is used in the label
 *
 * Since: 4.10
 */
gboolean
bobgui_font_dialog_button_get_use_font (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), FALSE);

  return self->use_font;
}

/**
 * bobgui_font_dialog_button_set_use_size:
 * @self: a `BobguiFontDialogButton`
 * @use_size: If `TRUE`, font name will be written using
 *   the chosen font size
 *
 * If @use_size is `TRUE`, the font name will be written
 * using the selected font size.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_button_set_use_size (BobguiFontDialogButton *self,
                                     gboolean             use_size)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self));

  if (self->use_size == use_size)
    return;

  self->use_size = use_size;

  apply_use_font (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USE_SIZE]);
}

/**
 * bobgui_font_dialog_button_get_use_size:
 * @self: a `BobguiFontDialogButton`
 *
 * Returns whether the selected font size is used in the label.
 *
 * Returns: whether the selected font size is used in the label
 *
 * Since: 4.10
 */
gboolean
bobgui_font_dialog_button_get_use_size (BobguiFontDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG_BUTTON (self), FALSE);

  return self->use_size;
}

/* }}} */

/* vim:set foldmethod=marker: */
