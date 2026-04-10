/* bobguientrycompletion.c
 * Copyright (C) 2003  Kristian Rietveld  <kris@bobgui.org>
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

/**
 * BobguiEntryCompletion:
 *
 * `BobguiEntryCompletion` is an auxiliary object to provide completion functionality
 * for `BobguiEntry`.
 *
 * It implements the [iface@Bobgui.CellLayout] interface, to allow the user
 * to add extra cells to the `BobguiTreeView` with completion matches.
 *
 * “Completion functionality” means that when the user modifies the text
 * in the entry, `BobguiEntryCompletion` checks which rows in the model match
 * the current content of the entry, and displays a list of matches.
 * By default, the matching is done by comparing the entry text
 * case-insensitively against the text column of the model (see
 * [method@Bobgui.EntryCompletion.set_text_column]), but this can be overridden
 * with a custom match function (see [method@Bobgui.EntryCompletion.set_match_func]).
 *
 * When the user selects a completion, the content of the entry is
 * updated. By default, the content of the entry is replaced by the
 * text column of the model, but this can be overridden by connecting
 * to the [signal@Bobgui.EntryCompletion::match-selected] signal and updating the
 * entry in the signal handler. Note that you should return %TRUE from
 * the signal handler to suppress the default behaviour.
 *
 * To add completion functionality to an entry, use
 * [method@Bobgui.Entry.set_completion].
 *
 * `BobguiEntryCompletion` uses a [class@Bobgui.TreeModelFilter] model to
 * represent the subset of the entire model that is currently matching.
 * While the `BobguiEntryCompletion` signals
 * [signal@Bobgui.EntryCompletion::match-selected] and
 * [signal@Bobgui.EntryCompletion::cursor-on-match] take the original model
 * and an iter pointing to that model as arguments, other callbacks and
 * signals (such as `BobguiCellLayoutDataFunc` or
 * [signal@Bobgui.CellArea::apply-attributes)]
 * will generally take the filter model as argument. As long as you are
 * only calling [method@Bobgui.TreeModel.get], this will make no difference to
 * you. If for some reason, you need the original model, use
 * [method@Bobgui.TreeModelFilter.get_model]. Don’t forget to use
 * [method@Bobgui.TreeModelFilter.convert_iter_to_child_iter] to obtain a
 * matching iter.
 *
 * Deprecated: 4.10
 */

#include "config.h"

#include "bobguientrycompletion.h"

#include "bobguientryprivate.h"
#include "bobguitextprivate.h"
#include "bobguicelllayout.h"
#include "bobguicellareabox.h"

#include "bobguicellrenderertext.h"
#include "bobguitreeselection.h"
#include "bobguitreeview.h"
#include "bobguiscrolledwindow.h"
#include "bobguisizerequest.h"
#include "bobguibox.h"
#include "bobguipopover.h"
#include "bobguientry.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollerkey.h"
#include "bobguigestureclick.h"

#include "bobguiprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"

#include <string.h>

#define PAGE_STEP 14
#define COMPLETION_TIMEOUT 100

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* signals */
enum
{
  INSERT_PREFIX,
  MATCH_SELECTED,
  CURSOR_ON_MATCH,
  NO_MATCHES,
  LAST_SIGNAL
};

/* properties */
enum
{
  PROP_0,
  PROP_MODEL,
  PROP_MINIMUM_KEY_LENGTH,
  PROP_TEXT_COLUMN,
  PROP_INLINE_COMPLETION,
  PROP_POPUP_COMPLETION,
  PROP_POPUP_SET_WIDTH,
  PROP_POPUP_SINGLE_MATCH,
  PROP_INLINE_SELECTION,
  PROP_CELL_AREA,
  NUM_PROPERTIES
};


static void     bobgui_entry_completion_cell_layout_init    (BobguiCellLayoutIface      *iface);
static BobguiCellArea* bobgui_entry_completion_get_area        (BobguiCellLayout           *cell_layout);

static void     bobgui_entry_completion_constructed         (GObject      *object);
static void     bobgui_entry_completion_set_property        (GObject      *object,
                                                          guint         prop_id,
                                                          const GValue *value,
                                                          GParamSpec   *pspec);
static void     bobgui_entry_completion_get_property        (GObject      *object,
                                                          guint         prop_id,
                                                          GValue       *value,
                                                          GParamSpec   *pspec);
static void     bobgui_entry_completion_finalize            (GObject      *object);
static void     bobgui_entry_completion_dispose             (GObject      *object);

static gboolean bobgui_entry_completion_visible_func        (BobguiTreeModel       *model,
                                                          BobguiTreeIter        *iter,
                                                          gpointer            data);
static void     bobgui_entry_completion_list_activated      (BobguiTreeView        *treeview,
                                                          BobguiTreePath        *path,
                                                          BobguiTreeViewColumn  *column,
                                                          gpointer            user_data);
static void     bobgui_entry_completion_selection_changed   (BobguiTreeSelection   *selection,
                                                          gpointer            data);

static gboolean bobgui_entry_completion_match_selected      (BobguiEntryCompletion *completion,
                                                          BobguiTreeModel       *model,
                                                          BobguiTreeIter        *iter);
static gboolean bobgui_entry_completion_real_insert_prefix  (BobguiEntryCompletion *completion,
                                                          const char         *prefix);
static gboolean bobgui_entry_completion_cursor_on_match     (BobguiEntryCompletion *completion,
                                                          BobguiTreeModel       *model,
                                                          BobguiTreeIter        *iter);
static gboolean bobgui_entry_completion_insert_completion   (BobguiEntryCompletion *completion,
                                                          BobguiTreeModel       *model,
                                                          BobguiTreeIter        *iter);
static void     bobgui_entry_completion_insert_completion_text (BobguiEntryCompletion *completion,
                                                             const char *text);
static void     connect_completion_signals                  (BobguiEntryCompletion *completion);
static void     disconnect_completion_signals               (BobguiEntryCompletion *completion);


static GParamSpec *entry_completion_props[NUM_PROPERTIES] = { NULL, };

static guint entry_completion_signals[LAST_SIGNAL] = { 0 };

/* BobguiBuildable */
static void     bobgui_entry_completion_buildable_init      (BobguiBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiEntryCompletion, bobgui_entry_completion, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_LAYOUT,
                                                bobgui_entry_completion_cell_layout_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_entry_completion_buildable_init))


static void
bobgui_entry_completion_class_init (BobguiEntryCompletionClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *)klass;

  object_class->constructed = bobgui_entry_completion_constructed;
  object_class->set_property = bobgui_entry_completion_set_property;
  object_class->get_property = bobgui_entry_completion_get_property;
  object_class->dispose = bobgui_entry_completion_dispose;
  object_class->finalize = bobgui_entry_completion_finalize;

  klass->match_selected = bobgui_entry_completion_match_selected;
  klass->insert_prefix = bobgui_entry_completion_real_insert_prefix;
  klass->cursor_on_match = bobgui_entry_completion_cursor_on_match;
  klass->no_matches = NULL;

  /**
   * BobguiEntryCompletion::insert-prefix:
   * @widget: the object which received the signal
   * @prefix: the common prefix of all possible completions
   *
   * Emitted when the inline autocompletion is triggered.
   *
   * The default behaviour is to make the entry display the
   * whole prefix and select the newly inserted part.
   *
   * Applications may connect to this signal in order to insert only a
   * smaller part of the @prefix into the entry - e.g. the entry used in
   * the `BobguiFileChooser` inserts only the part of the prefix up to the
   * next '/'.
   *
   * Returns: %TRUE if the signal has been handled
   */
  entry_completion_signals[INSERT_PREFIX] =
    g_signal_new (I_("insert-prefix"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiEntryCompletionClass, insert_prefix),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__STRING,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_STRING);

  /**
   * BobguiEntryCompletion::match-selected:
   * @widget: the object which received the signal
   * @model: the `BobguiTreeModel` containing the matches
   * @iter: a `BobguiTreeIter` positioned at the selected match
   *
   * Emitted when a match from the list is selected.
   *
   * The default behaviour is to replace the contents of the
   * entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Note that @model is the model that was passed to
   * [method@Bobgui.EntryCompletion.set_model].
   *
   * Returns: %TRUE if the signal has been handled
   */
  entry_completion_signals[MATCH_SELECTED] =
    g_signal_new (I_("match-selected"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiEntryCompletionClass, match_selected),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_TREE_MODEL,
                  BOBGUI_TYPE_TREE_ITER);

  /**
   * BobguiEntryCompletion::cursor-on-match:
   * @widget: the object which received the signal
   * @model: the `BobguiTreeModel` containing the matches
   * @iter: a `BobguiTreeIter` positioned at the selected match
   *
   * Emitted when a match from the cursor is on a match of the list.
   *
   * The default behaviour is to replace the contents
   * of the entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Note that @model is the model that was passed to
   * [method@Bobgui.EntryCompletion.set_model].
   *
   * Returns: %TRUE if the signal has been handled
   */
  entry_completion_signals[CURSOR_ON_MATCH] =
    g_signal_new (I_("cursor-on-match"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiEntryCompletionClass, cursor_on_match),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_TREE_MODEL,
                  BOBGUI_TYPE_TREE_ITER);

  /**
   * BobguiEntryCompletion::no-matches:
   * @widget: the object which received the signal
   *
   * Emitted when the filter model has zero
   * number of rows in completion_complete method.
   *
   * In other words when `BobguiEntryCompletion` is out of suggestions.
   */
  entry_completion_signals[NO_MATCHES] =
    g_signal_new (I_("no-matches"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiEntryCompletionClass, no_matches),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiEntryCompletion:model:
   *
   * The model used as data source.
   */
  entry_completion_props[PROP_MODEL] =
      g_param_spec_object ("model", NULL, NULL,
                           BOBGUI_TYPE_TREE_MODEL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiEntryCompletion:minimum-key-length:
   *
   * The minimum key length as set for completion.
   */
  entry_completion_props[PROP_MINIMUM_KEY_LENGTH] =
      g_param_spec_int ("minimum-key-length", NULL, NULL,
                        0, G_MAXINT, 1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:text-column:
   *
   * The column of the model containing the strings.
   *
   * Note that the strings must be UTF-8.
   */
  entry_completion_props[PROP_TEXT_COLUMN] =
    g_param_spec_int ("text-column", NULL, NULL,
                      -1, G_MAXINT, -1,
                      BOBGUI_PARAM_READWRITE);

  /**
   * BobguiEntryCompletion:inline-completion:
   *
   * Determines whether the common prefix of the possible completions
   * should be inserted automatically in the entry.
   *
   * Note that this requires text-column to be set, even if you are
   * using a custom match function.
   */
  entry_completion_props[PROP_INLINE_COMPLETION] =
      g_param_spec_boolean ("inline-completion", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:popup-completion:
   *
   * Determines whether the possible completions should be
   * shown in a popup window.
   */
  entry_completion_props[PROP_POPUP_COMPLETION] =
      g_param_spec_boolean ("popup-completion", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:popup-set-width:
   *
   * Determines whether the completions popup window will be
   * resized to the width of the entry.
   */
  entry_completion_props[PROP_POPUP_SET_WIDTH] =
      g_param_spec_boolean ("popup-set-width", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:popup-single-match:
   *
   * Determines whether the completions popup window will shown
   * for a single possible completion.
   *
   * You probably want to set this to %FALSE if you are using
   * [property@Bobgui.EntryCompletion:inline-completion].
   */
  entry_completion_props[PROP_POPUP_SINGLE_MATCH] =
      g_param_spec_boolean ("popup-single-match", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:inline-selection:
   *
   * Determines whether the possible completions on the popup
   * will appear in the entry as you navigate through them.
   */
  entry_completion_props[PROP_INLINE_SELECTION] =
      g_param_spec_boolean ("inline-selection", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntryCompletion:cell-area:
   *
   * The `BobguiCellArea` used to layout cell renderers in the treeview column.
   *
   * If no area is specified when creating the entry completion with
   * [ctor@Bobgui.EntryCompletion.new_with_area], a horizontally oriented
   * [class@Bobgui.CellAreaBox] will be used.
   */
  entry_completion_props[PROP_CELL_AREA] =
      g_param_spec_object ("cell-area", NULL, NULL,
                           BOBGUI_TYPE_CELL_AREA,
                           BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, entry_completion_props);
}


static void
bobgui_entry_completion_buildable_custom_tag_end (BobguiBuildable *buildable,
                                                BobguiBuilder   *builder,
                                                GObject      *child,
                                                const char   *tagname,
                                                gpointer      data)
{
  /* Just ignore the boolean return from here */
  _bobgui_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data);
}

static void
bobgui_entry_completion_buildable_init (BobguiBuildableIface *iface)
{
  iface->add_child = _bobgui_cell_layout_buildable_add_child;
  iface->custom_tag_start = _bobgui_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = bobgui_entry_completion_buildable_custom_tag_end;
}

static void
bobgui_entry_completion_cell_layout_init (BobguiCellLayoutIface *iface)
{
  iface->get_area = bobgui_entry_completion_get_area;
}

static void
bobgui_entry_completion_init (BobguiEntryCompletion *completion)
{
  completion->minimum_key_length = 1;
  completion->text_column = -1;
  completion->has_completion = FALSE;
  completion->inline_completion = FALSE;
  completion->popup_completion = TRUE;
  completion->popup_set_width = TRUE;
  completion->popup_single_match = TRUE;
  completion->inline_selection = FALSE;

  completion->filter_model = NULL;
  completion->insert_text_signal_group = NULL;
}

static gboolean
propagate_to_entry (BobguiEventControllerKey *key,
                    guint                  keyval,
                    guint                  keycode,
                    GdkModifierType        modifiers,
                    BobguiEntryCompletion    *completion)
{
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  return bobgui_event_controller_key_forward (key, BOBGUI_WIDGET (text));
}

static void
bobgui_entry_completion_constructed (GObject *object)
{
  BobguiEntryCompletion        *completion = BOBGUI_ENTRY_COMPLETION (object);
  BobguiTreeSelection          *sel;
  BobguiEventController        *controller;

  G_OBJECT_CLASS (bobgui_entry_completion_parent_class)->constructed (object);

  if (!completion->cell_area)
    {
      completion->cell_area = bobgui_cell_area_box_new ();
      g_object_ref_sink (completion->cell_area);
    }

  /* completions */
  completion->tree_view = bobgui_tree_view_new ();
  g_signal_connect (completion->tree_view, "row-activated",
                    G_CALLBACK (bobgui_entry_completion_list_activated),
                    completion);

  bobgui_tree_view_set_enable_search (BOBGUI_TREE_VIEW (completion->tree_view), FALSE);
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (completion->tree_view), FALSE);
  bobgui_tree_view_set_hover_selection (BOBGUI_TREE_VIEW (completion->tree_view), TRUE);
  bobgui_tree_view_set_activate_on_single_click (BOBGUI_TREE_VIEW (completion->tree_view), TRUE);

  sel = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (completion->tree_view));
  bobgui_tree_selection_set_mode (sel, BOBGUI_SELECTION_SINGLE);
  bobgui_tree_selection_unselect_all (sel);
  g_signal_connect (sel, "changed",
                    G_CALLBACK (bobgui_entry_completion_selection_changed),
                    completion);
  completion->first_sel_changed = TRUE;

  completion->column = bobgui_tree_view_column_new_with_area (completion->cell_area);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (completion->tree_view), completion->column);

  completion->scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (completion->scrolled_window),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);

  /* a nasty hack to get the completions treeview to size nicely */
  bobgui_widget_set_size_request (bobgui_scrolled_window_get_vscrollbar (BOBGUI_SCROLLED_WINDOW (completion->scrolled_window)),
                               -1, 0);

  /* pack it all */
  completion->popup_window = bobgui_popover_new ();
  bobgui_popover_set_position (BOBGUI_POPOVER (completion->popup_window), BOBGUI_POS_BOTTOM);
  bobgui_popover_set_autohide (BOBGUI_POPOVER (completion->popup_window), FALSE);
  bobgui_popover_set_has_arrow (BOBGUI_POPOVER (completion->popup_window), FALSE);
  bobgui_widget_add_css_class (completion->popup_window, "entry-completion");

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (propagate_to_entry), completion);
  g_signal_connect (controller, "key-released",
                    G_CALLBACK (propagate_to_entry), completion);
  bobgui_widget_add_controller (completion->popup_window, controller);

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
  g_signal_connect_swapped (controller, "released",
                            G_CALLBACK (_bobgui_entry_completion_popdown),
                            completion);
  bobgui_widget_add_controller (completion->popup_window, controller);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (completion->scrolled_window),
                                 completion->tree_view);
  bobgui_widget_set_hexpand (completion->scrolled_window, TRUE);
  bobgui_widget_set_vexpand (completion->scrolled_window, TRUE);
  bobgui_popover_set_child (BOBGUI_POPOVER (completion->popup_window), completion->scrolled_window);
}


static void
bobgui_entry_completion_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (object);
  BobguiCellArea *area;

  switch (prop_id)
    {
      case PROP_MODEL:
        bobgui_entry_completion_set_model (completion,
                                        g_value_get_object (value));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        bobgui_entry_completion_set_minimum_key_length (completion,
                                                     g_value_get_int (value));
        break;

      case PROP_TEXT_COLUMN:
        completion->text_column = g_value_get_int (value);
        break;

      case PROP_INLINE_COMPLETION:
	bobgui_entry_completion_set_inline_completion (completion,
						    g_value_get_boolean (value));
        break;

      case PROP_POPUP_COMPLETION:
	bobgui_entry_completion_set_popup_completion (completion,
						   g_value_get_boolean (value));
        break;

      case PROP_POPUP_SET_WIDTH:
	bobgui_entry_completion_set_popup_set_width (completion,
						  g_value_get_boolean (value));
        break;

      case PROP_POPUP_SINGLE_MATCH:
	bobgui_entry_completion_set_popup_single_match (completion,
						     g_value_get_boolean (value));
        break;

      case PROP_INLINE_SELECTION:
	bobgui_entry_completion_set_inline_selection (completion,
						   g_value_get_boolean (value));
        break;

      case PROP_CELL_AREA:
        /* Construct-only, can only be assigned once */
        area = g_value_get_object (value);
        if (area)
          {
            if (completion->cell_area != NULL)
              {
                g_warning ("cell-area has already been set, ignoring construct property");
                g_object_ref_sink (area);
                g_object_unref (area);
              }
            else
              completion->cell_area = g_object_ref_sink (area);
          }
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
bobgui_entry_completion_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (object);

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value,
                            bobgui_entry_completion_get_model (completion));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        g_value_set_int (value, bobgui_entry_completion_get_minimum_key_length (completion));
        break;

      case PROP_TEXT_COLUMN:
        g_value_set_int (value, bobgui_entry_completion_get_text_column (completion));
        break;

      case PROP_INLINE_COMPLETION:
        g_value_set_boolean (value, bobgui_entry_completion_get_inline_completion (completion));
        break;

      case PROP_POPUP_COMPLETION:
        g_value_set_boolean (value, bobgui_entry_completion_get_popup_completion (completion));
        break;

      case PROP_POPUP_SET_WIDTH:
        g_value_set_boolean (value, bobgui_entry_completion_get_popup_set_width (completion));
        break;

      case PROP_POPUP_SINGLE_MATCH:
        g_value_set_boolean (value, bobgui_entry_completion_get_popup_single_match (completion));
        break;

      case PROP_INLINE_SELECTION:
        g_value_set_boolean (value, bobgui_entry_completion_get_inline_selection (completion));
        break;

      case PROP_CELL_AREA:
        g_value_set_object (value, completion->cell_area);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
bobgui_entry_completion_finalize (GObject *object)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (object);

  g_free (completion->case_normalized_key);
  g_free (completion->completion_prefix);

  if (completion->match_notify)
    (* completion->match_notify) (completion->match_data);

  G_OBJECT_CLASS (bobgui_entry_completion_parent_class)->finalize (object);
}

static void
bobgui_entry_completion_dispose (GObject *object)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (object);

  if (completion->entry)
    bobgui_entry_set_completion (BOBGUI_ENTRY (completion->entry), NULL);

  g_clear_object (&completion->cell_area);

  G_OBJECT_CLASS (bobgui_entry_completion_parent_class)->dispose (object);
}

/* implement cell layout interface (only need to return the underlying cell area) */
static BobguiCellArea*
bobgui_entry_completion_get_area (BobguiCellLayout *cell_layout)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (cell_layout);

  if (G_UNLIKELY (!completion->cell_area))
    {
      completion->cell_area = bobgui_cell_area_box_new ();
      g_object_ref_sink (completion->cell_area);
    }

  return completion->cell_area;
}

/* all those callbacks */
static gboolean
bobgui_entry_completion_default_completion_func (BobguiEntryCompletion *completion,
                                              const char         *key,
                                              BobguiTreeIter        *iter,
                                              gpointer            user_data)
{
  char *item = NULL;
  char *normalized_string;
  char *case_normalized_string;

  gboolean ret = FALSE;

  BobguiTreeModel *model;

  model = bobgui_tree_model_filter_get_model (completion->filter_model);

  g_return_val_if_fail (bobgui_tree_model_get_column_type (model, completion->text_column) == G_TYPE_STRING,
                        FALSE);

  bobgui_tree_model_get (model, iter,
                      completion->text_column, &item,
                      -1);

  if (item != NULL)
    {
      normalized_string = g_utf8_normalize (item, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

          if (!strncmp (key, case_normalized_string, strlen (key)))
            ret = TRUE;

          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }
  g_free (item);

  return ret;
}

static gboolean
bobgui_entry_completion_visible_func (BobguiTreeModel *model,
                                   BobguiTreeIter  *iter,
                                   gpointer      data)
{
  gboolean ret = FALSE;

  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (data);

  if (!completion->case_normalized_key)
    return ret;

  if (completion->match_func)
    ret = (* completion->match_func) (completion,
                                            completion->case_normalized_key,
                                            iter,
                                            completion->match_data);
  else if (completion->text_column >= 0)
    ret = bobgui_entry_completion_default_completion_func (completion,
                                                        completion->case_normalized_key,
                                                        iter,
                                                        NULL);

  return ret;
}

static void
bobgui_entry_completion_list_activated (BobguiTreeView       *treeview,
                                     BobguiTreePath       *path,
                                     BobguiTreeViewColumn *column,
                                     gpointer           user_data)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (user_data);
  BobguiTreeIter iter;
  gboolean entry_set;
  BobguiTreeModel *model;
  BobguiTreeIter child_iter;
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (completion->filter_model), &iter, path);
  bobgui_tree_model_filter_convert_iter_to_child_iter (completion->filter_model,
                                                    &child_iter,
                                                    &iter);
  model = bobgui_tree_model_filter_get_model (completion->filter_model);

  g_signal_handler_block (text, completion->changed_id);
  g_signal_emit (completion, entry_completion_signals[MATCH_SELECTED],
                 0, model, &child_iter, &entry_set);
  g_signal_handler_unblock (text, completion->changed_id);

  _bobgui_entry_completion_popdown (completion);
}

static void
bobgui_entry_completion_selection_changed (BobguiTreeSelection *selection,
                                        gpointer          data)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (data);

  if (completion->first_sel_changed)
    {
      completion->first_sel_changed = FALSE;
      if (bobgui_widget_is_focus (completion->tree_view))
        bobgui_tree_selection_unselect_all (selection);
    }
}

/* public API */

/**
 * bobgui_entry_completion_new:
 *
 * Creates a new `BobguiEntryCompletion` object.
 *
 * Returns: A newly created `BobguiEntryCompletion` object
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
BobguiEntryCompletion *
bobgui_entry_completion_new (void)
{
  BobguiEntryCompletion *completion;

  completion = g_object_new (BOBGUI_TYPE_ENTRY_COMPLETION, NULL);

  return completion;
}

/**
 * bobgui_entry_completion_new_with_area:
 * @area: the `BobguiCellArea` used to layout cells
 *
 * Creates a new `BobguiEntryCompletion` object using the
 * specified @area.
 *
 * The `BobguiCellArea` is used to layout cells in the underlying
 * `BobguiTreeViewColumn` for the drop-down menu.
 *
 * Returns: A newly created `BobguiEntryCompletion` object
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
BobguiEntryCompletion *
bobgui_entry_completion_new_with_area (BobguiCellArea *area)
{
  BobguiEntryCompletion *completion;

  completion = g_object_new (BOBGUI_TYPE_ENTRY_COMPLETION, "cell-area", area, NULL);

  return completion;
}

/**
 * bobgui_entry_completion_get_entry:
 * @completion: a `BobguiEntryCompletion`
 *
 * Gets the entry @completion has been attached to.
 *
 * Returns: (transfer none): The entry @completion has been attached to
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
BobguiWidget *
bobgui_entry_completion_get_entry (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->entry;
}

/**
 * bobgui_entry_completion_set_model:
 * @completion: a `BobguiEntryCompletion`
 * @model: (nullable): the `BobguiTreeModel`
 *
 * Sets the model for a `BobguiEntryCompletion`.
 *
 * If @completion already has a model set, it will remove it
 * before setting the new model. If model is %NULL, then it
 * will unset the model.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_model (BobguiEntryCompletion *completion,
                                BobguiTreeModel       *model)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (model == NULL || BOBGUI_IS_TREE_MODEL (model));

  if (!model)
    {
      bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (completion->tree_view),
                               NULL);
      _bobgui_entry_completion_popdown (completion);
      completion->filter_model = NULL;
      return;
    }

  /* code will unref the old filter model (if any) */
  completion->filter_model =
    BOBGUI_TREE_MODEL_FILTER (bobgui_tree_model_filter_new (model, NULL));
  bobgui_tree_model_filter_set_visible_func (completion->filter_model,
                                          bobgui_entry_completion_visible_func,
                                          completion,
                                          NULL);

  bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (completion->tree_view),
                           BOBGUI_TREE_MODEL (completion->filter_model));
  g_object_unref (completion->filter_model);

  g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_MODEL]);

  if (bobgui_widget_get_visible (completion->popup_window))
    _bobgui_entry_completion_resize_popup (completion);
}

/**
 * bobgui_entry_completion_get_model:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns the model the `BobguiEntryCompletion` is using as data source.
 *
 * Returns %NULL if the model is unset.
 *
 * Returns: (nullable) (transfer none): A `BobguiTreeModel`
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
BobguiTreeModel *
bobgui_entry_completion_get_model (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), NULL);

  if (!completion->filter_model)
    return NULL;

  return bobgui_tree_model_filter_get_model (completion->filter_model);
}

/**
 * bobgui_entry_completion_set_match_func:
 * @completion: a `BobguiEntryCompletion`
 * @func: the `BobguiEntryCompletion`MatchFunc to use
 * @func_data: user data for @func
 * @func_notify: destroy notify for @func_data.
 *
 * Sets the match function for @completion to be @func.
 *
 * The match function is used to determine if a row should or
 * should not be in the completion list.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_match_func (BobguiEntryCompletion          *completion,
                                     BobguiEntryCompletionMatchFunc  func,
                                     gpointer                     func_data,
                                     GDestroyNotify               func_notify)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  if (completion->match_notify)
    (* completion->match_notify) (completion->match_data);

  completion->match_func = func;
  completion->match_data = func_data;
  completion->match_notify = func_notify;
}

/**
 * bobgui_entry_completion_set_minimum_key_length:
 * @completion: a `BobguiEntryCompletion`
 * @length: the minimum length of the key in order to start completing
 *
 * Requires the length of the search key for @completion to be at least
 * @length.
 *
 * This is useful for long lists, where completing using a small
 * key takes a lot of time and will come up with meaningless results anyway
 * (ie, a too large dataset).
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_minimum_key_length (BobguiEntryCompletion *completion,
                                             int                 length)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (length >= 0);

  if (completion->minimum_key_length != length)
    {
      completion->minimum_key_length = length;

      g_object_notify_by_pspec (G_OBJECT (completion),
                                entry_completion_props[PROP_MINIMUM_KEY_LENGTH]);
    }
}

/**
 * bobgui_entry_completion_get_minimum_key_length:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns the minimum key length as set for @completion.
 *
 * Returns: The currently used minimum key length
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
int
bobgui_entry_completion_get_minimum_key_length (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), 0);

  return completion->minimum_key_length;
}

/**
 * bobgui_entry_completion_complete:
 * @completion: a `BobguiEntryCompletion`
 *
 * Requests a completion operation, or in other words a refiltering of the
 * current list with completions, using the current key.
 *
 * The completion list view will be updated accordingly.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_complete (BobguiEntryCompletion *completion)
{
  char *tmp;
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (BOBGUI_IS_ENTRY (completion->entry));

  if (!completion->filter_model)
    return;

  g_free (completion->case_normalized_key);

  tmp = g_utf8_normalize (bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry)),
                          -1, G_NORMALIZE_ALL);
  completion->case_normalized_key = g_utf8_casefold (tmp, -1);
  g_free (tmp);

  bobgui_tree_model_filter_refilter (completion->filter_model);

  if (!bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (completion->filter_model), &iter))
    g_signal_emit (completion, entry_completion_signals[NO_MATCHES], 0);

  if (bobgui_widget_get_visible (completion->popup_window))
    _bobgui_entry_completion_resize_popup (completion);
}

/**
 * bobgui_entry_completion_set_text_column:
 * @completion: a `BobguiEntryCompletion`
 * @column: the column in the model of @completion to get strings from
 *
 * Convenience function for setting up the most used case of this code: a
 * completion list with just strings.
 *
 * This function will set up @completion
 * to have a list displaying all (and just) strings in the completion list,
 * and to get those strings from @column in the model of @completion.
 *
 * This functions creates and adds a `BobguiCellRendererText` for the selected
 * column. If you need to set the text column, but don't want the cell
 * renderer, use g_object_set() to set the
 * [property@Bobgui.EntryCompletion:text-column] property directly.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_text_column (BobguiEntryCompletion *completion,
                                      int                 column)
{
  BobguiCellRenderer *cell;

  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (column >= 0);

  if (completion->text_column == column)
    return;

  completion->text_column = column;

  cell = bobgui_cell_renderer_text_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (completion),
                              cell, TRUE);
  bobgui_cell_layout_add_attribute (BOBGUI_CELL_LAYOUT (completion),
                                 cell,
                                 "text", column);

  g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_TEXT_COLUMN]);
}

/**
 * bobgui_entry_completion_get_text_column:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns the column in the model of @completion to get strings from.
 *
 * Returns: the column containing the strings
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
int
bobgui_entry_completion_get_text_column (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), -1);

  return completion->text_column;
}

/* private */

/* some nasty size requisition */
void
_bobgui_entry_completion_resize_popup (BobguiEntryCompletion *completion)
{
  graphene_rect_t bounds;
  int matches, items, height;
  GdkSurface *surface;
  BobguiRequisition entry_req;
  BobguiRequisition tree_req;
  int width;

  surface = bobgui_native_get_surface (bobgui_widget_get_native (completion->entry));

  if (!surface)
    return;

  if (!completion->filter_model)
    return;

  if (!bobgui_widget_compute_bounds (completion->entry,
                                  BOBGUI_WIDGET (bobgui_widget_get_native (completion->entry)),
                                  &bounds))
    graphene_rect_init (&bounds, 0, 0, 0, 0);

  bobgui_widget_get_preferred_size (completion->entry, &entry_req, NULL);

  matches = bobgui_tree_model_iter_n_children (BOBGUI_TREE_MODEL (completion->filter_model), NULL);

  /* Call get preferred size on the on the tree view to force it to validate its
   * cells before calling into the cell size functions.
   */
  bobgui_widget_get_preferred_size (completion->tree_view, &tree_req, NULL);
  bobgui_tree_view_column_cell_get_size (completion->column, NULL, NULL, NULL, &height);

  bobgui_widget_realize (completion->tree_view);

  items = MIN (matches, 10);

  if (items <= 0)
    bobgui_widget_hide (completion->scrolled_window);
  else
    bobgui_widget_show (completion->scrolled_window);

  if (completion->popup_set_width)
    width = ceilf (bounds.size.width);
  else
    width = -1;

  bobgui_tree_view_columns_autosize (BOBGUI_TREE_VIEW (completion->tree_view));
  bobgui_scrolled_window_set_min_content_width (BOBGUI_SCROLLED_WINDOW (completion->scrolled_window), width);
  bobgui_widget_set_size_request (completion->popup_window, width, -1);
  bobgui_scrolled_window_set_min_content_height (BOBGUI_SCROLLED_WINDOW (completion->scrolled_window), items * height);

  bobgui_popover_present (BOBGUI_POPOVER (completion->popup_window));
}

static void
bobgui_entry_completion_popup (BobguiEntryCompletion *completion)
{
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  if (bobgui_widget_get_mapped (completion->popup_window))
    return;

  if (!bobgui_widget_get_mapped (BOBGUI_WIDGET (text)))
    return;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (text)))
    return;

  /* default on no match */
  completion->current_selected = -1;

  bobgui_widget_realize (completion->popup_window);

  _bobgui_entry_completion_resize_popup (completion);

  if (completion->filter_model)
    {
      BobguiTreePath *path;

      path = bobgui_tree_path_new_from_indices (0, -1);
      bobgui_tree_view_scroll_to_cell (BOBGUI_TREE_VIEW (completion->tree_view), path,
                                    NULL, FALSE, 0.0, 0.0);
      bobgui_tree_path_free (path);
    }

  bobgui_popover_popup (BOBGUI_POPOVER (completion->popup_window));
}

void
_bobgui_entry_completion_popdown (BobguiEntryCompletion *completion)
{
  if (!bobgui_widget_get_mapped (completion->popup_window))
    return;

  bobgui_popover_popdown (BOBGUI_POPOVER (completion->popup_window));
}

static gboolean
bobgui_entry_completion_match_selected (BobguiEntryCompletion *completion,
                                     BobguiTreeModel       *model,
                                     BobguiTreeIter        *iter)
{
  g_assert (completion->entry != NULL);

  char *str = NULL;

  bobgui_tree_model_get (model, iter, completion->text_column, &str, -1);
  bobgui_editable_set_text (BOBGUI_EDITABLE (completion->entry), str ? str : "");

  /* move cursor to the end */
  bobgui_editable_set_position (BOBGUI_EDITABLE (completion->entry), -1);

  g_free (str);

  return TRUE;
}

static gboolean
bobgui_entry_completion_cursor_on_match (BobguiEntryCompletion *completion,
                                      BobguiTreeModel       *model,
                                      BobguiTreeIter        *iter)
{
  g_assert (completion->entry != NULL);

  bobgui_entry_completion_insert_completion (completion, model, iter);

  return TRUE;
}

/**
 * bobgui_entry_completion_compute_prefix:
 * @completion: the entry completion
 * @key: The text to complete for
 *
 * Computes the common prefix that is shared by all rows in @completion
 * that start with @key.
 *
 * If no row matches @key, %NULL will be returned.
 * Note that a text column must have been set for this function to work,
 * see [method@Bobgui.EntryCompletion.set_text_column] for details.
 *
 * Returns: (nullable) (transfer full): The common prefix all rows
 *   starting with @key
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
char *
bobgui_entry_completion_compute_prefix (BobguiEntryCompletion *completion,
                                     const char         *key)
{
  BobguiTreeIter iter;
  char *prefix = NULL;
  gboolean valid;

  if (completion->text_column < 0)
    return NULL;

  valid = bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (completion->filter_model),
                                         &iter);

  while (valid)
    {
      char *text;

      bobgui_tree_model_get (BOBGUI_TREE_MODEL (completion->filter_model),
                          &iter, completion->text_column, &text,
                          -1);

      if (text && g_str_has_prefix (text, key))
        {
          if (!prefix)
            prefix = g_strdup (text);
          else
            {
              char *p = prefix;
              char *q = text;

              while (*p && *p == *q)
                {
                  p++;
                  q++;
                }

              *p = '\0';

              if (p > prefix)
                {
                  /* strip a partial multibyte character */
                  q = g_utf8_find_prev_char (prefix, p);
                  switch (g_utf8_get_char_validated (q, p - q))
                    {
                    case (gunichar)-2:
                    case (gunichar)-1:
                      *q = 0;
                      break;
                    default: ;
                    }
                }
            }
        }

      g_free (text);
      valid = bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (completion->filter_model),
                                        &iter);
    }

  return prefix;
}


static gboolean
bobgui_entry_completion_real_insert_prefix (BobguiEntryCompletion *completion,
                                         const char         *prefix)
{
  g_assert (completion->entry != NULL);

  if (prefix)
    {
      int key_len;
      int prefix_len;
      const char *key;

      prefix_len = g_utf8_strlen (prefix, -1);

      key = bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry));
      key_len = g_utf8_strlen (key, -1);

      if (prefix_len > key_len)
        {
          int pos = prefix_len;

          bobgui_editable_insert_text (BOBGUI_EDITABLE (completion->entry),
                                    prefix + strlen (key), -1, &pos);
          bobgui_editable_select_region (BOBGUI_EDITABLE (completion->entry),
                                      key_len, prefix_len);

          completion->has_completion = TRUE;
        }
    }

  return TRUE;
}

/**
 * bobgui_entry_completion_get_completion_prefix:
 * @completion: a `BobguiEntryCompletion`
 *
 * Get the original text entered by the user that triggered
 * the completion or %NULL if there’s no completion ongoing.
 *
 * Returns: (nullable): the prefix for the current completion
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
const char *
bobgui_entry_completion_get_completion_prefix (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->completion_prefix;
}

static void
bobgui_entry_completion_insert_completion_text (BobguiEntryCompletion *completion,
                                             const char         *new_text)
{
  int len;
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  if (completion->changed_id > 0)
    g_signal_handler_block (text, completion->changed_id);

  if (completion->insert_text_signal_group != NULL)
    g_signal_group_block (completion->insert_text_signal_group);

  bobgui_editable_set_text (BOBGUI_EDITABLE (completion->entry), new_text);

  len = g_utf8_strlen (completion->completion_prefix, -1);
  bobgui_editable_select_region (BOBGUI_EDITABLE (completion->entry), len, -1);

  if (completion->changed_id > 0)
    g_signal_handler_unblock (text, completion->changed_id);

  if (completion->insert_text_signal_group != NULL)
    g_signal_group_unblock (completion->insert_text_signal_group);
}

static gboolean
bobgui_entry_completion_insert_completion (BobguiEntryCompletion *completion,
                                        BobguiTreeModel       *model,
                                        BobguiTreeIter        *iter)
{
  char *str = NULL;

  if (completion->text_column < 0)
    return FALSE;

  bobgui_tree_model_get (model, iter,
                      completion->text_column, &str,
                      -1);

  bobgui_entry_completion_insert_completion_text (completion, str);

  g_free (str);

  return TRUE;
}

/**
 * bobgui_entry_completion_insert_prefix:
 * @completion: a `BobguiEntryCompletion`
 *
 * Requests a prefix insertion.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_insert_prefix (BobguiEntryCompletion *completion)
{
  g_return_if_fail (completion->entry != NULL);

  gboolean done;
  char *prefix;

  if (completion->insert_text_signal_group != NULL)
    g_signal_group_block (completion->insert_text_signal_group);

  prefix = bobgui_entry_completion_compute_prefix (completion,
                                                bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry)));

  if (prefix)
    {
      g_signal_emit (completion, entry_completion_signals[INSERT_PREFIX],
                     0, prefix, &done);
      g_free (prefix);
    }

  if (completion->insert_text_signal_group != NULL)
    g_signal_group_unblock (completion->insert_text_signal_group);
}

/**
 * bobgui_entry_completion_set_inline_completion:
 * @completion: a `BobguiEntryCompletion`
 * @inline_completion: %TRUE to do inline completion
 *
 * Sets whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_inline_completion (BobguiEntryCompletion *completion,
                                            gboolean            inline_completion)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  inline_completion = inline_completion != FALSE;

  if (completion->inline_completion != inline_completion)
    {
      completion->inline_completion = inline_completion;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_INLINE_COMPLETION]);
    }
}

/**
 * bobgui_entry_completion_get_inline_completion:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 *
 * Returns: %TRUE if inline completion is turned on
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
gboolean
bobgui_entry_completion_get_inline_completion (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), FALSE);

  return completion->inline_completion;
}

/**
 * bobgui_entry_completion_set_popup_completion:
 * @completion: a `BobguiEntryCompletion`
 * @popup_completion: %TRUE to do popup completion
 *
 * Sets whether the completions should be presented in a popup window.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_popup_completion (BobguiEntryCompletion *completion,
                                           gboolean            popup_completion)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  popup_completion = popup_completion != FALSE;

  if (completion->popup_completion != popup_completion)
    {
      completion->popup_completion = popup_completion;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_COMPLETION]);
    }
}


/**
 * bobgui_entry_completion_get_popup_completion:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns whether the completions should be presented in a popup window.
 *
 * Returns: %TRUE if popup completion is turned on
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
gboolean
bobgui_entry_completion_get_popup_completion (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->popup_completion;
}

/**
 * bobgui_entry_completion_set_popup_set_width:
 * @completion: a `BobguiEntryCompletion`
 * @popup_set_width: %TRUE to make the width of the popup the same as the entry
 *
 * Sets whether the completion popup window will be resized to be the same
 * width as the entry.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_popup_set_width (BobguiEntryCompletion *completion,
                                          gboolean            popup_set_width)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  popup_set_width = popup_set_width != FALSE;

  if (completion->popup_set_width != popup_set_width)
    {
      completion->popup_set_width = popup_set_width;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_SET_WIDTH]);
    }
}

/**
 * bobgui_entry_completion_get_popup_set_width:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns whether the completion popup window will be resized to the
 * width of the entry.
 *
 * Returns: %TRUE if the popup window will be resized to the width of
 *   the entry
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
gboolean
bobgui_entry_completion_get_popup_set_width (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->popup_set_width;
}


/**
 * bobgui_entry_completion_set_popup_single_match:
 * @completion: a `BobguiEntryCompletion`
 * @popup_single_match: %TRUE if the popup should appear even for a single match
 *
 * Sets whether the completion popup window will appear even if there is
 * only a single match.
 *
 * You may want to set this to %FALSE if you
 * are using [property@Bobgui.EntryCompletion:inline-completion].
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_popup_single_match (BobguiEntryCompletion *completion,
                                             gboolean            popup_single_match)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  popup_single_match = popup_single_match != FALSE;

  if (completion->popup_single_match != popup_single_match)
    {
      completion->popup_single_match = popup_single_match;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_SINGLE_MATCH]);
    }
}

/**
 * bobgui_entry_completion_get_popup_single_match:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns whether the completion popup window will appear even if there is
 * only a single match.
 *
 * Returns: %TRUE if the popup window will appear regardless of the
 *    number of matches
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
gboolean
bobgui_entry_completion_get_popup_single_match (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->popup_single_match;
}

/**
 * bobgui_entry_completion_set_inline_selection:
 * @completion: a `BobguiEntryCompletion`
 * @inline_selection: %TRUE to do inline selection
 *
 * Sets whether it is possible to cycle through the possible completions
 * inside the entry.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_completion_set_inline_selection (BobguiEntryCompletion *completion,
                                           gboolean inline_selection)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion));

  inline_selection = inline_selection != FALSE;

  if (completion->inline_selection != inline_selection)
    {
      completion->inline_selection = inline_selection;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_INLINE_SELECTION]);
    }
}

/**
 * bobgui_entry_completion_get_inline_selection:
 * @completion: a `BobguiEntryCompletion`
 *
 * Returns %TRUE if inline-selection mode is turned on.
 *
 * Returns: %TRUE if inline-selection mode is on
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
gboolean
bobgui_entry_completion_get_inline_selection (BobguiEntryCompletion *completion)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_COMPLETION (completion), FALSE);

  return completion->inline_selection;
}


static int
bobgui_entry_completion_timeout (gpointer data)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (data);

  completion->completion_timeout = 0;

  if (completion->filter_model &&
      g_utf8_strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry)), -1)
      >= completion->minimum_key_length)
    {
      int matches;
      gboolean popup_single;

      bobgui_entry_completion_complete (completion);
      matches = bobgui_tree_model_iter_n_children (BOBGUI_TREE_MODEL (completion->filter_model), NULL);
      bobgui_tree_selection_unselect_all (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (completion->tree_view)));

      g_object_get (completion, "popup-single-match", &popup_single, NULL);
      if (matches > (popup_single ? 0: 1))
        {
          if (bobgui_widget_get_visible (completion->popup_window))
            _bobgui_entry_completion_resize_popup (completion);
          else
            bobgui_entry_completion_popup (completion);
        }
      else
        _bobgui_entry_completion_popdown (completion);
    }
  else if (bobgui_widget_get_visible (completion->popup_window))
    _bobgui_entry_completion_popdown (completion);
  return G_SOURCE_REMOVE;
}

static inline gboolean
keyval_is_cursor_move (guint keyval)
{
  if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up)
    return TRUE;

  if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down)
    return TRUE;

  if (keyval == GDK_KEY_Page_Up)
    return TRUE;

  if (keyval == GDK_KEY_Page_Down)
    return TRUE;

  return FALSE;
}

static gboolean
bobgui_entry_completion_key_pressed (BobguiEventControllerKey *controller,
                                  guint                  keyval,
                                  guint                  keycode,
                                  GdkModifierType        state,
                                  gpointer               user_data)
{
  int matches;
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (user_data);
  BobguiWidget *widget = completion->entry;
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (widget));

  if (!completion->popup_completion)
    return FALSE;

  if (keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_KP_Enter ||
      keyval == GDK_KEY_ISO_Enter ||
      keyval == GDK_KEY_Escape)
    {
      if (completion->completion_timeout)
        {
          g_source_remove (completion->completion_timeout);
          completion->completion_timeout = 0;
        }
    }

  if (!bobgui_widget_get_mapped (completion->popup_window))
    return FALSE;

  matches = bobgui_tree_model_iter_n_children (BOBGUI_TREE_MODEL (completion->filter_model), NULL);

  if (keyval_is_cursor_move (keyval))
    {
      BobguiTreePath *path = NULL;

      if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up)
        {
          if (completion->current_selected < 0)
            completion->current_selected = matches - 1;
          else
            completion->current_selected--;
        }
      else if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down)
        {
          if (completion->current_selected < matches - 1)
            completion->current_selected++;
          else
            completion->current_selected = -1;
        }
      else if (keyval == GDK_KEY_Page_Up)
        {
          if (completion->current_selected < 0)
            completion->current_selected = matches - 1;
          else if (completion->current_selected == 0)
            completion->current_selected = -1;
          else if (completion->current_selected < matches)
            {
              completion->current_selected -= PAGE_STEP;
              if (completion->current_selected < 0)
                completion->current_selected = 0;
            }
          else
            {
              completion->current_selected -= PAGE_STEP;
              if (completion->current_selected < matches - 1)
                completion->current_selected = matches - 1;
            }
        }
      else if (keyval == GDK_KEY_Page_Down)
        {
          if (completion->current_selected < 0)
            completion->current_selected = 0;
          else if (completion->current_selected < matches - 1)
            {
              completion->current_selected += PAGE_STEP;
              if (completion->current_selected > matches - 1)
                completion->current_selected = matches - 1;
            }
          else if (completion->current_selected == matches - 1)
            {
              completion->current_selected = -1;
            }
          else
            {
              completion->current_selected += PAGE_STEP;
              if (completion->current_selected > matches - 1)
                completion->current_selected = matches - 1;
            }
        }

      if (completion->current_selected < 0)
        {
          bobgui_tree_selection_unselect_all (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (completion->tree_view)));

          if (completion->inline_selection &&
              completion->completion_prefix)
            {
              bobgui_editable_set_text (BOBGUI_EDITABLE (completion->entry),
                                     completion->completion_prefix);
              bobgui_editable_set_position (BOBGUI_EDITABLE (widget), -1);
            }
        }
      else if (completion->current_selected < matches)
        {
          path = bobgui_tree_path_new_from_indices (completion->current_selected, -1);
          bobgui_tree_view_set_cursor (BOBGUI_TREE_VIEW (completion->tree_view),
                                    path, NULL, FALSE);

          if (completion->inline_selection)
            {

              BobguiTreeIter iter;
              BobguiTreeIter child_iter;
              BobguiTreeModel *model = NULL;
              BobguiTreeSelection *sel;
              gboolean entry_set;

              sel = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (completion->tree_view));
              if (!bobgui_tree_selection_get_selected (sel, &model, &iter))
                return FALSE;
             bobgui_tree_model_filter_convert_iter_to_child_iter (BOBGUI_TREE_MODEL_FILTER (model), &child_iter, &iter);
              model = bobgui_tree_model_filter_get_model (BOBGUI_TREE_MODEL_FILTER (model));

              if (completion->completion_prefix == NULL)
                completion->completion_prefix = g_strdup (bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry)));

              g_signal_emit_by_name (completion, "cursor-on-match", model,
                                     &child_iter, &entry_set);
            }
        }

      bobgui_tree_path_free (path);

      return TRUE;
    }
  else if (keyval == GDK_KEY_Escape ||
           keyval == GDK_KEY_Left ||
           keyval == GDK_KEY_KP_Left ||
           keyval == GDK_KEY_Right ||
           keyval == GDK_KEY_KP_Right)
    {
      gboolean retval = TRUE;

      bobgui_entry_reset_im_context (BOBGUI_ENTRY (widget));
      _bobgui_entry_completion_popdown (completion);

      if (completion->current_selected < 0)
        {
          retval = FALSE;
          goto keypress_completion_out;
        }
      else if (completion->inline_selection)
        {
          /* Escape rejects the tentative completion */
          if (keyval == GDK_KEY_Escape)
            {
              if (completion->completion_prefix)
                bobgui_editable_set_text (BOBGUI_EDITABLE (completion->entry),
                                       completion->completion_prefix);
              else
                bobgui_editable_set_text (BOBGUI_EDITABLE (completion->entry), "");
            }

          /* Move the cursor to the end for Right/Esc */
          if (keyval == GDK_KEY_Right ||
              keyval == GDK_KEY_KP_Right ||
              keyval == GDK_KEY_Escape)
            bobgui_editable_set_position (BOBGUI_EDITABLE (widget), -1);
          /* Let the default keybindings run for Left, i.e. either move to the
 *            * previous character or select word if a modifier is used */
          else
            retval = FALSE;
        }

keypress_completion_out:
      if (completion->inline_selection)
        g_clear_pointer (&completion->completion_prefix, g_free);

      return retval;
    }
  else if (keyval == GDK_KEY_Tab ||
           keyval == GDK_KEY_KP_Tab ||
           keyval == GDK_KEY_ISO_Left_Tab)
    {
      bobgui_entry_reset_im_context (BOBGUI_ENTRY (widget));
      _bobgui_entry_completion_popdown (completion);

      g_clear_pointer (&completion->completion_prefix, g_free);

      return FALSE;
    }
  else if (keyval == GDK_KEY_ISO_Enter ||
           keyval == GDK_KEY_KP_Enter ||
           keyval == GDK_KEY_Return)
    {
      BobguiTreeIter iter;
      BobguiTreeModel *model = NULL;
      BobguiTreeModel *child_model;
      BobguiTreeIter child_iter;
      BobguiTreeSelection *sel;
      gboolean retval = TRUE;

      bobgui_entry_reset_im_context (BOBGUI_ENTRY (widget));
      _bobgui_entry_completion_popdown (completion);

      if (completion->current_selected < matches)
        {
          gboolean entry_set;

          sel = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (completion->tree_view));
          if (bobgui_tree_selection_get_selected (sel, &model, &iter))
            {
              bobgui_tree_model_filter_convert_iter_to_child_iter (BOBGUI_TREE_MODEL_FILTER (model), &child_iter, &iter);
              child_model = bobgui_tree_model_filter_get_model (BOBGUI_TREE_MODEL_FILTER (model));
              g_signal_handler_block (text, completion->changed_id);
              g_signal_emit_by_name (completion, "match-selected",
                                     child_model, &child_iter, &entry_set);
              g_signal_handler_unblock (text, completion->changed_id);

              if (!entry_set)
                {
                  char *str = NULL;

                  bobgui_tree_model_get (model, &iter,
                                      completion->text_column, &str,
                                      -1);

                  bobgui_editable_set_text (BOBGUI_EDITABLE (widget), str);

                  /* move the cursor to the end */
                  bobgui_editable_set_position (BOBGUI_EDITABLE (widget), -1);
                  g_free (str);
                }
            }
          else
            retval = FALSE;
        }

      g_clear_pointer (&completion->completion_prefix, g_free);

      return retval;
    }

  g_clear_pointer (&completion->completion_prefix, g_free);

  return FALSE;
}

static void
bobgui_entry_completion_changed (BobguiWidget *widget,
                              gpointer   user_data)
{
  BobguiEntryCompletion *completion = BOBGUI_ENTRY_COMPLETION (user_data);

  if (!completion->popup_completion)
    return;

  /* (re)install completion timeout */
  if (completion->completion_timeout)
    {
      g_source_remove (completion->completion_timeout);
      completion->completion_timeout = 0;
    }

  if (!bobgui_editable_get_text (BOBGUI_EDITABLE (widget)))
    return;

  /* no need to normalize for this test */
  if (completion->minimum_key_length > 0 &&
      strcmp ("", bobgui_editable_get_text (BOBGUI_EDITABLE (widget))) == 0)
    {
      if (bobgui_widget_get_visible (completion->popup_window))
        _bobgui_entry_completion_popdown (completion);
      return;
    }

  completion->completion_timeout =
    g_timeout_add (COMPLETION_TIMEOUT,
                   bobgui_entry_completion_timeout,
                   completion);
  gdk_source_set_static_name_by_id (completion->completion_timeout, "[bobgui] bobgui_entry_completion_timeout");
}

static gboolean
check_completion_callback (BobguiEntryCompletion *completion)
{
  completion->check_completion_idle = NULL;

  bobgui_entry_completion_complete (completion);
  bobgui_entry_completion_insert_prefix (completion);

  return FALSE;
}

static void
clear_completion_callback (GObject            *text,
                           GParamSpec         *pspec,
                           BobguiEntryCompletion *completion)
{
  if (!completion->inline_completion)
    return;

  if (pspec->name == I_("cursor-position") ||
      pspec->name == I_("selection-bound"))
    completion->has_completion = FALSE;
}

static gboolean
accept_completion_callback (BobguiEntryCompletion *completion)
{
  if (!completion->inline_completion)
    return FALSE;

  if (completion->has_completion)
    bobgui_editable_set_position (BOBGUI_EDITABLE (completion->entry),
                               bobgui_entry_buffer_get_length (bobgui_entry_get_buffer (BOBGUI_ENTRY (completion->entry))));

  return FALSE;
}

static void
text_focus_out (BobguiEntryCompletion *completion)
{
  if (!bobgui_widget_get_mapped (completion->popup_window))
    accept_completion_callback (completion);
}

static void
completion_inserted_text_callback (BobguiEntryBuffer     *buffer,
                                   guint               position,
                                   const char         *text,
                                   guint               length,
                                   BobguiEntryCompletion *completion)
{
  if (!completion->inline_completion)
    return;

  /* idle to update the selection based on the file list */
  if (completion->check_completion_idle == NULL)
    {
      completion->check_completion_idle = g_idle_source_new ();
      g_source_set_priority (completion->check_completion_idle, G_PRIORITY_HIGH);
      g_source_set_closure (completion->check_completion_idle,
                            g_cclosure_new_object (G_CALLBACK (check_completion_callback),
                                                   G_OBJECT (completion)));
      g_source_attach (completion->check_completion_idle, NULL);
      g_source_set_static_name (completion->check_completion_idle, "[bobgui] check_completion_callback");
    }
}

static void
connect_completion_signals (BobguiEntryCompletion *completion)
{
  BobguiEventController *controller;
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  controller = completion->entry_key_controller = bobgui_event_controller_key_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-entry-completion");
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_entry_completion_key_pressed), completion);
  bobgui_widget_add_controller (BOBGUI_WIDGET (text), controller);
  controller = completion->entry_focus_controller = bobgui_event_controller_focus_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-entry-completion");
  g_signal_connect_swapped (controller, "leave", G_CALLBACK (text_focus_out), completion);
  bobgui_widget_add_controller (BOBGUI_WIDGET (text), controller);

  completion->changed_id =
    g_signal_connect (text, "changed", G_CALLBACK (bobgui_entry_completion_changed), completion);

  completion->insert_text_signal_group = g_signal_group_new (BOBGUI_TYPE_ENTRY_BUFFER);
  g_signal_group_connect (completion->insert_text_signal_group, "inserted-text", G_CALLBACK (completion_inserted_text_callback), completion);
  g_object_bind_property (text, "buffer", completion->insert_text_signal_group, "target", G_BINDING_SYNC_CREATE);

  g_signal_connect (text, "notify", G_CALLBACK (clear_completion_callback), completion);
   g_signal_connect_swapped (text, "activate", G_CALLBACK (accept_completion_callback), completion);
}

static void
disconnect_completion_signals (BobguiEntryCompletion *completion)
{
  BobguiText *text = bobgui_entry_get_text_widget (BOBGUI_ENTRY (completion->entry));

  bobgui_widget_remove_controller (BOBGUI_WIDGET (text), completion->entry_key_controller);
  bobgui_widget_remove_controller (BOBGUI_WIDGET (text), completion->entry_focus_controller);

  if (completion->changed_id > 0 &&
      g_signal_handler_is_connected (text, completion->changed_id))
    {
      g_signal_handler_disconnect (text, completion->changed_id);
      completion->changed_id = 0;
    }

  g_clear_object (&completion->insert_text_signal_group);

  g_signal_handlers_disconnect_by_func (text, G_CALLBACK (clear_completion_callback), completion);
  g_signal_handlers_disconnect_by_func (text, G_CALLBACK (accept_completion_callback), completion);
}

void
_bobgui_entry_completion_disconnect (BobguiEntryCompletion *completion)
{
  if (completion->completion_timeout)
    {
      g_source_remove (completion->completion_timeout);
      completion->completion_timeout = 0;
    }
  if (completion->check_completion_idle)
    {
      g_source_destroy (completion->check_completion_idle);
      completion->check_completion_idle = NULL;
    }

  if (bobgui_widget_get_mapped (completion->popup_window))
    _bobgui_entry_completion_popdown (completion);

  disconnect_completion_signals (completion);

  bobgui_widget_unparent (completion->popup_window);

  completion->entry = NULL;
}

void
_bobgui_entry_completion_connect (BobguiEntryCompletion *completion,
                               BobguiEntry           *entry)
{
  completion->entry = BOBGUI_WIDGET (entry);

  bobgui_widget_set_parent (completion->popup_window, BOBGUI_WIDGET (entry));

  connect_completion_signals (completion);
}
