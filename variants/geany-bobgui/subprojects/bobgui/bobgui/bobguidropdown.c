/*
 * Copyright © 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguidropdown.h"

#include "bobguibuiltiniconprivate.h"
#include "bobguilistview.h"
#include "bobguilistitemfactory.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitemwidgetprivate.h"
#include "bobguipopover.h"
#include "bobguiprivate.h"
#include "bobguisingleselection.h"
#include "bobguifilterlistmodel.h"
#include "bobguistringfilter.h"
#include "bobguimultifilter.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "bobguitogglebutton.h"
#include "bobguiexpression.h"
#include "bobguistack.h"
#include "bobguisearchentry.h"
#include "bobguilabel.h"
#include "bobguilistitem.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguistringlist.h"
#include "bobguibox.h"
#include "bobguitypebuiltins.h"

/**
 * BobguiDropDown:
 *
 * Allows the user to choose an item from a list of options.
 *
 * <picture>
 *   <source srcset="drop-down-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiDropDown" src="drop-down.png">
 * </picture>
 *
 * The `BobguiDropDown` displays the [selected][property@Bobgui.DropDown:selected]
 * choice.
 *
 * The options are given to `BobguiDropDown` in the form of `GListModel`
 * and how the individual options are represented is determined by
 * a [class@Bobgui.ListItemFactory]. The default factory displays simple strings,
 * and adds a checkmark to the selected item in the popup.
 *
 * To set your own factory, use [method@Bobgui.DropDown.set_factory]. It is
 * possible to use a separate factory for the items in the popup, with
 * [method@Bobgui.DropDown.set_list_factory].
 *
 * `BobguiDropDown` knows how to obtain strings from the items in a
 * [class@Bobgui.StringList]; for other models, you have to provide an expression
 * to find the strings via [method@Bobgui.DropDown.set_expression].
 *
 * `BobguiDropDown` can optionally allow search in the popup, which is
 * useful if the list of options is long. To enable the search entry,
 * use [method@Bobgui.DropDown.set_enable_search].
 *
 * Here is a UI definition example for `BobguiDropDown` with a simple model:
 *
 * ```xml
 * <object class="BobguiDropDown">
 *   <property name="model">
 *     <object class="BobguiStringList">
 *       <items>
 *         <item translatable="yes">Factory</item>
 *         <item translatable="yes">Home</item>
 *         <item translatable="yes">Subway</item>
 *       </items>
 *     </object>
 *   </property>
 * </object>
 * ```
 *
 * If a `BobguiDropDown` is created in this manner, or with
 * [ctor@Bobgui.DropDown.new_from_strings], for instance, the object returned from
 * [method@Bobgui.DropDown.get_selected_item] will be a [class@Bobgui.StringObject].
 *
 * To learn more about the list widget framework, see the
 * [overview](section-list-widget.html).
 *
 * ## CSS nodes
 *
 * `BobguiDropDown` has a single CSS node with name dropdown,
 * with the button and popover nodes as children.
 *
 * ## Accessibility
 *
 * `BobguiDropDown` uses the [enum@Bobgui.AccessibleRole.combo_box] role.
 */

struct _BobguiDropDown
{
  BobguiWidget parent_instance;

  gboolean uses_default_factory;
  gboolean uses_default_list_factory;
  BobguiListItemFactory *factory;
  BobguiListItemFactory *list_factory;
  BobguiListItemFactory *header_factory;
  GListModel *model;
  BobguiSelectionModel *selection;
  GListModel *filter_model;
  BobguiSelectionModel *popup_selection;

  BobguiWidget *popup;
  BobguiWidget *button;
  BobguiWidget *arrow;

  BobguiWidget *popup_list;
  BobguiWidget *button_stack;
  BobguiWidget *button_item;
  BobguiWidget *button_placeholder;
  BobguiWidget *search_box;
  BobguiWidget *search_entry;

  BobguiExpression *expression;

  BobguiStringFilterMatchMode search_match_mode;

  guint enable_search : 1;
  guint show_arrow : 1;
};

struct _BobguiDropDownClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_0,
  PROP_FACTORY,
  PROP_HEADER_FACTORY,
  PROP_LIST_FACTORY,
  PROP_MODEL,
  PROP_SELECTED,
  PROP_SELECTED_ITEM,
  PROP_ENABLE_SEARCH,
  PROP_EXPRESSION,
  PROP_SHOW_ARROW,
  PROP_SEARCH_MATCH_MODE,

  N_PROPS
};

enum
{
  ACTIVATE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (BobguiDropDown, bobgui_drop_down, BOBGUI_TYPE_WIDGET)

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void
button_toggled (BobguiWidget *widget,
                gpointer   data)
{
  BobguiDropDown *self = data;

  if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)))
    bobgui_popover_popup (BOBGUI_POPOVER (self->popup));
  else
    bobgui_popover_popdown (BOBGUI_POPOVER (self->popup));

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                               BOBGUI_ACCESSIBLE_STATE_EXPANDED, bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)),
                               -1);
}

static void
popover_closed (BobguiPopover *popover,
                gpointer    data)
{
  BobguiDropDown *self = data;

  bobgui_editable_set_text (BOBGUI_EDITABLE (self->search_entry), "");
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (self->button), FALSE);
}

static void
row_activated (BobguiListView *listview,
               guint        position,
               gpointer     data)
{
  BobguiDropDown *self = data;
  BobguiFilter *filter;

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (self->button), FALSE);
  bobgui_popover_popdown (BOBGUI_POPOVER (self->popup));

  /* reset the filter so positions are 1-1 */
  filter = bobgui_filter_list_model_get_filter (BOBGUI_FILTER_LIST_MODEL (self->filter_model));
  if (BOBGUI_IS_STRING_FILTER (filter))
    bobgui_string_filter_set_search (BOBGUI_STRING_FILTER (filter), "");
  bobgui_drop_down_set_selected (self, bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->popup_selection)));
}

static void
selection_changed (BobguiSingleSelection *selection,
                   GParamSpec         *pspec,
                   gpointer            data)
{
  BobguiDropDown *self = data;
  guint selected;
  BobguiFilter *filter;

  selected = bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->selection));

  bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT);

  if (selected == BOBGUI_INVALID_LIST_POSITION)
    {
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->button_stack), "empty");
    }
  else
    {
      GObject *item;

      item = bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (self->selection));

      if (self->expression)
        {
          GValue value = G_VALUE_INIT;

          if (bobgui_expression_evaluate (self->expression, item, &value))
            bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                            BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT, g_value_get_string (&value),
                                            -1);

          g_value_unset (&value);
        }
      else if (BOBGUI_IS_STRING_OBJECT (item))
        {
          const char *string;

          string = bobgui_string_object_get_string (BOBGUI_STRING_OBJECT (item));

          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                          BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT, string,
                                            -1);
        }

      bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->button_stack), "item");
    }

  if (selected != bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self->button_item)))
    {
      bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (self->button_item),
                                 selected,
                                 bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (self->selection)),
                                 FALSE);
    }

  /* reset the filter so positions are 1-1 */
  filter = bobgui_filter_list_model_get_filter (BOBGUI_FILTER_LIST_MODEL (self->filter_model));
  if (BOBGUI_IS_STRING_FILTER (filter))
    bobgui_string_filter_set_search (BOBGUI_STRING_FILTER (filter), "");
  bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (self->popup_selection), selected);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
}

static void
selection_item_changed (BobguiSingleSelection *selection,
                        GParamSpec         *pspec,
                        gpointer            data)
{
  BobguiDropDown *self = data;
  gpointer item;

  item = bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (self->selection));

  if (item != bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self->button_item)))
    {
      bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (self->button_item),
                                 bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->selection)),
                                 item,
                                 FALSE);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED_ITEM]);
}

static void
bobgui_drop_down_activate (BobguiDropDown *self)
{
  bobgui_widget_activate (self->button);
}

static void
update_filter (BobguiDropDown *self)
{
  if (self->filter_model)
    {
      BobguiFilter *filter;

      if (self->expression)
        {
          filter = BOBGUI_FILTER (bobgui_string_filter_new (bobgui_expression_ref (self->expression)));
          bobgui_string_filter_set_match_mode (BOBGUI_STRING_FILTER (filter), self->search_match_mode);
        }
      else
        filter = BOBGUI_FILTER (bobgui_every_filter_new ());
      bobgui_filter_list_model_set_filter (BOBGUI_FILTER_LIST_MODEL (self->filter_model), filter);
      g_object_unref (filter);
    }
}

static void
search_changed (BobguiSearchEntry *entry, gpointer data)
{
  BobguiDropDown *self = data;
  const char *text;
  BobguiFilter *filter;

  text  = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  filter = bobgui_filter_list_model_get_filter (BOBGUI_FILTER_LIST_MODEL (self->filter_model));
  if (BOBGUI_IS_STRING_FILTER (filter))
    bobgui_string_filter_set_search (BOBGUI_STRING_FILTER (filter), text);
}

static void
search_stop (BobguiSearchEntry *entry, gpointer data)
{
  BobguiDropDown *self = data;
  BobguiFilter *filter;

  filter = bobgui_filter_list_model_get_filter (BOBGUI_FILTER_LIST_MODEL (self->filter_model));
  if (BOBGUI_IS_STRING_FILTER (filter))
    {
      if (bobgui_string_filter_get_search (BOBGUI_STRING_FILTER (filter)))
        bobgui_string_filter_set_search (BOBGUI_STRING_FILTER (filter), NULL);
      else
        bobgui_popover_popdown (BOBGUI_POPOVER (self->popup));
    }
}

static void
bobgui_drop_down_dispose (GObject *object)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (object);

  g_clear_pointer (&self->popup, bobgui_widget_unparent);
  g_clear_pointer (&self->button, bobgui_widget_unparent);

  g_clear_object (&self->model);
  if (self->selection)
    {
      g_signal_handlers_disconnect_by_func (self->selection, selection_changed, self);
      g_signal_handlers_disconnect_by_func (self->selection, selection_item_changed, self);
    }
  g_clear_object (&self->filter_model);
  g_clear_pointer (&self->expression, bobgui_expression_unref);
  g_clear_object (&self->selection);
  g_clear_object (&self->popup_selection);
  g_clear_object (&self->factory);
  g_clear_object (&self->list_factory);
  g_clear_object (&self->header_factory);

  G_OBJECT_CLASS (bobgui_drop_down_parent_class)->dispose (object);
}

static void
bobgui_drop_down_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_FACTORY:
      g_value_set_object (value, self->factory);
      break;

    case PROP_HEADER_FACTORY:
      g_value_set_object (value, self->header_factory);
      break;

    case PROP_LIST_FACTORY:
      g_value_set_object (value, self->list_factory);
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_SELECTED:
      g_value_set_uint (value, bobgui_drop_down_get_selected (self));
      break;

    case PROP_SELECTED_ITEM:
      g_value_set_object (value, bobgui_drop_down_get_selected_item (self));
      break;

    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, self->enable_search);
      break;

    case PROP_EXPRESSION:
      bobgui_value_set_expression (value, self->expression);
      break;

    case PROP_SHOW_ARROW:
      g_value_set_boolean (value, bobgui_drop_down_get_show_arrow (self));
      break;

    case PROP_SEARCH_MATCH_MODE:
      g_value_set_enum (value, bobgui_drop_down_get_search_match_mode (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_drop_down_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_FACTORY:
      bobgui_drop_down_set_factory (self, g_value_get_object (value));
      break;

    case PROP_HEADER_FACTORY:
      bobgui_drop_down_set_header_factory (self, g_value_get_object (value));
      break;

    case PROP_LIST_FACTORY:
      bobgui_drop_down_set_list_factory (self, g_value_get_object (value));
      break;

    case PROP_MODEL:
      bobgui_drop_down_set_model (self, g_value_get_object (value));
      break;

    case PROP_SELECTED:
      bobgui_drop_down_set_selected (self, g_value_get_uint (value));
      break;

    case PROP_ENABLE_SEARCH:
      bobgui_drop_down_set_enable_search (self, g_value_get_boolean (value));
      break;

    case PROP_EXPRESSION:
      bobgui_drop_down_set_expression (self, bobgui_value_get_expression (value));
      break;

    case PROP_SHOW_ARROW:
      bobgui_drop_down_set_show_arrow (self, g_value_get_boolean (value));
      break;

    case PROP_SEARCH_MATCH_MODE:
      bobgui_drop_down_set_search_match_mode (self, g_value_get_enum (value));
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_drop_down_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  bobgui_widget_measure (self->button,
                      orientation,
                      size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_drop_down_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  bobgui_widget_size_allocate (self->button, &(BobguiAllocation) { 0, 0, width, height }, baseline);

  bobgui_widget_set_size_request (self->popup, width, -1);
  bobgui_widget_queue_resize (self->popup);

  bobgui_popover_present (BOBGUI_POPOVER (self->popup));
}

static gboolean
bobgui_drop_down_focus (BobguiWidget        *widget,
                     BobguiDirectionType  direction)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  if (self->popup && bobgui_widget_get_visible (self->popup))
    return bobgui_widget_child_focus (self->popup, direction);
  else
    return bobgui_widget_child_focus (self->button, direction);
}

static gboolean
bobgui_drop_down_grab_focus (BobguiWidget *widget)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  return bobgui_widget_grab_focus (self->button);
}

static void
bobgui_drop_down_root (BobguiWidget *widget)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  BOBGUI_WIDGET_CLASS (bobgui_drop_down_parent_class)->root (widget);

  if (self->factory)
    bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (self->button_item), self->factory);
}

static void
bobgui_drop_down_unroot (BobguiWidget *widget)
{
  BobguiDropDown *self = BOBGUI_DROP_DOWN (widget);

  if (self->factory)
    bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (self->button_item), NULL);

  BOBGUI_WIDGET_CLASS (bobgui_drop_down_parent_class)->unroot (widget);
}

static void
bobgui_drop_down_class_init (BobguiDropDownClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_drop_down_dispose;
  gobject_class->get_property = bobgui_drop_down_get_property;
  gobject_class->set_property = bobgui_drop_down_set_property;

  widget_class->measure = bobgui_drop_down_measure;
  widget_class->size_allocate = bobgui_drop_down_size_allocate;
  widget_class->focus = bobgui_drop_down_focus;
  widget_class->grab_focus = bobgui_drop_down_grab_focus;
  widget_class->root = bobgui_drop_down_root;
  widget_class->unroot = bobgui_drop_down_unroot;

  /**
   * BobguiDropDown:factory:
   *
   * Factory for populating list items.
   */
  properties[PROP_FACTORY] =
    g_param_spec_object ("factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:header-factory:
   *
   * The factory for creating header widgets for the popup.
   *
   * Since: 4.12
   */
  properties[PROP_HEADER_FACTORY] =
    g_param_spec_object ("header-factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:list-factory:
   *
   * The factory for populating list items in the popup.
   *
   * If this is not set, [property@Bobgui.DropDown:factory] is used.
   */
  properties[PROP_LIST_FACTORY] =
    g_param_spec_object ("list-factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:model:
   *
   * Model for the displayed items.
   */
  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:selected:
   *
   * The position of the selected item.
   *
   * If no item is selected, the property has the value
   * %BOBGUI_INVALID_LIST_POSITION.
   */
  properties[PROP_SELECTED] =
    g_param_spec_uint ("selected", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:selected-item:
   *
   * The selected item.
   */
  properties[PROP_SELECTED_ITEM] =
    g_param_spec_object ("selected-item", NULL, NULL,
                         G_TYPE_OBJECT,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:enable-search:
   *
   * Whether to show a search entry in the popup.
   *
   * Note that search requires [property@Bobgui.DropDown:expression]
   * to be set.
   */
  properties[PROP_ENABLE_SEARCH] =
    g_param_spec_boolean  ("enable-search", NULL, NULL,
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:expression: (type BobguiExpression)
   *
   * An expression to evaluate to obtain strings to match against the search
   * term.
   *
   * See [property@Bobgui.DropDown:enable-search] for how to enable search.
   * If [property@Bobgui.DropDown:factory] is not set, the expression is also
   * used to bind strings to labels produced by a default factory.
   */
  properties[PROP_EXPRESSION] =
    bobgui_param_spec_expression ("expression", NULL, NULL,
                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:show-arrow:
   *
   * Whether to show an arrow within the BobguiDropDown widget.
   *
   * Since: 4.6
   */
  properties[PROP_SHOW_ARROW] =
    g_param_spec_boolean  ("show-arrow", NULL, NULL,
                           TRUE,
                           G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiDropDown:search-match-mode:
   *
   * The match mode for the search filter.
   *
   * Since: 4.12
   */
  properties[PROP_SEARCH_MATCH_MODE] =
    g_param_spec_enum  ("search-match-mode", NULL, NULL,
                           BOBGUI_TYPE_STRING_FILTER_MATCH_MODE,
                           BOBGUI_STRING_FILTER_MATCH_MODE_PREFIX,
                           G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  /**
   * BobguiDropDown::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the drop down is activated.
   *
   * The `::activate` signal on `BobguiDropDown` is an action signal and
   * emitting it causes the drop down to pop up its dropdown.
   *
   * Since: 4.6
   */
  signals[ACTIVATE] =
      g_signal_new_class_handler (I_ ("activate"),
                                  G_OBJECT_CLASS_TYPE (gobject_class),
                                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_CALLBACK (bobgui_drop_down_activate),
                                  NULL, NULL,
                                  NULL,
                                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE]);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguidropdown.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, arrow);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, button_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, button_item);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, popup);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, popup_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, search_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiDropDown, search_entry);

  bobgui_widget_class_bind_template_callback (widget_class, row_activated);
  bobgui_widget_class_bind_template_callback (widget_class, button_toggled);
  bobgui_widget_class_bind_template_callback (widget_class, popover_closed);
  bobgui_widget_class_bind_template_callback (widget_class, search_changed);
  bobgui_widget_class_bind_template_callback (widget_class, search_stop);

  bobgui_widget_class_set_css_name (widget_class, I_("dropdown"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX);
}

static void
setup_item (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item,
            gpointer                  data)
{
  BobguiWidget *box;
  BobguiWidget *label;
  BobguiWidget *icon;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_box_append (BOBGUI_BOX (box), label);
  icon = g_object_new (BOBGUI_TYPE_IMAGE,
                       "icon-name", "object-select-symbolic",
                       "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                       NULL);
  bobgui_box_append (BOBGUI_BOX (box), icon);
  bobgui_list_item_set_child (list_item, box);
}

static void
selected_item_changed (BobguiDropDown *self,
                       GParamSpec  *pspec,
                       BobguiListItem *list_item)
{
  BobguiWidget *box;
  BobguiWidget *icon;

  box = bobgui_list_item_get_child (list_item);
  icon = bobgui_widget_get_last_child (box);

  if (bobgui_drop_down_get_selected_item (self) == bobgui_list_item_get_item (list_item))
    bobgui_widget_set_opacity (icon, 1.0);
  else
    bobgui_widget_set_opacity (icon, 0.0);
}

static void
root_changed (BobguiWidget   *box,
              GParamSpec  *pspec,
              BobguiDropDown *self)
{
  BobguiWidget *icon;

  icon = bobgui_widget_get_last_child (box);

  if (bobgui_widget_get_ancestor (box, BOBGUI_TYPE_POPOVER) == self->popup)
    bobgui_widget_set_visible (icon, TRUE);
  else
    bobgui_widget_set_visible (icon, FALSE);
}

static void
bind_item (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item,
           gpointer                  data)
{
  BobguiDropDown *self = data;
  gpointer item;
  BobguiWidget *box;
  BobguiWidget *label;
  GValue value = G_VALUE_INIT;

  item = bobgui_list_item_get_item (list_item);
  box = bobgui_list_item_get_child (list_item);
  label = bobgui_widget_get_first_child (box);

  if (self->expression &&
      bobgui_expression_evaluate (self->expression, item, &value))
    {
      bobgui_label_set_label (BOBGUI_LABEL (label), g_value_get_string (&value));
      g_value_unset (&value);
    }
  else if (BOBGUI_IS_STRING_OBJECT (item))
    {
      const char *string;

      string = bobgui_string_object_get_string (BOBGUI_STRING_OBJECT (item));
      bobgui_label_set_label (BOBGUI_LABEL (label), string);
    }
  else
    {
      g_critical ("Either BobguiDropDown:factory or BobguiDropDown:expression must be set");
    }

  g_signal_connect (self, "notify::selected-item",
                    G_CALLBACK (selected_item_changed), list_item);
  selected_item_changed (self, NULL, list_item);

  g_signal_connect (box, "notify::root",
                    G_CALLBACK (root_changed), self);
  root_changed (box, NULL, self);
}

static void
unbind_item (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item,
             gpointer                  data)
{
  BobguiDropDown *self = data;
  BobguiWidget *box;

  box = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (self, selected_item_changed, list_item);
  g_signal_handlers_disconnect_by_func (box, root_changed, self);
}

static void
set_default_factory (BobguiDropDown *self)
{
  BobguiListItemFactory *factory;

  factory = bobgui_signal_list_item_factory_new ();

  g_signal_connect (factory, "setup", G_CALLBACK (setup_item), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_item), self);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_item), self);

  bobgui_drop_down_set_factory (self, factory);
  self->uses_default_factory = TRUE;

  if (self->uses_default_list_factory)
    bobgui_drop_down_set_list_factory (self, NULL);

  g_object_unref (factory);
}

static void
bobgui_drop_down_init (BobguiDropDown *self)
{
  g_type_ensure (BOBGUI_TYPE_BUILTIN_ICON);
  g_type_ensure (BOBGUI_TYPE_LIST_ITEM_WIDGET);

  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  self->show_arrow = bobgui_widget_get_visible (self->arrow);

  set_default_factory (self);

  self->search_match_mode = BOBGUI_STRING_FILTER_MATCH_MODE_PREFIX;
}

/**
 * bobgui_drop_down_new:
 * @model: (transfer full) (nullable): the model to use
 * @expression: (transfer full) (nullable): the expression to use
 *
 * Creates a new `BobguiDropDown`.
 *
 * You may want to call [method@Bobgui.DropDown.set_factory]
 * to set up a way to map its items to widgets.
 *
 * Returns: a new `BobguiDropDown`
 */
BobguiWidget *
bobgui_drop_down_new (GListModel    *model,
                   BobguiExpression *expression)
{
  BobguiWidget *self;

  self = g_object_new (BOBGUI_TYPE_DROP_DOWN,
                       "model", model,
                       "expression", expression,
                       NULL);

  /* we're consuming the references */
  g_clear_object (&model);
  g_clear_pointer (&expression, bobgui_expression_unref);

  return self;
}

/**
 * bobgui_drop_down_new_from_strings:
 * @strings: (array zero-terminated=1): The strings to put in the dropdown
 *
 * Creates a new `BobguiDropDown` that is populated with
 * the strings.
 *
 * Returns: a new `BobguiDropDown`
 */
BobguiWidget *
bobgui_drop_down_new_from_strings (const char * const *strings)
{
  return bobgui_drop_down_new (G_LIST_MODEL (bobgui_string_list_new (strings)), NULL);
}

/**
 * bobgui_drop_down_get_model:
 * @self: a `BobguiDropDown`
 *
 * Gets the model that provides the displayed items.
 *
 * Returns: (nullable) (transfer none): The model in use
 */
GListModel *
bobgui_drop_down_get_model (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  return self->model;
}

/**
 * bobgui_drop_down_set_model:
 * @self: a `BobguiDropDown`
 * @model: (nullable) (transfer none): the model to use
 *
 * Sets the `GListModel` to use.
 */
void
bobgui_drop_down_set_model (BobguiDropDown *self,
                         GListModel  *model)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));

  if (!g_set_object (&self->model, model))
    return;

  if (model == NULL)
    {
      bobgui_list_view_set_model (BOBGUI_LIST_VIEW (self->popup_list), NULL);

      if (self->selection)
        {
          g_signal_handlers_disconnect_by_func (self->selection, selection_changed, self);
          g_signal_handlers_disconnect_by_func (self->selection, selection_item_changed, self);
        }

      g_clear_object (&self->selection);
      g_clear_object (&self->filter_model);
      g_clear_object (&self->popup_selection);
    }
  else
    {
      GListModel *filter_model;
      BobguiSelectionModel *selection;

      filter_model = G_LIST_MODEL (bobgui_filter_list_model_new (g_object_ref (model), NULL));
      g_set_object (&self->filter_model, filter_model);

      update_filter (self);

      selection = BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (filter_model));
      g_set_object (&self->popup_selection, selection);
      bobgui_list_view_set_model (BOBGUI_LIST_VIEW (self->popup_list), selection);
      g_object_unref (selection);

      selection = BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (g_object_ref (model)));
      g_set_object (&self->selection, selection);
      g_object_unref (selection);

      g_signal_connect (self->selection, "notify::selected", G_CALLBACK (selection_changed), self);
      g_signal_connect (self->selection, "notify::selected-item", G_CALLBACK (selection_item_changed), self);
      selection_changed (BOBGUI_SINGLE_SELECTION (self->selection), NULL, self);
      selection_item_changed (BOBGUI_SINGLE_SELECTION (self->selection), NULL, self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_drop_down_get_factory:
 * @self: a `BobguiDropDown`
 *
 * Gets the factory that's currently used to populate list items.
 *
 * The factory returned by this function is always used for the
 * item in the button. It is also used for items in the popup
 * if [property@Bobgui.DropDown:list-factory] is not set.
 *
 * Returns: (nullable) (transfer none): The factory in use
 */
BobguiListItemFactory *
bobgui_drop_down_get_factory (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  return self->factory;
}

/**
 * bobgui_drop_down_set_factory:
 * @self: a `BobguiDropDown`
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating list items.
 */
void
bobgui_drop_down_set_factory (BobguiDropDown        *self,
                           BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));
  g_return_if_fail (factory == NULL || BOBGUI_LIST_ITEM_FACTORY (factory));

  if (!g_set_object (&self->factory, factory))
    return;

  if (bobgui_widget_get_root (BOBGUI_WIDGET (self)))
    bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (self->button_item), factory);

  if (self->list_factory == NULL)
    {
      bobgui_list_view_set_factory (BOBGUI_LIST_VIEW (self->popup_list), factory);
      self->uses_default_list_factory = TRUE;
    }

  self->uses_default_factory = factory != NULL;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

/**
 * bobgui_drop_down_get_header_factory:
 * @self: a `BobguiDropDown`
 *
 * Gets the factory that's currently used to create header widgets for the popup.
 *
 * Returns: (nullable) (transfer none): The factory in use
 *
 * Since: 4.12
 */
BobguiListItemFactory *
bobgui_drop_down_get_header_factory (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  return self->header_factory;
}

/**
 * bobgui_drop_down_set_header_factory:
 * @self: a `BobguiDropDown`
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for creating header widgets for the popup.
 *
 * Since: 4.12
 */
void
bobgui_drop_down_set_header_factory (BobguiDropDown        *self,
                                  BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));
  g_return_if_fail (factory == NULL || BOBGUI_LIST_ITEM_FACTORY (factory));

  if (!g_set_object (&self->header_factory, factory))
    return;

  bobgui_list_view_set_header_factory (BOBGUI_LIST_VIEW (self->popup_list), self->header_factory);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADER_FACTORY]);
}

/**
 * bobgui_drop_down_get_list_factory:
 * @self: a `BobguiDropDown`
 *
 * Gets the factory that's currently used to populate list items in the popup.
 *
 * Returns: (nullable) (transfer none): The factory in use
 */
BobguiListItemFactory *
bobgui_drop_down_get_list_factory (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  return self->list_factory;
}

/**
 * bobgui_drop_down_set_list_factory:
 * @self: a `BobguiDropDown`
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating list items in the popup.
 */
void
bobgui_drop_down_set_list_factory (BobguiDropDown        *self,
                                BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));
  g_return_if_fail (factory == NULL || BOBGUI_LIST_ITEM_FACTORY (factory));

  if (!g_set_object (&self->list_factory, factory))
    return;

  if (self->list_factory != NULL)
    bobgui_list_view_set_factory (BOBGUI_LIST_VIEW (self->popup_list), self->list_factory);
  else
    bobgui_list_view_set_factory (BOBGUI_LIST_VIEW (self->popup_list), self->factory);

  self->uses_default_list_factory = factory != NULL;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LIST_FACTORY]);
}

/**
 * bobgui_drop_down_set_selected:
 * @self: a `BobguiDropDown`
 * @position: the position of the item to select, or %BOBGUI_INVALID_LIST_POSITION
 *
 * Selects the item at the given position.
 */
void
bobgui_drop_down_set_selected (BobguiDropDown *self,
                            guint        position)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));

  if (self->selection == NULL)
    return;

  if (bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->selection)) == position)
    return;

  bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (self->selection), position);
}

/**
 * bobgui_drop_down_get_selected:
 * @self: a `BobguiDropDown`
 *
 * Gets the position of the selected item.
 *
 * Returns: the position of the selected item, or %BOBGUI_INVALID_LIST_POSITION
 *   if no item is selected
 */
guint
bobgui_drop_down_get_selected (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->selection == NULL)
    return BOBGUI_INVALID_LIST_POSITION;

  return bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->selection));
}

/**
 * bobgui_drop_down_get_selected_item:
 * @self: a `BobguiDropDown`
 *
 * Gets the selected item. If no item is selected, %NULL is returned.
 *
 * Returns: (transfer none) (type GObject) (nullable): The selected item
 */
gpointer
bobgui_drop_down_get_selected_item (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  if (self->selection == NULL)
    return NULL;

  return bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (self->selection));
}

/**
 * bobgui_drop_down_set_enable_search:
 * @self: a `BobguiDropDown`
 * @enable_search: whether to enable search
 *
 * Sets whether a search entry will be shown in the popup that
 * allows to search for items in the list.
 *
 * Note that [property@Bobgui.DropDown:expression] must be set for
 * search to work.
 */
void
bobgui_drop_down_set_enable_search (BobguiDropDown *self,
                                 gboolean     enable_search)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));

  enable_search = !!enable_search;

  if (self->enable_search == enable_search)
    return;

  self->enable_search = enable_search;

  bobgui_editable_set_text (BOBGUI_EDITABLE (self->search_entry), "");
  bobgui_widget_set_visible (self->search_box, enable_search);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLE_SEARCH]);
}

/**
 * bobgui_drop_down_get_enable_search:
 * @self: a `BobguiDropDown`
 *
 * Returns whether search is enabled.
 *
 * Returns: %TRUE if the popup includes a search entry
 */
gboolean
bobgui_drop_down_get_enable_search (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), FALSE);

  return self->enable_search;
}

/**
 * bobgui_drop_down_set_expression:
 * @self: a `BobguiDropDown`
 * @expression: (nullable): a `BobguiExpression`
 *
 * Sets the expression that gets evaluated to obtain strings from items.
 *
 * This is used for search in the popup. The expression must have
 * a value type of %G_TYPE_STRING.
 */
void
bobgui_drop_down_set_expression (BobguiDropDown   *self,
                              BobguiExpression *expression)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));
  g_return_if_fail (expression == NULL ||
                    bobgui_expression_get_value_type (expression) == G_TYPE_STRING);

  if (self->expression == expression)
    return;

  if (self->expression)
    bobgui_expression_unref (self->expression);
  self->expression = expression;
  if (self->expression)
    bobgui_expression_ref (self->expression);

  if (self->uses_default_factory)
    set_default_factory (self);

  update_filter (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPRESSION]);
}

/**
 * bobgui_drop_down_get_expression:
 * @self: a `BobguiDropDown`
 *
 * Gets the expression set that is used to obtain strings from items.
 *
 * See [method@Bobgui.DropDown.set_expression].
 *
 * Returns: (nullable) (transfer none): a `BobguiExpression`
 */
BobguiExpression *
bobgui_drop_down_get_expression (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), NULL);

  return self->expression;
}

/**
 * bobgui_drop_down_set_show_arrow:
 * @self: a `BobguiDropDown`
 * @show_arrow: whether to show an arrow within the widget
 *
 * Sets whether an arrow will be displayed within the widget.
 *
 * Since: 4.6
 */
void
bobgui_drop_down_set_show_arrow (BobguiDropDown *self,
                              gboolean     show_arrow)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));

  show_arrow = !!show_arrow;

  if (self->show_arrow == show_arrow)
    return;

  self->show_arrow = show_arrow;

  bobgui_widget_set_visible (self->arrow, show_arrow);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_ARROW]);
}

/**
 * bobgui_drop_down_get_show_arrow:
 * @self: a `BobguiDropDown`
 *
 * Returns whether to show an arrow within the widget.
 *
 * Returns: %TRUE if an arrow will be shown.
 *
 * Since: 4.6
 */
gboolean
bobgui_drop_down_get_show_arrow (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), FALSE);

  return self->show_arrow;
}

/**
 * bobgui_drop_down_set_search_match_mode:
 * @self: a `BobguiDropDown`
 * @search_match_mode: the new match mode
 *
 * Sets the match mode for the search filter.
 *
 * Since: 4.12
 */
void
bobgui_drop_down_set_search_match_mode (BobguiDropDown *self,
                                     BobguiStringFilterMatchMode search_match_mode)
{
  g_return_if_fail (BOBGUI_IS_DROP_DOWN (self));

  if (self->search_match_mode == search_match_mode)
    return;

  self->search_match_mode = search_match_mode;

  update_filter (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEARCH_MATCH_MODE]);
}

/**
 * bobgui_drop_down_get_search_match_mode:
 * @self: a `BobguiDropDown`
 *
 * Returns the match mode that the search filter is using.
 *
 * Returns: the match mode of the search filter
 *
 * Since: 4.12
 */
BobguiStringFilterMatchMode
bobgui_drop_down_get_search_match_mode (BobguiDropDown *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_DOWN (self), BOBGUI_STRING_FILTER_MATCH_MODE_PREFIX);

  return self->search_match_mode;
}
