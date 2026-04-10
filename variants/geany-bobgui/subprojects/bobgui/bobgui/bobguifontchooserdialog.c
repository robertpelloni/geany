/* BOBGUI - The Bobgui Framework
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

#include <stdlib.h>
#include <glib/gprintf.h>
#include <string.h>

#include "bobguifontchooserdialogprivate.h"
#include "deprecated/bobguifontchooser.h"
#include "deprecated/bobguifontchooserwidget.h"
#include "bobguifontchooserwidgetprivate.h"
#include "bobguifontchooserutils.h"
#include <glib/gi18n-lib.h>
#include "bobguibuildable.h"
#include "bobguiprivate.h"
#include "bobguiwidget.h"
#include "bobguisettings.h"
#include "deprecated/bobguidialogprivate.h"
#include "bobguitogglebutton.h"
#include "bobguiheaderbar.h"
#include "bobguiactionable.h"
#include "bobguieventcontrollerkey.h"
#include "bobguiaccessible.h"
#include "svg/bobguisvg.h"
#include "bobguiimage.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _BobguiFontChooserDialogClass BobguiFontChooserDialogClass;

struct _BobguiFontChooserDialog
{
  BobguiDialog parent_instance;

  BobguiWidget *fontchooser;
  BobguiWidget *select_button;
  BobguiWidget *cancel_button;
  BobguiWidget *tweak_button;
};

struct _BobguiFontChooserDialogClass
{
  BobguiDialogClass parent_class;
};

/**
 * BobguiFontChooserDialog:
 *
 * The `BobguiFontChooserDialog` widget is a dialog for selecting a font.
 *
 * <picture>
 *   <source srcset="fontchooser-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiFontChooserDialog" src="fontchooser.png">
 * </picture>
 *
 * `BobguiFontChooserDialog` implements the [iface@Bobgui.FontChooser] interface
 * and does not provide much API of its own.
 *
 * To create a `BobguiFontChooserDialog`, use [ctor@Bobgui.FontChooserDialog.new].
 *
 * # BobguiFontChooserDialog as BobguiBuildable
 *
 * The `BobguiFontChooserDialog` implementation of the `BobguiBuildable`
 * interface exposes the buttons with the names “select_button”
 * and “cancel_button”.
 *
 * ## CSS nodes
 *
 * `BobguiFontChooserDialog` has a single CSS node with the name `window` and style
 * class `.fontchooser`.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] instead
 */

static void     bobgui_font_chooser_dialog_buildable_interface_init     (BobguiBuildableIface *iface);
static GObject *bobgui_font_chooser_dialog_buildable_get_internal_child (BobguiBuildable *buildable,
                                                                      BobguiBuilder   *builder,
                                                                      const char   *childname);

G_DEFINE_TYPE_WITH_CODE (BobguiFontChooserDialog, bobgui_font_chooser_dialog, BOBGUI_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_FONT_CHOOSER,
                                                _bobgui_font_chooser_delegate_iface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_font_chooser_dialog_buildable_interface_init))

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_font_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
    default:
      g_object_set_property (G_OBJECT (dialog->fontchooser), pspec->name, value);
      break;
    }
}

static void
bobgui_font_chooser_dialog_get_property (GObject      *object,
                                      guint         prop_id,
                                      GValue       *value,
                                      GParamSpec   *pspec)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
    default:
      g_object_get_property (G_OBJECT (dialog->fontchooser), pspec->name, value);
      break;
    }
}

static void
font_activated_cb (BobguiFontChooser *fontchooser,
                   const char     *fontname,
                   gpointer        user_data)
{
  BobguiDialog *dialog = user_data;

  bobgui_dialog_response (dialog, BOBGUI_RESPONSE_OK);
}

static gboolean
dialog_forward_key (BobguiEventControllerKey *controller,
                    guint                  keyval,
                    guint                  keycode,
                    GdkModifierType        modifiers,
                    BobguiWidget             *widget)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (widget);

  return bobgui_event_controller_key_forward (controller, dialog->fontchooser);
}

static void
update_tweak_button (BobguiFontChooserDialog *dialog)
{
  BobguiFontChooserLevel level;

  if (!dialog->tweak_button)
    return;

  g_object_get (dialog->fontchooser, "level", &level, NULL);
  bobgui_widget_set_visible (dialog->tweak_button,
                          (level & (BOBGUI_FONT_CHOOSER_LEVEL_FEATURES |
                                    BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS)) != 0);
}

static void
setup_tweak_button (BobguiFontChooserDialog *dialog)
{
  gboolean use_header;

  if (dialog->tweak_button)
    return;

  g_object_get (dialog, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
      BobguiWidget *button;
      BobguiWidget *header;
      GActionGroup *actions;
      BobguiWidget *image;
      BobguiSvg *svg;

      actions = G_ACTION_GROUP (g_simple_action_group_new ());
      g_action_map_add_action (G_ACTION_MAP (actions), bobgui_font_chooser_widget_get_tweak_action (dialog->fontchooser));
      bobgui_widget_insert_action_group (BOBGUI_WIDGET (dialog), "font", actions);
      g_object_unref (actions);

      button = bobgui_toggle_button_new ();
      bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button), "font.tweak");
      bobgui_widget_set_focus_on_click (button, FALSE);
      bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
      svg = bobgui_svg_new_from_resource ("/org/bobgui/libbobgui/icons/sliders.gpa");
      image = bobgui_image_new_from_paintable (GDK_PAINTABLE (svg));
      g_object_unref (svg);
      bobgui_button_set_child (BOBGUI_BUTTON (button), image);
      bobgui_widget_set_tooltip_text (button, _("Change Font Features"));
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                      _("Change Font Features"),
                                      -1);

      header = bobgui_dialog_get_header_bar (BOBGUI_DIALOG (dialog));
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), button);

      dialog->tweak_button = button;

      update_tweak_button (dialog);
    }
}

static void
bobgui_font_chooser_dialog_map (BobguiWidget *widget)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (widget);

  setup_tweak_button (dialog);

  BOBGUI_WIDGET_CLASS (bobgui_font_chooser_dialog_parent_class)->map (widget);
}

static void
update_button (BobguiFontChooserDialog *dialog)
{
  PangoFontDescription *desc;

  desc = bobgui_font_chooser_get_font_desc (BOBGUI_FONT_CHOOSER (dialog->fontchooser));

  bobgui_widget_set_sensitive (dialog->select_button, desc != NULL);

  if (desc)
    pango_font_description_free (desc);
}

static void
bobgui_font_chooser_dialog_dispose (GObject *object)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (object);

  if (dialog->fontchooser)
    {
      g_signal_handlers_disconnect_by_func (dialog->fontchooser,
                                            update_button,
                                            dialog);
      g_signal_handlers_disconnect_by_func (dialog->fontchooser,
                                            update_tweak_button,
                                            dialog);
    }

  /* tweak_button is not a template child */
  g_clear_pointer (&dialog->tweak_button, bobgui_widget_unparent);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (dialog), BOBGUI_TYPE_FONT_CHOOSER_DIALOG);

  G_OBJECT_CLASS (bobgui_font_chooser_dialog_parent_class)->dispose (object);
}

static void
bobgui_font_chooser_dialog_class_init (BobguiFontChooserDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->dispose = bobgui_font_chooser_dialog_dispose;
  gobject_class->get_property = bobgui_font_chooser_dialog_get_property;
  gobject_class->set_property = bobgui_font_chooser_dialog_set_property;

  widget_class->map = bobgui_font_chooser_dialog_map;

  _bobgui_font_chooser_install_properties (gobject_class);

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/libbobgui/ui/bobguifontchooserdialog.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserDialog, fontchooser);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserDialog, select_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserDialog, cancel_button);
  bobgui_widget_class_bind_template_callback (widget_class, font_activated_cb);
  bobgui_widget_class_bind_template_callback (widget_class, dialog_forward_key);
}

static void
bobgui_font_chooser_dialog_init (BobguiFontChooserDialog *dialog)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
  bobgui_dialog_set_use_header_bar_from_setting (BOBGUI_DIALOG (dialog));

  _bobgui_font_chooser_set_delegate (BOBGUI_FONT_CHOOSER (dialog),
                                  BOBGUI_FONT_CHOOSER (dialog->fontchooser));

  g_signal_connect_swapped (dialog->fontchooser, "notify::font-desc",
                            G_CALLBACK (update_button), dialog);
  update_button (dialog);
  g_signal_connect_swapped (dialog->fontchooser, "notify::level",
                            G_CALLBACK (update_tweak_button), dialog);
}

/**
 * bobgui_font_chooser_dialog_new:
 * @title: (nullable): Title of the dialog
 * @parent: (nullable): Transient parent of the dialog
 *
 * Creates a new `BobguiFontChooserDialog`.
 *
 * Returns: a new `BobguiFontChooserDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.FontDialog] instead
 */
BobguiWidget*
bobgui_font_chooser_dialog_new (const char *title,
                             BobguiWindow   *parent)
{
  BobguiFontChooserDialog *dialog;

  dialog = g_object_new (BOBGUI_TYPE_FONT_CHOOSER_DIALOG,
                         "title", title,
                         "transient-for", parent,
                         NULL);

  return BOBGUI_WIDGET (dialog);
}

static void
bobgui_font_chooser_dialog_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = bobgui_font_chooser_dialog_buildable_get_internal_child;
}

static GObject *
bobgui_font_chooser_dialog_buildable_get_internal_child (BobguiBuildable *buildable,
                                                      BobguiBuilder   *builder,
                                                      const char   *childname)
{
  BobguiFontChooserDialog *dialog = BOBGUI_FONT_CHOOSER_DIALOG (buildable);

  if (g_strcmp0 (childname, "select_button") == 0)
    return G_OBJECT (dialog->select_button);
  else if (g_strcmp0 (childname, "cancel_button") == 0)
    return G_OBJECT (dialog->cancel_button);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

void
bobgui_font_chooser_dialog_set_filter (BobguiFontChooserDialog *dialog,
                                    BobguiFilter            *filter)
{
  bobgui_font_chooser_widget_set_filter (BOBGUI_FONT_CHOOSER_WIDGET (dialog->fontchooser), filter);
}
