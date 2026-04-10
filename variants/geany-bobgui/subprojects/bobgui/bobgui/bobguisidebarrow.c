/* bobguisidebarrow.c
 *
 * Copyright (C) 2015 Carlos Soriano <csoriano@gnome.org>
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
 */

#include "config.h"

#include "bobguisidebarrowprivate.h"
/* For section and place type enums */
#include "bobguiplacessidebarprivate.h"
#include "bobguiwidget.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguirevealer.h"
#include "bobguispinner.h"
#include "bobguiprivatetypebuiltins.h"
#include "bobguiprivate.h"

#ifdef HAVE_CLOUDPROVIDERS
#include <cloudproviders.h>
#endif

struct _BobguiSidebarRow
{
  BobguiListBoxRow parent_instance;
  GIcon *start_icon;
  GIcon *end_icon;
  BobguiWidget *start_icon_widget;
  BobguiWidget *end_icon_widget;
  char *label;
  char *tooltip;
  BobguiWidget *label_widget;
  gboolean ejectable;
  BobguiWidget *eject_button;
  int order_index;
  BobguiPlacesSectionType section_type;
  BobguiPlacesPlaceType place_type;
  char *uri;
  GDrive *drive;
  GVolume *volume;
  GMount *mount;
  GObject *cloud_provider_account;
  gboolean placeholder;
  BobguiPlacesSidebar *sidebar;
  BobguiWidget *revealer;
  BobguiWidget *busy_spinner;
};

G_DEFINE_TYPE (BobguiSidebarRow, bobgui_sidebar_row, BOBGUI_TYPE_LIST_BOX_ROW)

enum
{
  PROP_0,
  PROP_START_ICON,
  PROP_END_ICON,
  PROP_LABEL,
  PROP_TOOLTIP,
  PROP_EJECTABLE,
  PROP_SIDEBAR,
  PROP_ORDER_INDEX,
  PROP_SECTION_TYPE,
  PROP_PLACE_TYPE,
  PROP_URI,
  PROP_DRIVE,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_CLOUD_PROVIDER_ACCOUNT,
  PROP_PLACEHOLDER,
  LAST_PROP
};

static GParamSpec *properties [LAST_PROP];

#ifdef HAVE_CLOUDPROVIDERS

static void
cloud_row_update (BobguiSidebarRow *self)
{
  CloudProvidersAccount *account;
  GIcon *end_icon;
  int provider_status;

  account = CLOUD_PROVIDERS_ACCOUNT (self->cloud_provider_account);
  provider_status = cloud_providers_account_get_status (account);
  switch (provider_status)
    {
      case CLOUD_PROVIDERS_ACCOUNT_STATUS_IDLE:
        end_icon = NULL;
        break;

      case CLOUD_PROVIDERS_ACCOUNT_STATUS_SYNCING:
        end_icon = g_themed_icon_new ("emblem-synchronizing-symbolic");
        break;

      case CLOUD_PROVIDERS_ACCOUNT_STATUS_ERROR:
        end_icon = g_themed_icon_new ("dialog-warning-symbolic");
        break;

      default:
        return;
    }

  g_object_set (self,
                "label", cloud_providers_account_get_name (account),
                NULL);
  g_object_set (self,
                "tooltip", cloud_providers_account_get_status_details (account),
                NULL);
  g_object_set (self,
                "end-icon", end_icon,
                NULL);

  if (end_icon != NULL)
    g_object_unref (end_icon);
}

#endif

static void
bobgui_sidebar_row_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiSidebarRow *self = BOBGUI_SIDEBAR_ROW (object);

  switch (prop_id)
    {
    case PROP_SIDEBAR:
      g_value_set_object (value, self->sidebar);
      break;

    case PROP_START_ICON:
      g_value_set_object (value, self->start_icon);
      break;

    case PROP_END_ICON:
      g_value_set_object (value, self->end_icon);
      break;

    case PROP_LABEL:
      g_value_set_string (value, self->label);
      break;

    case PROP_TOOLTIP:
      g_value_set_string (value, self->tooltip);
      break;

    case PROP_EJECTABLE:
      g_value_set_boolean (value, self->ejectable);
      break;

    case PROP_ORDER_INDEX:
      g_value_set_int (value, self->order_index);
      break;

    case PROP_SECTION_TYPE:
      g_value_set_enum (value, self->section_type);
      break;

    case PROP_PLACE_TYPE:
      g_value_set_enum (value, self->place_type);
      break;

    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;

    case PROP_DRIVE:
      g_value_set_object (value, self->drive);
      break;

    case PROP_VOLUME:
      g_value_set_object (value, self->volume);
      break;

    case PROP_MOUNT:
      g_value_set_object (value, self->mount);
      break;

    case PROP_CLOUD_PROVIDER_ACCOUNT:
      g_value_set_object (value, self->cloud_provider_account);
      break;

    case PROP_PLACEHOLDER:
      g_value_set_boolean (value, self->placeholder);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_sidebar_row_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiSidebarRow *self = BOBGUI_SIDEBAR_ROW (object);

  switch (prop_id)
    {
    case PROP_SIDEBAR:
      self->sidebar = g_value_get_object (value);
      break;

    case PROP_START_ICON:
      {
        g_clear_object (&self->start_icon);
        object = g_value_get_object (value);
        if (object != NULL)
          {
            self->start_icon = G_ICON (g_object_ref (object));
            bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->start_icon_widget), self->start_icon);
          }
        else
          {
            bobgui_image_clear (BOBGUI_IMAGE (self->start_icon_widget));
          }
        break;
      }

    case PROP_END_ICON:
      {
        g_clear_object (&self->end_icon);
        object = g_value_get_object (value);
        if (object != NULL)
          {
            self->end_icon = G_ICON (g_object_ref (object));
            bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->end_icon_widget), self->end_icon);
          }
        else
          {
            bobgui_image_clear (BOBGUI_IMAGE (self->end_icon_widget));
          }
          bobgui_widget_set_visible (self->end_icon_widget, object != NULL);
        break;
      }

    case PROP_LABEL:
      g_free (self->label);
      self->label = g_strdup (g_value_get_string (value));
      bobgui_label_set_text (BOBGUI_LABEL (self->label_widget), self->label);
      break;

    case PROP_TOOLTIP:
      g_free (self->tooltip);
      self->tooltip = g_strdup (g_value_get_string (value));
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), self->tooltip);
      break;

    case PROP_EJECTABLE:
      self->ejectable = g_value_get_boolean (value);
      bobgui_widget_set_visible (self->eject_button, self->ejectable);
      break;

    case PROP_ORDER_INDEX:
      self->order_index = g_value_get_int (value);
      break;

    case PROP_SECTION_TYPE:
      self->section_type = g_value_get_enum (value);
      break;

    case PROP_PLACE_TYPE:
      self->place_type = g_value_get_enum (value);
      break;

    case PROP_URI:
      g_free (self->uri);
      self->uri = g_strdup (g_value_get_string (value));
      break;

    case PROP_DRIVE:
      g_set_object (&self->drive, g_value_get_object (value));
      break;

    case PROP_VOLUME:
      g_set_object (&self->volume, g_value_get_object (value));
      break;

    case PROP_MOUNT:
      g_set_object (&self->mount, g_value_get_object (value));
      break;

    case PROP_CLOUD_PROVIDER_ACCOUNT:
#ifdef HAVE_CLOUDPROVIDERS
      if (self->cloud_provider_account != NULL)
        g_signal_handlers_disconnect_by_data (self->cloud_provider_account, self);

      self->cloud_provider_account = g_value_dup_object (value);

      if (self->cloud_provider_account != NULL)
        {
          g_signal_connect_swapped (self->cloud_provider_account, "notify::name",
                                    G_CALLBACK (cloud_row_update), self);
          g_signal_connect_swapped (self->cloud_provider_account, "notify::status",
                                    G_CALLBACK (cloud_row_update), self);
          g_signal_connect_swapped (self->cloud_provider_account, "notify::status-details",
                                    G_CALLBACK (cloud_row_update), self);
        }
#endif
      break;

    case PROP_PLACEHOLDER:
      {
        self->placeholder = g_value_get_boolean (value);
        if (self->placeholder)
          {
            g_clear_object (&self->start_icon);
            g_clear_object (&self->end_icon);
            g_free (self->label);
            self->label = NULL;
            g_free (self->tooltip);
            self->tooltip = NULL;
            bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), NULL);
            self->ejectable = FALSE;
            self->section_type = BOBGUI_PLACES_SECTION_BOOKMARKS;
            self->place_type = BOBGUI_PLACES_BOOKMARK_PLACEHOLDER;
            g_free (self->uri);
            self->uri = NULL;
            g_clear_object (&self->drive);
            g_clear_object (&self->volume);
            g_clear_object (&self->mount);
            g_clear_object (&self->cloud_provider_account);

            bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (self), NULL);

            bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "sidebar-placeholder-row");
          }

        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_child_revealed (GObject    *self,
                   GParamSpec *pspec,
                   gpointer    user_data)
{
 /* We need to hide the actual widget because if not the BobguiListBoxRow will
  * still allocate the paddings, even if the revealer is not revealed, and
  * therefore the row will be still somewhat visible. */
  if (!bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (self)))
    bobgui_widget_set_visible (BOBGUI_WIDGET (BOBGUI_SIDEBAR_ROW (user_data)), FALSE);
}

void
bobgui_sidebar_row_reveal (BobguiSidebarRow *self)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (self), TRUE);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (self->revealer), TRUE);
}

void
bobgui_sidebar_row_hide (BobguiSidebarRow *self,
                      gboolean       immediate)
{
  guint transition_duration;

  transition_duration = bobgui_revealer_get_transition_duration (BOBGUI_REVEALER (self->revealer));
  if (immediate)
      bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (self->revealer), 0);

  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (self->revealer), FALSE);

  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (self->revealer), transition_duration);
}

void
bobgui_sidebar_row_set_start_icon (BobguiSidebarRow *self,
                                GIcon         *icon)
{
  g_return_if_fail (BOBGUI_IS_SIDEBAR_ROW (self));

  if (self->start_icon != icon)
    {
      g_set_object (&self->start_icon, icon);
      if (self->start_icon != NULL)
        bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->start_icon_widget), self->start_icon);
      else
        bobgui_image_clear (BOBGUI_IMAGE (self->start_icon_widget));

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_START_ICON]);
    }
}

void
bobgui_sidebar_row_set_end_icon (BobguiSidebarRow *self,
                              GIcon         *icon)
{
  g_return_if_fail (BOBGUI_IS_SIDEBAR_ROW (self));

  if (self->end_icon != icon)
    {
      g_set_object (&self->end_icon, icon);
      if (self->end_icon != NULL)
        bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->end_icon_widget), self->end_icon);
      else
        if (self->end_icon_widget != NULL)
          bobgui_image_clear (BOBGUI_IMAGE (self->end_icon_widget));

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_END_ICON]);
    }
}

static void
bobgui_sidebar_row_finalize (GObject *object)
{
  BobguiSidebarRow *self = BOBGUI_SIDEBAR_ROW (object);

  g_clear_object (&self->start_icon);
  g_clear_object (&self->end_icon);
  g_free (self->label);
  self->label = NULL;
  g_free (self->tooltip);
  self->tooltip = NULL;
  g_free (self->uri);
  self->uri = NULL;
  g_clear_object (&self->drive);
  g_clear_object (&self->volume);
  g_clear_object (&self->mount);
#ifdef HAVE_CLOUDPROVIDERS
  if (self->cloud_provider_account != NULL)
    g_signal_handlers_disconnect_by_data (self->cloud_provider_account, self);
  g_clear_object (&self->cloud_provider_account);
#endif

  G_OBJECT_CLASS (bobgui_sidebar_row_parent_class)->finalize (object);
}

static void
bobgui_sidebar_row_init (BobguiSidebarRow *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  bobgui_widget_set_focus_on_click (BOBGUI_WIDGET (self), FALSE);
}

static void
bobgui_sidebar_row_class_init (BobguiSidebarRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = bobgui_sidebar_row_get_property;
  object_class->set_property = bobgui_sidebar_row_set_property;
  object_class->finalize = bobgui_sidebar_row_finalize;

  properties [PROP_SIDEBAR] =
    g_param_spec_object ("sidebar", NULL, NULL,
                         BOBGUI_TYPE_PLACES_SIDEBAR,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_START_ICON] =
    g_param_spec_object ("start-icon", NULL, NULL,
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_END_ICON] =
    g_param_spec_object ("end-icon", NULL, NULL,
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_LABEL] =
    g_param_spec_string ("label", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_TOOLTIP] =
    g_param_spec_string ("tooltip", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_EJECTABLE] =
    g_param_spec_boolean ("ejectable", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE |
                           G_PARAM_STATIC_STRINGS));

  properties [PROP_ORDER_INDEX] =
    g_param_spec_int ("order-index", NULL, NULL,
                      0, G_MAXINT, 0,
                      (G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS));

  properties [PROP_SECTION_TYPE] =
    g_param_spec_enum ("section-type", NULL, NULL,
                       BOBGUI_TYPE_PLACES_SECTION_TYPE,
                       BOBGUI_PLACES_SECTION_INVALID,
                       (G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS |
                        G_PARAM_CONSTRUCT_ONLY));

  properties [PROP_PLACE_TYPE] =
    g_param_spec_enum ("place-type", NULL, NULL,
                       BOBGUI_TYPE_PLACES_PLACE_TYPE,
                       BOBGUI_PLACES_INVALID,
                       (G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS |
                        G_PARAM_CONSTRUCT_ONLY));

  properties [PROP_URI] =
    g_param_spec_string ("uri", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_DRIVE] =
    g_param_spec_object ("drive", NULL, NULL,
                         G_TYPE_DRIVE,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_VOLUME] =
    g_param_spec_object ("volume", NULL, NULL,
                         G_TYPE_VOLUME,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_MOUNT] =
    g_param_spec_object ("mount", NULL, NULL,
                         G_TYPE_MOUNT,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_CLOUD_PROVIDER_ACCOUNT] =
    g_param_spec_object ("cloud-provider-account", NULL, NULL,
                         G_TYPE_OBJECT,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_PLACEHOLDER] =
    g_param_spec_boolean ("placeholder", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE |
                           G_PARAM_CONSTRUCT_ONLY |
                           G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/libbobgui/ui/bobguisidebarrow.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, start_icon_widget);
  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, end_icon_widget);
  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, label_widget);
  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, eject_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, revealer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiSidebarRow, busy_spinner);

  bobgui_widget_class_bind_template_callback (widget_class, on_child_revealed);
  bobgui_widget_class_set_css_name (widget_class, I_("row"));
}

BobguiSidebarRow*
bobgui_sidebar_row_clone (BobguiSidebarRow *self)
{
 return g_object_new (BOBGUI_TYPE_SIDEBAR_ROW,
                      "sidebar", self->sidebar,
                      "start-icon", self->start_icon,
                      "end-icon", self->end_icon,
                      "label", self->label,
                      "tooltip", self->tooltip,
                      "ejectable", self->ejectable,
                      "order-index", self->order_index,
                      "section-type", self->section_type,
                      "place-type", self->place_type,
                      "uri", self->uri,
                      "drive", self->drive,
                      "volume", self->volume,
                      "mount", self->mount,
                      "cloud-provider-account", self->cloud_provider_account,
                      NULL);
}

BobguiWidget*
bobgui_sidebar_row_get_eject_button (BobguiSidebarRow *self)
{
  return self->eject_button;
}

void
bobgui_sidebar_row_set_busy (BobguiSidebarRow *row,
                          gboolean       is_busy)
{
  g_return_if_fail (BOBGUI_IS_SIDEBAR_ROW (row));

  bobgui_widget_set_visible (row->busy_spinner, is_busy);
}
