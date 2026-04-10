/*
 * bobguiappchooserwidget.c: an app-chooser widget
 *
 * Copyright (C) 2004 Novell, Inc.
 * Copyright (C) 2007, 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Dave Camp <dave@novell.com>
 *          Alexander Larsson <alexl@redhat.com>
 *          Cosimo Cecchi <ccecchi@redhat.com>
 */

#include "config.h"

#include "bobguiappchooserwidget.h"

#include "bobguimarshalers.h"
#include "bobguiappchooserwidget.h"
#include "bobguiappchooserprivate.h"
#include "bobguiliststore.h"
#include "bobguiorientable.h"
#include "bobguiscrolledwindow.h"
#include "bobguilabel.h"
#include "bobguigestureclick.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"
#include "bobguibox.h"
#include "bobguilistview.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "bobguisingleselection.h"
#include "bobguifilterlistmodel.h"
#include "bobguistringfilter.h"
#include "bobguisortlistmodel.h"
#include "bobguistringsorter.h"
#include "bobguicustomsorter.h"
#include "bobguilistheader.h"
#include "bobguisearchentry.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiAppChooserWidget:
 *
 * `BobguiAppChooserWidget` is a widget for selecting applications.
 *
 * It is the main building block for [class@Bobgui.AppChooserDialog].
 * Most applications only need to use the latter; but you can use
 * this widget as part of a larger widget if you have special needs.
 *
 * `BobguiAppChooserWidget` offers detailed control over what applications
 * are shown, using the
 * [property@Bobgui.AppChooserWidget:show-default],
 * [property@Bobgui.AppChooserWidget:show-recommended],
 * [property@Bobgui.AppChooserWidget:show-fallback],
 * [property@Bobgui.AppChooserWidget:show-other] and
 * [property@Bobgui.AppChooserWidget:show-all] properties. See the
 * [iface@Bobgui.AppChooser] documentation for more information about these
 * groups of applications.
 *
 * To keep track of the selected application, use the
 * [signal@Bobgui.AppChooserWidget::application-selected] and
 * [signal@Bobgui.AppChooserWidget::application-activated] signals.
 *
 * ## CSS nodes
 *
 * `BobguiAppChooserWidget` has a single CSS node with name appchooser.
 *
 * Deprecated: 4.10: The application selection widgets should be
 *   implemented according to the design of each platform and/or
 *   application requiring them.
 */

G_DECLARE_FINAL_TYPE (BobguiAppItem, bobgui_app_item, BOBGUI, APP_ITEM, GObject)

struct _BobguiAppItem
{
  GObject parent_instance;
  GAppInfo *app_info;
  gboolean is_default;
  gboolean is_recommended;
  gboolean is_fallback;
};

enum {
  ITEM_PROP_NAME = 1,
  NUM_ITEM_PROPS,
};

static GParamSpec *item_properties[NUM_ITEM_PROPS];

G_DEFINE_TYPE (BobguiAppItem, bobgui_app_item, G_TYPE_OBJECT)

static void
bobgui_app_item_init (BobguiAppItem *item)
{
}

static void
bobgui_app_item_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiAppItem *item = BOBGUI_APP_ITEM (object);

  switch (prop_id)
    {
    case ITEM_PROP_NAME:
      g_value_set_string (value, g_app_info_get_display_name (item->app_info));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_app_item_finalize (GObject *object)
{
  BobguiAppItem *item = BOBGUI_APP_ITEM (object);

  g_object_unref (item->app_info);

  G_OBJECT_CLASS (bobgui_app_item_parent_class)->finalize (object);
}

static void
bobgui_app_item_class_init (BobguiAppItemClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bobgui_app_item_get_property;
  object_class->finalize = bobgui_app_item_finalize;

  item_properties[ITEM_PROP_NAME] = g_param_spec_string ("name", NULL, NULL,
                                                         NULL,
                                                         G_PARAM_READABLE);

  g_object_class_install_properties (object_class, NUM_ITEM_PROPS, item_properties);
}

static BobguiAppItem *
bobgui_app_item_new (GAppInfo *app_info,
                  gboolean is_default,
                  gboolean is_recommended,
                  gboolean is_fallback)
{
  BobguiAppItem *item;

  item = g_object_new (bobgui_app_item_get_type (), NULL);

  item->app_info = g_object_ref (app_info);
  item->is_default = is_default;
  item->is_recommended = is_recommended;
  item->is_fallback = is_fallback;

  return item;
}

typedef struct _BobguiAppChooserWidgetClass   BobguiAppChooserWidgetClass;

struct _BobguiAppChooserWidget {
  BobguiWidget parent_instance;

  GAppInfo *selected_app_info;

  BobguiWidget *overlay;

  char *content_type;
  char *default_text;

  guint show_default     : 1;
  guint show_recommended : 1;
  guint show_fallback    : 1;
  guint show_other       : 1;
  guint show_all         : 1;

  GListStore *app_info_store;
  BobguiListItemFactory *header_factory;
  BobguiStringFilter *filter;
  BobguiStringSorter *sorter;
  BobguiWidget *program_list;
  BobguiWidget *no_apps_label;
  BobguiWidget *no_apps;

  GAppInfoMonitor *monitor;

  BobguiWidget *popup_menu;
};

struct _BobguiAppChooserWidgetClass {
  BobguiWidgetClass parent_class;

  void (* application_selected)  (BobguiAppChooserWidget *self,
                                  GAppInfo            *app_info);

  void (* application_activated) (BobguiAppChooserWidget *self,
                                  GAppInfo            *app_info);
};

enum {
  PROP_CONTENT_TYPE = 1,
  PROP_GFILE,
  PROP_SHOW_DEFAULT,
  PROP_SHOW_RECOMMENDED,
  PROP_SHOW_FALLBACK,
  PROP_SHOW_OTHER,
  PROP_SHOW_ALL,
  PROP_DEFAULT_TEXT,
  N_PROPERTIES
};

enum {
  SIGNAL_APPLICATION_SELECTED,
  SIGNAL_APPLICATION_ACTIVATED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static void bobgui_app_chooser_widget_iface_init (BobguiAppChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiAppChooserWidget, bobgui_app_chooser_widget, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_APP_CHOOSER,
                                                bobgui_app_chooser_widget_iface_init));

static gboolean
bobgui_app_chooser_widget_add_section (BobguiAppChooserWidget *self,
                                    gboolean             recommended,
                                    gboolean             fallback,
                                    GList               *applications,
                                    GList               *exclude_apps)
{
  gboolean retval;

  retval = FALSE;

  for (GList *l = applications; l != NULL; l = l->next)
    {
      GAppInfo *app = l->data;
      BobguiAppItem *item;

      if (self->content_type != NULL &&
          !g_app_info_supports_uris (app) &&
          !g_app_info_supports_files (app))
        continue;

      if (g_list_find (exclude_apps, app))
        continue;

      item = bobgui_app_item_new (app, FALSE, recommended, fallback);
      g_list_store_append (self->app_info_store, item);
      g_object_unref (item);
      retval = TRUE;
    }

  return retval;
}

static void
bobgui_app_chooser_add_default (BobguiAppChooserWidget *self,
                             GAppInfo            *app)
{
  BobguiAppItem *item;

  item = bobgui_app_item_new (app, TRUE, FALSE, FALSE);
  g_list_store_append (self->app_info_store, item);
  g_object_unref (item);
}

static void
update_no_applications_label (BobguiAppChooserWidget *self)
{
  char *text = NULL, *desc = NULL;
  const char *string;

  if (self->default_text == NULL)
    {
      if (self->content_type)
        desc = g_content_type_get_description (self->content_type);

      string = text = g_strdup_printf (_("No apps found for “%s”."), desc);
      g_free (desc);
    }
  else
    {
      string = self->default_text;
    }

  bobgui_label_set_text (BOBGUI_LABEL (self->no_apps_label), string);

  g_free (text);
}

static void
bobgui_app_chooser_widget_select_first (BobguiAppChooserWidget *self)
{
  bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (bobgui_list_view_get_model (BOBGUI_LIST_VIEW (self->program_list))), 0);
}

static void
bobgui_app_chooser_widget_real_add_items (BobguiAppChooserWidget *self)
{
  GList *all_applications = NULL;
  GList *recommended_apps = NULL;
  GList *fallback_apps = NULL;
  GList *exclude_apps = NULL;
  GAppInfo *default_app = NULL;
  gboolean show_headings;
  gboolean apps_added;

  show_headings = TRUE;
  apps_added = FALSE;

  if (self->show_all)
    show_headings = FALSE;

  bobgui_list_view_set_header_factory (BOBGUI_LIST_VIEW (self->program_list),
                                    show_headings ? self->header_factory : NULL);

  if (self->show_default && self->content_type)
    {
      default_app = g_app_info_get_default_for_type (self->content_type, FALSE);

      if (default_app != NULL)
        {
          bobgui_app_chooser_add_default (self, default_app);
          apps_added = TRUE;
          exclude_apps = g_list_prepend (exclude_apps, default_app);
        }
    }

#ifndef G_OS_WIN32
  if ((self->content_type && self->show_recommended) || self->show_all)
    {
      if (self->content_type)
	recommended_apps = g_app_info_get_recommended_for_type (self->content_type);

      apps_added |= bobgui_app_chooser_widget_add_section (self,
                                                        !self->show_all, /* mark as recommended */
                                                        FALSE, /* mark as fallback */
                                                        recommended_apps, exclude_apps);

      exclude_apps = g_list_concat (exclude_apps,
                                    g_list_copy (recommended_apps));
    }

  if ((self->content_type && self->show_fallback) || self->show_all)
    {
      if (self->content_type)
	fallback_apps = g_app_info_get_fallback_for_type (self->content_type);

      apps_added |= bobgui_app_chooser_widget_add_section (self,
                                                        FALSE, /* mark as recommended */
                                                        !self->show_all, /* mark as fallback */
                                                        fallback_apps, exclude_apps);
      exclude_apps = g_list_concat (exclude_apps,
                                    g_list_copy (fallback_apps));
    }
#endif

  if (self->show_other || self->show_all)
    {
      all_applications = g_app_info_get_all ();

      apps_added |= bobgui_app_chooser_widget_add_section (self,
                                                        FALSE,
                                                        FALSE,
                                                        all_applications, exclude_apps);
    }

  if (!apps_added)
    update_no_applications_label (self);

  bobgui_widget_set_visible (self->no_apps, !apps_added);

  bobgui_app_chooser_widget_select_first (self);

  if (default_app != NULL)
    g_object_unref (default_app);

  g_list_free_full (all_applications, g_object_unref);
  g_list_free_full (recommended_apps, g_object_unref);
  g_list_free_full (fallback_apps, g_object_unref);
  g_list_free (exclude_apps);
}

static void
bobgui_app_chooser_widget_initialize_items (BobguiAppChooserWidget *self)
{
  bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
}

static void
app_info_changed (GAppInfoMonitor     *monitor,
                  BobguiAppChooserWidget *self)
{
  bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
}

static void
bobgui_app_chooser_widget_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      self->content_type = g_value_dup_string (value);
      break;
    case PROP_SHOW_DEFAULT:
      bobgui_app_chooser_widget_set_show_default (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_RECOMMENDED:
      bobgui_app_chooser_widget_set_show_recommended (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_FALLBACK:
      bobgui_app_chooser_widget_set_show_fallback (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_OTHER:
      bobgui_app_chooser_widget_set_show_other (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_ALL:
      bobgui_app_chooser_widget_set_show_all (self, g_value_get_boolean (value));
      break;
    case PROP_DEFAULT_TEXT:
      bobgui_app_chooser_widget_set_default_text (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_app_chooser_widget_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, self->content_type);
      break;
    case PROP_SHOW_DEFAULT:
      g_value_set_boolean (value, self->show_default);
      break;
    case PROP_SHOW_RECOMMENDED:
      g_value_set_boolean (value, self->show_recommended);
      break;
    case PROP_SHOW_FALLBACK:
      g_value_set_boolean (value, self->show_fallback);
      break;
    case PROP_SHOW_OTHER:
      g_value_set_boolean (value, self->show_other);
      break;
    case PROP_SHOW_ALL:
      g_value_set_boolean (value, self->show_all);
      break;
    case PROP_DEFAULT_TEXT:
      g_value_set_string (value, self->default_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_app_chooser_widget_constructed (GObject *object)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  if (G_OBJECT_CLASS (bobgui_app_chooser_widget_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (bobgui_app_chooser_widget_parent_class)->constructed (object);

  bobgui_app_chooser_widget_initialize_items (self);
}

static void
bobgui_app_chooser_widget_finalize (GObject *object)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  g_free (self->content_type);
  g_free (self->default_text);
  g_signal_handlers_disconnect_by_func (self->monitor, app_info_changed, self);
  g_object_unref (self->monitor);
  g_object_unref (self->app_info_store);
  g_object_unref (self->header_factory);
  g_object_unref (self->filter);
  g_object_unref (self->sorter);

  G_OBJECT_CLASS (bobgui_app_chooser_widget_parent_class)->finalize (object);
}

static void
bobgui_app_chooser_widget_dispose (GObject *object)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  g_clear_object (&self->selected_app_info);

  if (self->overlay)
    {
      bobgui_widget_unparent (self->overlay);
      self->overlay = NULL;
    }

  G_OBJECT_CLASS (bobgui_app_chooser_widget_parent_class)->dispose (object);
}

static void
bobgui_app_chooser_widget_measure (BobguiWidget       *widget,
                                BobguiOrientation  orientation,
                                int             for_size,
                                int            *minimum,
                                int            *natural,
                                int            *minimum_baseline,
                                int            *natural_baseline)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (widget);

  bobgui_widget_measure (self->overlay, orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_app_chooser_widget_snapshot (BobguiWidget   *widget,
                                 BobguiSnapshot *snapshot)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (widget);

  bobgui_widget_snapshot_child (widget, self->overlay, snapshot);
}

static void
bobgui_app_chooser_widget_size_allocate (BobguiWidget *widget,
                                      int        width,
                                      int        height,
                                      int        baseline)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (widget);

  BOBGUI_WIDGET_CLASS (bobgui_app_chooser_widget_parent_class)->size_allocate (widget, width, height, baseline);

  bobgui_widget_size_allocate (self->overlay,
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            },baseline);
}

static void
bobgui_app_chooser_widget_class_init (BobguiAppChooserWidgetClass *klass)
{
  BobguiWidgetClass *widget_class;
  GObjectClass *gobject_class;
  GParamSpec *pspec;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = bobgui_app_chooser_widget_dispose;
  gobject_class->finalize = bobgui_app_chooser_widget_finalize;
  gobject_class->set_property = bobgui_app_chooser_widget_set_property;
  gobject_class->get_property = bobgui_app_chooser_widget_get_property;
  gobject_class->constructed = bobgui_app_chooser_widget_constructed;

  widget_class = BOBGUI_WIDGET_CLASS (klass);
  widget_class->measure = bobgui_app_chooser_widget_measure;
  widget_class->size_allocate = bobgui_app_chooser_widget_size_allocate;
  widget_class->snapshot = bobgui_app_chooser_widget_snapshot;

  g_object_class_override_property (gobject_class, PROP_CONTENT_TYPE, "content-type");

  /**
   * BobguiAppChooserWidget:show-default:
   *
   * Determines whether the app chooser should show the default
   * handler for the content type in a separate section.
   *
   * If %FALSE, the default handler is listed among the recommended
   * applications.
   */
  pspec = g_param_spec_boolean ("show-default", NULL, NULL,
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_DEFAULT, pspec);

  /**
   * BobguiAppChooserWidget:show-recommended:
   *
   * Determines whether the app chooser should show a section
   * for recommended applications.
   *
   * If %FALSE, the recommended applications are listed
   * among the other applications.
   */
  pspec = g_param_spec_boolean ("show-recommended", NULL, NULL,
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_RECOMMENDED, pspec);

  /**
   * BobguiAppChooserWidget:show-fallback:
   *
   * Determines whether the app chooser should show a section
   * for fallback applications.
   *
   * If %FALSE, the fallback applications are listed among the
   * other applications.
   */
  pspec = g_param_spec_boolean ("show-fallback", NULL, NULL,
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_FALLBACK, pspec);

  /**
   * BobguiAppChooserWidget:show-other:
   *
   * Determines whether the app chooser should show a section
   * for other applications.
   */
  pspec = g_param_spec_boolean ("show-other", NULL, NULL,
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_OTHER, pspec);

  /**
   * BobguiAppChooserWidget:show-all:
   *
   * If %TRUE, the app chooser presents all applications
   * in a single list, without subsections for default,
   * recommended or related applications.
   */
  pspec = g_param_spec_boolean ("show-all", NULL, NULL,
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_ALL, pspec);

  /**
   * BobguiAppChooserWidget:default-text:
   *
   * The text that appears in the widget when there are no applications
   * for the given content type.
   */
  pspec = g_param_spec_string ("default-text", NULL, NULL,
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_DEFAULT_TEXT, pspec);

  /**
   * BobguiAppChooserWidget::application-selected:
   * @self: the object which received the signal
   * @application: the selected `GAppInfo`
   *
   * Emitted when an application item is selected from the widget's list.
   */
  signals[SIGNAL_APPLICATION_SELECTED] =
    g_signal_new (I_("application-selected"),
                  BOBGUI_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiAppChooserWidgetClass, application_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_APP_INFO);

  /**
   * BobguiAppChooserWidget::application-activated:
   * @self: the object which received the signal
   * @application: the activated `GAppInfo`
   *
   * Emitted when an application item is activated from the widget's list.
   *
   * This usually happens when the user double clicks an item, or an item
   * is selected and the user presses one of the keys Space, Shift+Space,
   * Return or Enter.
   */
  signals[SIGNAL_APPLICATION_ACTIVATED] =
    g_signal_new (I_("application-activated"),
                  BOBGUI_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiAppChooserWidgetClass, application_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_APP_INFO);

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/ui/bobguiappchooserwidget.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiAppChooserWidget, program_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAppChooserWidget, no_apps_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAppChooserWidget, no_apps);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAppChooserWidget, overlay);

  bobgui_widget_class_set_css_name (widget_class, I_("appchooser"));
}

static void
setup_listitem_cb (BobguiListItemFactory *factory,
                   BobguiListItem        *list_item)
{
  BobguiWidget *box;
  BobguiWidget *image;
  BobguiWidget *label;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  image = bobgui_image_new ();
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (image),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                  "App icon",
                                  -1);
  bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
  bobgui_box_append (BOBGUI_BOX (box), image);
  label = bobgui_label_new ("");
  bobgui_box_append (BOBGUI_BOX (box), label);
  bobgui_list_item_set_child (list_item, box);
}

static void
bind_listitem_cb (BobguiListItemFactory *factory,
                  BobguiListItem        *list_item)
{
  BobguiWidget *image;
  BobguiWidget *label;
  BobguiAppItem *app_item;

  image = bobgui_widget_get_first_child (bobgui_list_item_get_child (list_item));
  label = bobgui_widget_get_next_sibling (image);
  app_item = bobgui_list_item_get_item (list_item);

  bobgui_image_set_from_gicon (BOBGUI_IMAGE (image), g_app_info_get_icon (app_item->app_info));
  bobgui_label_set_label (BOBGUI_LABEL (label), g_app_info_get_display_name (app_item->app_info));
  bobgui_list_item_set_accessible_label (list_item, g_app_info_get_display_name (app_item->app_info));
}

static void
setup_header_cb (BobguiListItemFactory *factory,
                 BobguiListItem        *list_item)
{
  BobguiListHeader *header = BOBGUI_LIST_HEADER (list_item);
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_widget_add_css_class (label, "heading");
  bobgui_widget_set_margin_start (label, 20);
  bobgui_widget_set_margin_end (label, 20);
  bobgui_widget_set_margin_top (label, 10);
  bobgui_widget_set_margin_bottom (label, 10);

  bobgui_list_header_set_child (header, label);
}

static void
bind_header_cb (BobguiListItemFactory *factory,
                BobguiListItem        *list_item)
{
  BobguiListHeader *header = BOBGUI_LIST_HEADER (list_item);
  BobguiWidget *label;
  BobguiAppItem *app_item;

  label = bobgui_list_header_get_child (header);
  app_item = bobgui_list_header_get_item (header);

  if (app_item->is_default)
    bobgui_label_set_label (BOBGUI_LABEL (label), _("Default App"));
  else if (app_item->is_recommended)
    bobgui_label_set_label (BOBGUI_LABEL (label), _("Recommended Apps"));
  else if (app_item->is_fallback)
    bobgui_label_set_label (BOBGUI_LABEL (label), _("Related Apps"));
  else
    bobgui_label_set_label (BOBGUI_LABEL (label), _("Other Apps"));
}

static void
activate_cb (BobguiListView         *list,
             guint                position,
             BobguiAppChooserWidget *self)
{
  BobguiAppItem *app_item;

  app_item = g_list_model_get_item (G_LIST_MODEL (bobgui_list_view_get_model (list)), position);

  g_set_object (&self->selected_app_info, app_item->app_info);

  g_signal_emit (self, signals[SIGNAL_APPLICATION_ACTIVATED], 0, self->selected_app_info);

  g_object_unref (app_item);
}

static void
selection_changed_cb (GListModel          *model,
                      GParamSpec          *pspec,
                      BobguiAppChooserWidget *self)
{
  guint position;
  BobguiAppItem *app_item;

  position = bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (model));
  if (position == BOBGUI_INVALID_LIST_POSITION)
    return;

  app_item = g_list_model_get_item (model, position);

  g_set_object (&self->selected_app_info, app_item->app_info);

  g_signal_emit (self, signals[SIGNAL_APPLICATION_SELECTED], 0, self->selected_app_info);

  g_object_unref (app_item);
}

static int
compare_section (gconstpointer a,
                 gconstpointer b,
                 gpointer      data)
{
  const BobguiAppItem *item1 = a;
  const BobguiAppItem *item2 = b;

  if (item1->is_default && !item2->is_default)
    return -1;
  else if (!item1->is_default && item2->is_default)
    return 1;

  if (item1->is_recommended && !item2->is_recommended)
    return -1;
  else if (!item1->is_recommended && item2->is_recommended)
    return 1;

  if (item1->is_fallback && !item2->is_fallback)
    return 1;
  else if (!item1->is_fallback && item2->is_fallback)
    return -1;

  return 0;
}

static void
bobgui_app_chooser_widget_init (BobguiAppChooserWidget *self)
{
  BobguiListItemFactory *factory;
  BobguiSingleSelection *selection;
  BobguiExpression *expression;
  BobguiFilterListModel *filter;
  BobguiSortListModel *sort;
  BobguiCustomSorter *section_sorter;

  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  expression = bobgui_property_expression_new (bobgui_app_item_get_type (), NULL, "name");
  self->filter = bobgui_string_filter_new (bobgui_expression_ref (expression));
  self->sorter = bobgui_string_sorter_new (expression);

  self->app_info_store = g_list_store_new (bobgui_app_item_get_type ());
  filter = bobgui_filter_list_model_new (G_LIST_MODEL (g_object_ref (self->app_info_store)), BOBGUI_FILTER (g_object_ref (self->filter)));
  sort = bobgui_sort_list_model_new (G_LIST_MODEL (filter), BOBGUI_SORTER (g_object_ref (self->sorter)));

  section_sorter = bobgui_custom_sorter_new (compare_section, NULL, NULL);
  bobgui_sort_list_model_set_section_sorter (sort, BOBGUI_SORTER (section_sorter));
  g_object_unref (section_sorter);

  selection = bobgui_single_selection_new (G_LIST_MODEL (sort));

  g_signal_connect (selection, "notify::selected",
                    G_CALLBACK (selection_changed_cb), self);

  bobgui_list_view_set_model (BOBGUI_LIST_VIEW (self->program_list), BOBGUI_SELECTION_MODEL (selection));
  g_object_unref (selection);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem_cb), NULL);

  bobgui_list_view_set_factory (BOBGUI_LIST_VIEW (self->program_list), factory);
  g_object_unref (factory);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_header_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_header_cb), NULL);

  bobgui_list_view_set_header_factory (BOBGUI_LIST_VIEW (self->program_list), factory);
  self->header_factory = factory;

  g_signal_connect (self->program_list, "activate",
                    G_CALLBACK (activate_cb), self);

  self->monitor = g_app_info_monitor_get ();
  g_signal_connect (self->monitor, "changed",
                    G_CALLBACK (app_info_changed), self);
}

static GAppInfo *
bobgui_app_chooser_widget_get_app_info (BobguiAppChooser *object)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  if (self->selected_app_info == NULL)
    return NULL;

  return g_object_ref (self->selected_app_info);
}

static void
bobgui_app_chooser_widget_refresh (BobguiAppChooser *object)
{
  BobguiAppChooserWidget *self = BOBGUI_APP_CHOOSER_WIDGET (object);

  g_list_store_remove_all (self->app_info_store);
  bobgui_app_chooser_widget_real_add_items (self);
}

static void
bobgui_app_chooser_widget_iface_init (BobguiAppChooserIface *iface)
{
  iface->get_app_info = bobgui_app_chooser_widget_get_app_info;
  iface->refresh = bobgui_app_chooser_widget_refresh;
}

/**
 * bobgui_app_chooser_widget_new:
 * @content_type: the content type to show applications for
 *
 * Creates a new `BobguiAppChooserWidget` for applications
 * that can handle content of the given type.
 *
 * Returns: a newly created `BobguiAppChooserWidget`
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget *
bobgui_app_chooser_widget_new (const char *content_type)
{
  return g_object_new (BOBGUI_TYPE_APP_CHOOSER_WIDGET,
                       "content-type", content_type,
                       NULL);
}

/**
 * bobgui_app_chooser_widget_set_show_default:
 * @self: a `BobguiAppChooserWidget`
 * @setting: the new value for [property@Bobgui.AppChooserWidget:show-default]
 *
 * Sets whether the app chooser should show the default handler
 * for the content type in a separate section.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_show_default (BobguiAppChooserWidget *self,
                                         gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (self->show_default != setting)
    {
      self->show_default = setting;

      g_object_notify (G_OBJECT (self), "show-default");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_show_default:
 * @self: a `BobguiAppChooserWidget`
 *
 * Gets whether the app chooser should show the default handler
 * for the content type in a separate section.
 *
 * Returns: the value of [property@Bobgui.AppChooserWidget:show-default]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_widget_get_show_default (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->show_default;
}

/**
 * bobgui_app_chooser_widget_set_show_recommended:
 * @self: a `BobguiAppChooserWidget`
 * @setting: the new value for [property@Bobgui.AppChooserWidget:show-recommended]
 *
 * Sets whether the app chooser should show recommended applications
 * for the content type in a separate section.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_show_recommended (BobguiAppChooserWidget *self,
                                             gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (self->show_recommended != setting)
    {
      self->show_recommended = setting;

      g_object_notify (G_OBJECT (self), "show-recommended");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_show_recommended:
 * @self: a `BobguiAppChooserWidget`
 *
 * Gets whether the app chooser should show recommended applications
 * for the content type in a separate section.
 *
 * Returns: the value of [property@Bobgui.AppChooserWidget:show-recommended]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_widget_get_show_recommended (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->show_recommended;
}

/**
 * bobgui_app_chooser_widget_set_show_fallback:
 * @self: a `BobguiAppChooserWidget`
 * @setting: the new value for [property@Bobgui.AppChooserWidget:show-fallback]
 *
 * Sets whether the app chooser should show related applications
 * for the content type in a separate section.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_show_fallback (BobguiAppChooserWidget *self,
                                          gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (self->show_fallback != setting)
    {
      self->show_fallback = setting;

      g_object_notify (G_OBJECT (self), "show-fallback");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_show_fallback:
 * @self: a `BobguiAppChooserWidget`
 *
 * Gets whether the app chooser should show related applications
 * for the content type in a separate section.
 *
 * Returns: the value of [property@Bobgui.AppChooserWidget:show-fallback]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_widget_get_show_fallback (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->show_fallback;
}

/**
 * bobgui_app_chooser_widget_set_show_other:
 * @self: a `BobguiAppChooserWidget`
 * @setting: the new value for [property@Bobgui.AppChooserWidget:show-other]
 *
 * Sets whether the app chooser should show applications
 * which are unrelated to the content type.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_show_other (BobguiAppChooserWidget *self,
                                       gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (self->show_other != setting)
    {
      self->show_other = setting;

      g_object_notify (G_OBJECT (self), "show-other");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_show_other:
 * @self: a `BobguiAppChooserWidget`
 *
 * Gets whether the app chooser should show applications
 * which are unrelated to the content type.
 *
 * Returns: the value of [property@Bobgui.AppChooserWidget:show-other]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_widget_get_show_other (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->show_other;
}

/**
 * bobgui_app_chooser_widget_set_show_all:
 * @self: a `BobguiAppChooserWidget`
 * @setting: the new value for [property@Bobgui.AppChooserWidget:show-all]
 *
 * Sets whether the app chooser should show all applications
 * in a flat list.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_show_all (BobguiAppChooserWidget *self,
                                     gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (self->show_all != setting)
    {
      self->show_all = setting;

      g_object_notify (G_OBJECT (self), "show-all");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_show_all:
 * @self: a `BobguiAppChooserWidget`
 *
 * Gets whether the app chooser should show all applications
 * in a flat list.
 *
 * Returns: the value of [property@Bobgui.AppChooserWidget:show-all]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_widget_get_show_all (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->show_all;
}

/**
 * bobgui_app_chooser_widget_set_default_text:
 * @self: a `BobguiAppChooserWidget`
 * @text: the new value for [property@Bobgui.AppChooserWidget:default-text]
 *
 * Sets the text that is shown if there are not applications
 * that can handle the content type.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_widget_set_default_text (BobguiAppChooserWidget *self,
                                         const char          *text)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self));

  if (g_strcmp0 (text, self->default_text) != 0)
    {
      g_free (self->default_text);
      self->default_text = g_strdup (text);

      g_object_notify (G_OBJECT (self), "default-text");

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_widget_get_default_text:
 * @self: a `BobguiAppChooserWidget`
 *
 * Returns the text that is shown if there are not applications
 * that can handle the content type.
 *
 * Returns: (nullable): the value of [property@Bobgui.AppChooserWidget:default-text]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
const char *
bobgui_app_chooser_widget_get_default_text (BobguiAppChooserWidget *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_WIDGET (self), NULL);

  return self->default_text;
}

static void
changed_cb (BobguiEditable         *editable,
            BobguiAppChooserWidget *self)
{
  bobgui_string_filter_set_search (self->filter, bobgui_editable_get_text (editable));
}

void
_bobgui_app_chooser_widget_set_search_entry (BobguiAppChooserWidget *self,
                                          BobguiEditable         *entry)
{
  g_object_bind_property (self->no_apps, "visible",
                          entry, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect (entry, "changed", G_CALLBACK (changed_cb), self);
}
