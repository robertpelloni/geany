/* bobguicombobox.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@bobgui.org>
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

#include "config.h"

#include "bobguicomboboxprivate.h"

#include "bobguibox.h"
#include "bobguicellareabox.h"
#include "bobguicelllayout.h"
#include "bobguicellrenderertext.h"
#include "bobguicellview.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguiframe.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguiliststore.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguishortcutcontroller.h"
#include "bobguitogglebutton.h"
#include "bobguitreepopoverprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

#include <gobject/gvaluecollector.h>
#include <string.h>
#include <stdarg.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiComboBox:
 *
 * A `BobguiComboBox` is a widget that allows the user to choose from a list of
 * valid choices.
 *
 * <picture>
 *   <source srcset="combo-box-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiComboBox" src="combo-box.png">
 * </picture>
 *
 * The `BobguiComboBox` displays the selected choice; when activated, the
 * `BobguiComboBox` displays a popup which allows the user to make a new choice.
 *
 * The `BobguiComboBox` uses the model-view pattern; the list of valid choices
 * is specified in the form of a tree model, and the display of the choices
 * can be adapted to the data in the model by using cell renderers, as you
 * would in a tree view. This is possible since `BobguiComboBox` implements the
 * [iface@Bobgui.CellLayout] interface. The tree model holding the valid
 * choices is not restricted to a flat list, it can be a real tree, and the
 * popup will reflect the tree structure.
 *
 * To allow the user to enter values not in the model, the
 * [property@Bobgui.ComboBox:has-entry] property allows the `BobguiComboBox` to
 * contain a [class@Bobgui.Entry]. This entry can be accessed by calling
 * [method@Bobgui.ComboBox.get_child] on the combo box.
 *
 * For a simple list of textual choices, the model-view API of `BobguiComboBox`
 * can be a bit overwhelming. In this case, [class@Bobgui.ComboBoxText] offers
 * a simple alternative. Both `BobguiComboBox` and `BobguiComboBoxText` can contain
 * an entry.
 *
 * ## CSS nodes
 *
 * ```
 * combobox
 * ├── box.linked
 * │   ╰── button.combo
 * │       ╰── box
 * │           ├── cellview
 * │           ╰── arrow
 * ╰── window.popup
 * ```
 *
 * A normal combobox contains a box with the .linked class, a button
 * with the .combo class and inside those buttons, there are a cellview and
 * an arrow.
 *
 * ```
 * combobox
 * ├── box.linked
 * │   ├── entry.combo
 * │   ╰── button.combo
 * │       ╰── box
 * │           ╰── arrow
 * ╰── window.popup
 * ```
 *
 * A `BobguiComboBox` with an entry has a single CSS node with name combobox.
 * It contains a box with the .linked class. That box contains an entry and
 * a button, both with the .combo class added. The button also contains another
 * node with name arrow.
 *
 * ## Accessibility
 *
 * `BobguiComboBox` uses the [enum@Bobgui.AccessibleRole.combo_box] role.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown] instead
 */

typedef struct
{
  BobguiWidget *child;

  BobguiTreeModel *model;

  BobguiCellArea *area;

  int active; /* Only temporary */
  BobguiTreeRowReference *active_row;

  BobguiWidget *cell_view;

  BobguiWidget *box;
  BobguiWidget *button;
  BobguiWidget *arrow;

  BobguiWidget *popup_widget;

  guint popup_idle_id;
  guint scroll_timer;
  guint resize_idle_id;

  /* For "has-entry" specific behavior we track
   * an automated cell renderer and text column
   */
  int   text_column;
  BobguiCellRenderer *text_renderer;

  int id_column;

  guint popup_in_progress : 1;
  guint popup_shown : 1;
  guint has_frame : 1;
  guint is_cell_renderer : 1;
  guint editing_canceled : 1;
  guint auto_scroll : 1;
  guint button_sensitivity : 2;
  guint has_entry : 1;
  guint popup_fixed_width : 1;

  BobguiTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GDestroyNotify              row_separator_destroy;
} BobguiComboBoxPrivate;

/* There are 2 modes to this widget, which can be characterized as follows:
 *
 * 1) no child added:
 *
 * cell_view -> BobguiCellView, regular child
 * button -> BobguiToggleButton set_parent to combo
 * arrow -> BobguiArrow set_parent to button
 * popup_widget -> BobguiMenu
 *
 * 2) child added:
 *
 * cell_view -> NULL
 * button -> BobguiToggleButton set_parent to combo
 * arrow -> BobguiArrow, child of button
 * popup_widget -> BobguiMenu
 */

enum {
  ACTIVATE,
  CHANGED,
  MOVE_ACTIVE,
  POPUP,
  POPDOWN,
  FORMAT_ENTRY_TEXT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_ACTIVE,
  PROP_HAS_FRAME,
  PROP_POPUP_SHOWN,
  PROP_BUTTON_SENSITIVITY,
  PROP_EDITING_CANCELED,
  PROP_HAS_ENTRY,
  PROP_ENTRY_TEXT_COLUMN,
  PROP_POPUP_FIXED_WIDTH,
  PROP_ID_COLUMN,
  PROP_ACTIVE_ID,
  PROP_CHILD
};

static guint combo_box_signals[LAST_SIGNAL] = {0,};

/* common */

static void     bobgui_combo_box_cell_layout_init     (BobguiCellLayoutIface *iface);
static void     bobgui_combo_box_cell_editable_init   (BobguiCellEditableIface *iface);
static void     bobgui_combo_box_constructed          (GObject          *object);
static void     bobgui_combo_box_dispose              (GObject          *object);
static void     bobgui_combo_box_unmap                (BobguiWidget        *widget);

static void     bobgui_combo_box_set_property         (GObject         *object,
                                                    guint            prop_id,
                                                    const GValue    *value,
                                                    GParamSpec      *spec);
static void     bobgui_combo_box_get_property         (GObject         *object,
                                                    guint            prop_id,
                                                    GValue          *value,
                                                    GParamSpec      *spec);

static gboolean bobgui_combo_box_grab_focus           (BobguiWidget       *widget);
static void     bobgui_combo_box_button_toggled       (BobguiWidget       *widget,
                                                    gpointer         data);

static void     bobgui_combo_box_menu_show            (BobguiWidget        *menu,
                                                    gpointer          user_data);
static void     bobgui_combo_box_menu_hide            (BobguiWidget        *menu,
                                                    gpointer          user_data);

static void     bobgui_combo_box_unset_model          (BobguiComboBox      *combo_box);

static void     bobgui_combo_box_set_active_internal  (BobguiComboBox      *combo_box,
                                                    BobguiTreePath      *path);

static void     bobgui_combo_box_real_move_active     (BobguiComboBox      *combo_box,
                                                    BobguiScrollType     scroll);
static void     bobgui_combo_box_real_popup           (BobguiComboBox      *combo_box);
static gboolean bobgui_combo_box_real_popdown         (BobguiComboBox      *combo_box);

static gboolean bobgui_combo_box_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                                        double                    dx,
                                                        double                    dy,
                                                        BobguiComboBox              *combo_box);

/* listening to the model */
static void     bobgui_combo_box_model_row_inserted   (BobguiTreeModel     *model,
                                                    BobguiTreePath      *path,
                                                    BobguiTreeIter      *iter,
                                                    gpointer          user_data);
static void     bobgui_combo_box_model_row_deleted    (BobguiTreeModel     *model,
                                                    BobguiTreePath      *path,
                                                    gpointer          user_data);
static void     bobgui_combo_box_model_rows_reordered (BobguiTreeModel     *model,
                                                    BobguiTreePath      *path,
                                                    BobguiTreeIter      *iter,
                                                    int              *new_order,
                                                    gpointer          user_data);
static void     bobgui_combo_box_model_row_changed    (BobguiTreeModel     *model,
                                                    BobguiTreePath      *path,
                                                    BobguiTreeIter      *iter,
                                                    gpointer          data);

static void     bobgui_combo_box_menu_activate        (BobguiWidget        *menu,
                                                    const char       *path,
                                                    BobguiComboBox      *combo_box);
static void     bobgui_combo_box_update_sensitivity   (BobguiComboBox      *combo_box);
static gboolean bobgui_combo_box_menu_key (BobguiEventControllerKey *key,
                                        guint                  keyval,
                                        guint                  keycode,
                                        GdkModifierType        modifiers,
                                        BobguiComboBox           *combo_box);
static void     bobgui_combo_box_menu_popup           (BobguiComboBox      *combo_box);

/* cell layout */
static BobguiCellArea *bobgui_combo_box_cell_layout_get_area       (BobguiCellLayout    *cell_layout);

static gboolean bobgui_combo_box_mnemonic_activate              (BobguiWidget    *widget,
                                                              gboolean      group_cycling);

static void     bobgui_combo_box_child_show                     (BobguiWidget       *widget,
                                                              BobguiComboBox     *combo_box);
static void     bobgui_combo_box_child_hide                     (BobguiWidget       *widget,
                                                              BobguiComboBox     *combo_box);

/* BobguiComboBox:has-entry callbacks */
static void     bobgui_combo_box_entry_contents_changed         (BobguiEntry        *entry,
                                                              gpointer         user_data);
static void     bobgui_combo_box_entry_active_changed           (BobguiComboBox     *combo_box,
                                                              gpointer         user_data);
static char    *bobgui_combo_box_format_entry_text              (BobguiComboBox     *combo_box,
                                                              const char      *path);

/* BobguiBuildable method implementation */
static BobguiBuildableIface *parent_buildable_iface;

static void     bobgui_combo_box_buildable_init                 (BobguiBuildableIface  *iface);
static void     bobgui_combo_box_buildable_add_child            (BobguiBuildable       *buildable,
                                                              BobguiBuilder         *builder,
                                                              GObject            *child,
                                                              const char         *type);
static gboolean bobgui_combo_box_buildable_custom_tag_start     (BobguiBuildable       *buildable,
                                                              BobguiBuilder         *builder,
                                                              GObject            *child,
                                                              const char         *tagname,
                                                              BobguiBuildableParser *parser,
                                                              gpointer           *data);
static void     bobgui_combo_box_buildable_custom_tag_end       (BobguiBuildable       *buildable,
                                                              BobguiBuilder         *builder,
                                                              GObject            *child,
                                                              const char         *tagname,
                                                              gpointer            data);
static GObject *bobgui_combo_box_buildable_get_internal_child   (BobguiBuildable       *buildable,
                                                              BobguiBuilder         *builder,
                                                              const char         *childname);



/* BobguiCellEditable method implementations */
static void     bobgui_combo_box_start_editing                  (BobguiCellEditable *cell_editable,
                                                              GdkEvent        *event);

G_DEFINE_TYPE_WITH_CODE (BobguiComboBox, bobgui_combo_box, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiComboBox)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_LAYOUT,
                                                bobgui_combo_box_cell_layout_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_EDITABLE,
                                                bobgui_combo_box_cell_editable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_combo_box_buildable_init))


/* common */
static void
bobgui_combo_box_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (widget);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  bobgui_widget_measure (priv->box,
                      orientation,
                      size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_combo_box_activate (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  bobgui_widget_activate (priv->button);
}

static void
bobgui_combo_box_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (widget);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  bobgui_widget_size_allocate (priv->box,
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            }, baseline);

  bobgui_widget_set_size_request (priv->popup_widget, width, -1);
  bobgui_widget_queue_resize (priv->popup_widget);

  bobgui_popover_present (BOBGUI_POPOVER (priv->popup_widget));
}

static void
bobgui_combo_box_compute_expand (BobguiWidget *widget,
                              gboolean  *hexpand,
                              gboolean  *vexpand)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (widget);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiWidget *child = priv->child;

  if (child && child != priv->cell_view)
    {
      *hexpand = bobgui_widget_compute_expand (child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
bobgui_combo_box_class_init (BobguiComboBoxClass *klass)
{
  GObjectClass *object_class;
  BobguiWidgetClass *widget_class;

  widget_class = (BobguiWidgetClass *)klass;
  widget_class->mnemonic_activate = bobgui_combo_box_mnemonic_activate;
  widget_class->grab_focus = bobgui_combo_box_grab_focus;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->measure = bobgui_combo_box_measure;
  widget_class->size_allocate = bobgui_combo_box_size_allocate;
  widget_class->unmap = bobgui_combo_box_unmap;
  widget_class->compute_expand = bobgui_combo_box_compute_expand;

  object_class = (GObjectClass *)klass;
  object_class->constructed = bobgui_combo_box_constructed;
  object_class->dispose = bobgui_combo_box_dispose;
  object_class->set_property = bobgui_combo_box_set_property;
  object_class->get_property = bobgui_combo_box_get_property;

  klass->activate = bobgui_combo_box_activate;
  klass->format_entry_text = bobgui_combo_box_format_entry_text;

  /* signals */
  /**
   * BobguiComboBox::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the combo box is activated.
   *
   * The `::activate` signal on `BobguiComboBox` is an action signal and
   * emitting it causes the combo box to pop up its dropdown.
   *
   * Since: 4.6
   */
  combo_box_signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiComboBoxClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, combo_box_signals[ACTIVATE]);

  /**
   * BobguiComboBox::changed:
   * @widget: the object which received the signal
   *
   * Emitted when the active item is changed.
   *
   * The can be due to the user selecting a different item from the list,
   * or due to a call to [method@Bobgui.ComboBox.set_active_iter]. It will
   * also be emitted while typing into the entry of a combo box with an entry.
   */
  combo_box_signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiComboBoxClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiComboBox::move-active:
   * @widget: the object that received the signal
   * @scroll_type: a `BobguiScrollType`
   *
   * Emitted to move the active selection.
   *
   * This is an [keybinding signal](class.SignalAction.html).
   */
  combo_box_signals[MOVE_ACTIVE] =
    g_signal_new_class_handler (I_("move-active"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_combo_box_real_move_active),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                BOBGUI_TYPE_SCROLL_TYPE);

  /**
   * BobguiComboBox::popup:
   * @widget: the object that received the signal
   *
   * Emitted to popup the combo box list.
   *
   * This is an [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is Alt+Down.
   */
  combo_box_signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_combo_box_real_popup),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);
  /**
   * BobguiComboBox::popdown:
   * @button: the object which received the signal
   *
   * Emitted to popdown the combo box list.
   *
   * This is an [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are Alt+Up and Escape.
   *
   * Returns: whether the combo box was popped down
   */
  combo_box_signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_combo_box_real_popdown),
                                NULL, NULL,
                                _bobgui_marshal_BOOLEAN__VOID,
                                G_TYPE_BOOLEAN, 0);

  /**
   * BobguiComboBox::format-entry-text:
   * @combo: the object which received the signal
   * @path: the [struct@Bobgui.TreePath] string from the combo box's current model
   *   to format text for
   *
   * Emitted to allow changing how the text in a combo box's entry is displayed.
   *
   * See [property@Bobgui.ComboBox:has-entry].
   *
   * Connect a signal handler which returns an allocated string representing
   * @path. That string will then be used to set the text in the combo box's
   * entry. The default signal handler uses the text from the
   * [property@Bobgui.ComboBox:entry-text-column] model column.
   *
   * Here's an example signal handler which fetches data from the model and
   * displays it in the entry.
   * ```c
   * static char *
   * format_entry_text_callback (BobguiComboBox *combo,
   *                             const char *path,
   *                             gpointer     user_data)
   * {
   *   BobguiTreeIter iter;
   *   BobguiTreeModel model;
   *   double       value;
   *
   *   model = bobgui_combo_box_get_model (combo);
   *
   *   bobgui_tree_model_get_iter_from_string (model, &iter, path);
   *   bobgui_tree_model_get (model, &iter,
   *                       THE_DOUBLE_VALUE_COLUMN, &value,
   *                       -1);
   *
   *   return g_strdup_printf ("%g", value);
   * }
   * ```
   *
   * Returns: (transfer full): a newly allocated string representing @path
   *   for the current `BobguiComboBox` model.
   */
  combo_box_signals[FORMAT_ENTRY_TEXT] =
    g_signal_new (I_("format-entry-text"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiComboBoxClass, format_entry_text),
                  _bobgui_single_string_accumulator, NULL,
                  _bobgui_marshal_STRING__STRING,
                  G_TYPE_STRING, 1, G_TYPE_STRING);

  /* key bindings */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Down, GDK_ALT_MASK,
                                       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Down, GDK_ALT_MASK,
                                       "popup",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Up, GDK_ALT_MASK,
                                       "popdown",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Up, GDK_ALT_MASK,
                                       "popdown",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "popdown",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Up, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_STEP_UP);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Up, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_STEP_UP);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Up, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_PAGE_UP);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Page_Up, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_PAGE_UP);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Home, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_START);
  bobgui_widget_class_add_binding_signal (widget_class,
                                      GDK_KEY_KP_Home, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_START);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Down, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_STEP_DOWN);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Down, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_STEP_DOWN);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Down, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_PAGE_DOWN);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Page_Down, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_PAGE_DOWN);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_End, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_END);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_End, 0,
                                       "move-active",
                                       "(i)", BOBGUI_SCROLL_END);

  /* properties */
  g_object_class_override_property (object_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  /**
   * BobguiComboBox:model:
   *
   * The model from which the combo box takes its values.
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
                                                        BOBGUI_TYPE_TREE_MODEL,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * BobguiComboBox:active:
   *
   * The item which is currently active.
   *
   * If the model is a non-flat treemodel, and the active item is not an
   * immediate child of the root of the tree, this property has the value
   * `bobgui_tree_path_get_indices (path)[0]`, where `path` is the
   * [struct@Bobgui.TreePath] of the active item.
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active", NULL, NULL,
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiComboBox:has-frame:
   *
   * The `has-frame` property controls whether a frame is drawn around the entry.
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_FRAME,
                                   g_param_spec_boolean ("has-frame", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiComboBox:popup-shown:
   *
   * Whether the combo boxes dropdown is popped up.
   *
   * Note that this property is mainly useful, because
   * it allows you to connect to notify::popup-shown.
   */
  g_object_class_install_property (object_class,
                                   PROP_POPUP_SHOWN,
                                   g_param_spec_boolean ("popup-shown", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READABLE));


   /**
    * BobguiComboBox:button-sensitivity:
    *
    * Whether the dropdown button is sensitive when
    * the model is empty.
    */
   g_object_class_install_property (object_class,
                                    PROP_BUTTON_SENSITIVITY,
                                    g_param_spec_enum ("button-sensitivity", NULL, NULL,
                                                       BOBGUI_TYPE_SENSITIVITY_TYPE,
                                                       BOBGUI_SENSITIVITY_AUTO,
                                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * BobguiComboBox:has-entry:
    *
    * Whether the combo box has an entry.
    */
   g_object_class_install_property (object_class,
                                    PROP_HAS_ENTRY,
                                    g_param_spec_boolean ("has-entry", NULL, NULL,
                                                          FALSE,
                                                          BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

   /**
    * BobguiComboBox:entry-text-column:
    *
    * The model column to associate with strings from the entry.
    *
    * This is property only relevant if the combo was created with
    * [property@Bobgui.ComboBox:has-entry] is %TRUE.
    */
   g_object_class_install_property (object_class,
                                    PROP_ENTRY_TEXT_COLUMN,
                                    g_param_spec_int ("entry-text-column", NULL, NULL,
                                                      -1, G_MAXINT, -1,
                                                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * BobguiComboBox:id-column:
    *
    * The model column that provides string IDs for the values
    * in the model, if != -1.
    */
   g_object_class_install_property (object_class,
                                    PROP_ID_COLUMN,
                                    g_param_spec_int ("id-column", NULL, NULL,
                                                      -1, G_MAXINT, -1,
                                                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * BobguiComboBox:active-id:
    *
    * The value of the ID column of the active row.
    */
   g_object_class_install_property (object_class,
                                    PROP_ACTIVE_ID,
                                    g_param_spec_string ("active-id", NULL, NULL,
                                                         NULL,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * BobguiComboBox:popup-fixed-width:
    *
    * Whether the popup's width should be a fixed width matching the
    * allocated width of the combo box.
    */
   g_object_class_install_property (object_class,
                                    PROP_POPUP_FIXED_WIDTH,
                                    g_param_spec_boolean ("popup-fixed-width", NULL, NULL,
                                                          TRUE,
                                                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * BobguiComboBox:child:
    *
    * The child widget.
    */
   g_object_class_install_property (object_class,
                                    PROP_CHILD,
                                    g_param_spec_object ("child", NULL, NULL,
                                                         BOBGUI_TYPE_WIDGET,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguicombobox.ui");
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiComboBox, box);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiComboBox, button);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiComboBox, arrow);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiComboBox, area);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiComboBox, popup_widget);
  bobgui_widget_class_bind_template_callback (widget_class, bobgui_combo_box_button_toggled);
  bobgui_widget_class_bind_template_callback (widget_class, bobgui_combo_box_menu_activate);
  bobgui_widget_class_bind_template_callback (widget_class, bobgui_combo_box_menu_key);
  bobgui_widget_class_bind_template_callback (widget_class, bobgui_combo_box_menu_show);
  bobgui_widget_class_bind_template_callback (widget_class, bobgui_combo_box_menu_hide);

  bobgui_widget_class_set_css_name (widget_class, I_("combobox"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX);
}

static void
bobgui_combo_box_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_combo_box_buildable_add_child;
  iface->custom_tag_start = bobgui_combo_box_buildable_custom_tag_start;
  iface->custom_tag_end = bobgui_combo_box_buildable_custom_tag_end;
  iface->get_internal_child = bobgui_combo_box_buildable_get_internal_child;
}

static void
bobgui_combo_box_cell_layout_init (BobguiCellLayoutIface *iface)
{
  iface->get_area = bobgui_combo_box_cell_layout_get_area;
}

static void
bobgui_combo_box_cell_editable_init (BobguiCellEditableIface *iface)
{
  iface->start_editing = bobgui_combo_box_start_editing;
}

static gboolean
bobgui_combo_box_row_separator_func (BobguiTreeModel      *model,
                                  BobguiTreeIter       *iter,
                                  BobguiComboBox       *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->row_separator_func)
    return priv->row_separator_func (model, iter, priv->row_separator_data);

  return FALSE;
}

static void
bobgui_combo_box_init (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiEventController *controller;
  BobguiEventController **controllers;
  guint n_controllers, i;

  priv->active = -1;
  priv->active_row = NULL;

  priv->popup_shown = FALSE;
  priv->has_frame = TRUE;
  priv->is_cell_renderer = FALSE;
  priv->editing_canceled = FALSE;
  priv->auto_scroll = FALSE;
  priv->button_sensitivity = BOBGUI_SENSITIVITY_AUTO;
  priv->has_entry = FALSE;
  priv->popup_fixed_width = TRUE;

  priv->text_column = -1;
  priv->text_renderer = NULL;
  priv->id_column = -1;

  g_type_ensure (BOBGUI_TYPE_BUILTIN_ICON);
  g_type_ensure (BOBGUI_TYPE_TREE_POPOVER);
  bobgui_widget_init_template (BOBGUI_WIDGET (combo_box));

  bobgui_widget_remove_css_class (priv->button, "toggle");
  bobgui_widget_add_css_class (priv->button, "combo");

  bobgui_tree_popover_set_row_separator_func (BOBGUI_TREE_POPOVER (priv->popup_widget),
                                           (BobguiTreeViewRowSeparatorFunc)bobgui_combo_box_row_separator_func,
                                           combo_box, NULL);

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL |
                                                BOBGUI_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (bobgui_combo_box_scroll_controller_scroll),
                    combo_box);
  bobgui_widget_add_controller (BOBGUI_WIDGET (combo_box), controller);

  controllers = bobgui_widget_list_controllers (priv->popup_widget, BOBGUI_PHASE_BUBBLE, &n_controllers);
  for (i = 0; i < n_controllers; i ++)
    {
      controller = controllers[i];

      if (BOBGUI_IS_SHORTCUT_CONTROLLER (controller))
        {
          g_object_ref (controller);
          bobgui_widget_remove_controller (priv->popup_widget, controller);
          bobgui_widget_add_controller (priv->popup_widget, controller);
          break;
        }
    }
  g_free (controllers);
}

static void
bobgui_combo_box_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (object);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  switch (prop_id)
    {
    case PROP_MODEL:
      bobgui_combo_box_set_model (combo_box, g_value_get_object (value));
      break;

    case PROP_ACTIVE:
      bobgui_combo_box_set_active (combo_box, g_value_get_int (value));
      break;

    case PROP_HAS_FRAME:
      if (priv->has_frame != g_value_get_boolean (value))
        {
          priv->has_frame = g_value_get_boolean (value);
          if (priv->has_entry)
            bobgui_entry_set_has_frame (BOBGUI_ENTRY (priv->child), priv->has_frame);
          g_object_notify (object, "has-frame");
        }
      break;

    case PROP_POPUP_SHOWN:
      if (g_value_get_boolean (value))
        bobgui_combo_box_popup (combo_box);
      else
        bobgui_combo_box_popdown (combo_box);
      break;

    case PROP_BUTTON_SENSITIVITY:
      bobgui_combo_box_set_button_sensitivity (combo_box,
                                            g_value_get_enum (value));
      break;

    case PROP_POPUP_FIXED_WIDTH:
      bobgui_combo_box_set_popup_fixed_width (combo_box,
                                           g_value_get_boolean (value));
      break;

    case PROP_EDITING_CANCELED:
      if (priv->editing_canceled != g_value_get_boolean (value))
        {
          priv->editing_canceled = g_value_get_boolean (value);
          g_object_notify (object, "editing-canceled");
        }
      break;

    case PROP_HAS_ENTRY:
      priv->has_entry = g_value_get_boolean (value);
      break;

    case PROP_ENTRY_TEXT_COLUMN:
      bobgui_combo_box_set_entry_text_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ID_COLUMN:
      bobgui_combo_box_set_id_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ACTIVE_ID:
      bobgui_combo_box_set_active_id (combo_box, g_value_get_string (value));
      break;

    case PROP_CHILD:
      bobgui_combo_box_set_child (combo_box, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_combo_box_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (object);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value, priv->model);
        break;

      case PROP_ACTIVE:
        g_value_set_int (value, bobgui_combo_box_get_active (combo_box));
        break;

      case PROP_HAS_FRAME:
        g_value_set_boolean (value, priv->has_frame);
        break;

      case PROP_POPUP_SHOWN:
        g_value_set_boolean (value, priv->popup_shown);
        break;

      case PROP_BUTTON_SENSITIVITY:
        g_value_set_enum (value, priv->button_sensitivity);
        break;

      case PROP_POPUP_FIXED_WIDTH:
        g_value_set_boolean (value, priv->popup_fixed_width);
        break;

      case PROP_EDITING_CANCELED:
        g_value_set_boolean (value, priv->editing_canceled);
        break;

      case PROP_HAS_ENTRY:
        g_value_set_boolean (value, priv->has_entry);
        break;

      case PROP_ENTRY_TEXT_COLUMN:
        g_value_set_int (value, priv->text_column);
        break;

      case PROP_ID_COLUMN:
        g_value_set_int (value, priv->id_column);
        break;

      case PROP_ACTIVE_ID:
        g_value_set_string (value, bobgui_combo_box_get_active_id (combo_box));
        break;

      case PROP_CHILD:
        g_value_set_object (value, bobgui_combo_box_get_child (combo_box));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
bobgui_combo_box_button_toggled (BobguiWidget *widget,
                              gpointer   data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (data);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)))
    {
      if (!priv->popup_in_progress)
        bobgui_combo_box_popup (combo_box);
    }
  else
    {
      bobgui_combo_box_popdown (combo_box);
    }
}

static void
bobgui_combo_box_create_child (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->has_entry)
    {
      BobguiWidget *entry;

      entry = bobgui_entry_new ();
      bobgui_combo_box_set_child (combo_box, entry);

      bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "combo");

      g_signal_connect (combo_box, "changed",
                        G_CALLBACK (bobgui_combo_box_entry_active_changed), NULL);
    }
  else
    {
      priv->cell_view = bobgui_cell_view_new_with_context (priv->area, NULL);
      bobgui_widget_set_hexpand (priv->cell_view, TRUE);
      bobgui_cell_view_set_fit_model (BOBGUI_CELL_VIEW (priv->cell_view), TRUE);
      bobgui_cell_view_set_model (BOBGUI_CELL_VIEW (priv->cell_view), priv->model);
      bobgui_box_insert_child_after (BOBGUI_BOX (bobgui_widget_get_parent (priv->arrow)), priv->cell_view, NULL);
      priv->child = priv->cell_view;
    }
}

static void
bobgui_combo_box_add (BobguiComboBox *combo_box,
                   BobguiWidget    *widget)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->box == NULL)
    {
      bobgui_widget_set_parent (widget, BOBGUI_WIDGET (combo_box));
      return;
    }

  if (priv->has_entry && !BOBGUI_IS_ENTRY (widget))
    {
      g_warning ("Attempting to add a widget with type %s to a BobguiComboBox that needs an entry "
                 "(need an instance of BobguiEntry or of a subclass)",
                 G_OBJECT_TYPE_NAME (widget));
      return;
    }

  g_clear_pointer (&priv->cell_view, bobgui_widget_unparent);

  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_insert_child_after (BOBGUI_BOX (priv->box), widget, NULL);

  priv->child = widget;

  if (priv->has_entry)
    {
      g_signal_connect (widget, "changed",
                        G_CALLBACK (bobgui_combo_box_entry_contents_changed),
                        combo_box);

      bobgui_entry_set_has_frame (BOBGUI_ENTRY (widget), priv->has_frame);
    }
}

static void
bobgui_combo_box_remove (BobguiComboBox *combo_box,
                      BobguiWidget   *widget)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreePath *path;

  if (priv->has_entry)
    {
      if (widget && widget == priv->child)
        g_signal_handlers_disconnect_by_func (widget,
                                              bobgui_combo_box_entry_contents_changed,
                                              combo_box);
    }

  bobgui_box_remove (BOBGUI_BOX (priv->box), widget);

  priv->child = NULL;

  if (bobgui_widget_in_destruction (BOBGUI_WIDGET (combo_box)))
    return;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (combo_box));

  bobgui_combo_box_create_child (combo_box);

  if (bobgui_tree_row_reference_valid (priv->active_row))
    {
      path = bobgui_tree_row_reference_get_path (priv->active_row);
      bobgui_combo_box_set_active_internal (combo_box, path);
      bobgui_tree_path_free (path);
    }
  else
    {
      bobgui_combo_box_set_active_internal (combo_box, NULL);
    }
}

static void
bobgui_combo_box_menu_show (BobguiWidget *menu,
                         gpointer   user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  bobgui_combo_box_child_show (menu, user_data);

  priv->popup_in_progress = TRUE;
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (priv->button),
                                TRUE);
  priv->popup_in_progress = FALSE;
}

static void
bobgui_combo_box_menu_hide (BobguiWidget *menu,
                         gpointer   user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  bobgui_combo_box_child_hide (menu,user_data);

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (priv->button), FALSE);
}

static gboolean
cell_layout_is_sensitive (BobguiCellLayout *layout)
{
  GList *cells, *list;
  gboolean sensitive;

  cells = bobgui_cell_layout_get_cells (layout);

  sensitive = FALSE;
  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);

      if (sensitive)
        break;
    }
  g_list_free (cells);

  return sensitive;
}

static gboolean
tree_column_row_is_sensitive (BobguiComboBox *combo_box,
                              BobguiTreeIter *iter)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->row_separator_func)
    {
      if (priv->row_separator_func (priv->model, iter,
                                    priv->row_separator_data))
        return FALSE;
    }

  bobgui_cell_area_apply_attributes (priv->area, priv->model, iter, FALSE, FALSE);
  return cell_layout_is_sensitive (BOBGUI_CELL_LAYOUT (priv->area));
}

static void
bobgui_combo_box_menu_popup (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
#if 0
  int active_item;
  BobguiWidget *active;
  int width, min_width, nat_width;
#endif

  bobgui_tree_popover_open_submenu (BOBGUI_TREE_POPOVER (priv->popup_widget), "main");
  bobgui_popover_popup (BOBGUI_POPOVER (priv->popup_widget));

#if 0
  active_item = -1;
  if (bobgui_tree_row_reference_valid (priv->active_row))
    {
      BobguiTreePath *path;

      path = bobgui_tree_row_reference_get_path (priv->active_row);
      active_item = bobgui_tree_path_get_indices (path)[0];
      bobgui_tree_path_free (path);
    }

  /* FIXME handle nested menus better */
  //bobgui_tree_popover_set_active (BOBGUI_TREE_POPOVER (priv->popup_widget), active_item);

  width = bobgui_widget_get_width (BOBGUI_WIDGET (combo_box));
  bobgui_widget_set_size_request (priv->popup_widget, -1, -1);
  bobgui_widget_measure (priv->popup_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min_width, &nat_width, NULL, NULL);

  if (priv->popup_fixed_width)
    width = MAX (width, min_width);
  else
    width = MAX (width, nat_width);

  bobgui_widget_set_size_request (priv->popup_widget, width, -1);

  g_signal_handlers_disconnect_by_func (priv->popup_widget,
                                        bobgui_menu_update_scroll_offset,
                                        NULL);

  if (priv->cell_view == NULL)
    {
      g_object_set (priv->popup_widget,
                    "anchor-hints", (GDK_ANCHOR_FLIP_Y |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    "rect-anchor-dx", 0,
                    NULL);

      bobgui_menu_popup_at_widget (BOBGUI_MENU (priv->popup_widget),
                                bobgui_bin_get_child (BOBGUI_BIN (combo_box)),
                                GDK_GRAVITY_SOUTH_WEST,
                                GDK_GRAVITY_NORTH_WEST,
                                NULL);
    }
  else
    {
      int rect_anchor_dy = -2;
      GList *i;
      BobguiWidget *child;

      /* FIXME handle nested menus better */
      active = bobgui_menu_get_active (BOBGUI_MENU (priv->popup_widget));

      if (!(active && bobgui_widget_get_visible (active)))
        {
          GList *children;
          children = bobgui_menu_shell_get_items (BOBGUI_MENU_SHELL (priv->popup_widget));
          for (i = children; i && !active; i = i->next)
            {
              child = i->data;

              if (child && bobgui_widget_get_visible (child))
                active = child;
            }
          g_list_free (children);
        }

      if (active)
        {
          int child_height;
          GList *children;
          children = bobgui_menu_shell_get_items (BOBGUI_MENU_SHELL (priv->popup_widget));
          for (i = children; i && i->data != active; i = i->next)
            {
              child = i->data;

              if (child && bobgui_widget_get_visible (child))
                {
                  bobgui_widget_measure (child, BOBGUI_ORIENTATION_VERTICAL, -1,
                                      &child_height, NULL, NULL, NULL);
                  rect_anchor_dy -= child_height;
                }
            }
          g_list_free (children);

          bobgui_widget_measure (active, BOBGUI_ORIENTATION_VERTICAL, -1,
                              &child_height, NULL, NULL, NULL);
          rect_anchor_dy -= child_height / 2;
        }

      g_object_set (priv->popup_widget,
                    "anchor-hints", (GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    "rect-anchor-dy", rect_anchor_dy,
                    NULL);

      g_signal_connect (priv->popup_widget,
                        "popped-up",
                        G_CALLBACK (bobgui_menu_update_scroll_offset),
                        NULL);

      bobgui_menu_popup_at_widget (BOBGUI_MENU (priv->popup_widget),
                                BOBGUI_WIDGET (combo_box),
                                GDK_GRAVITY_WEST,
                                GDK_GRAVITY_NORTH_WEST,
                                NULL);
    }
#endif
}

/**
 * bobgui_combo_box_popup:
 * @combo_box: a `BobguiComboBox`
 *
 * Pops up the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Before calling this, @combo_box must be mapped, or nothing will happen.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_popup (BobguiComboBox *combo_box)
{
  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (combo_box)))
    g_signal_emit (combo_box, combo_box_signals[POPUP], 0);
}

/**
 * bobgui_combo_box_popup_for_device:
 * @combo_box: a `BobguiComboBox`
 * @device: a `GdkDevice`
 *
 * Pops up the menu of @combo_box.
 *
 * Note that currently this does not do anything with the device, as it was
 * previously only used for list-mode combo boxes, and those were removed
 * in BOBGUI 4. However, it is retained in case similar functionality is added
 * back later.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_popup_for_device (BobguiComboBox *combo_box,
                                GdkDevice   *device)
{
  /* As above, this currently does not do anything useful, and nothing with the
   * passed-in device. But the bits that are not blatantly obsolete are kept. */
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GDK_IS_DEVICE (device));

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (combo_box)))
    return;

  if (bobgui_widget_get_mapped (priv->popup_widget))
    return;

  bobgui_combo_box_menu_popup (combo_box);
}

static void
bobgui_combo_box_real_popup (BobguiComboBox *combo_box)
{
  bobgui_combo_box_menu_popup (combo_box);
}

static gboolean
bobgui_combo_box_real_popdown (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->popup_shown)
    {
      bobgui_combo_box_popdown (combo_box);
      return TRUE;
    }

  return FALSE;
}

/**
 * bobgui_combo_box_popdown:
 * @combo_box: a `BobguiComboBox`
 *
 * Hides the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_popdown (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  bobgui_popover_popdown (BOBGUI_POPOVER (priv->popup_widget));
}

static void
bobgui_combo_box_unset_model (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            bobgui_combo_box_model_row_inserted,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            bobgui_combo_box_model_row_deleted,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            bobgui_combo_box_model_rows_reordered,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            bobgui_combo_box_model_row_changed,
                                            combo_box);

      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->active_row)
    {
      bobgui_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }

  if (priv->cell_view)
    bobgui_cell_view_set_model (BOBGUI_CELL_VIEW (priv->cell_view), NULL);
}

static void
bobgui_combo_box_child_show (BobguiWidget *widget,
                          BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  priv->popup_shown = TRUE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static void
bobgui_combo_box_child_hide (BobguiWidget *widget,
                          BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  priv->popup_shown = FALSE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

typedef struct {
  BobguiComboBox *combo;
  BobguiTreePath *path;
  BobguiTreeIter iter;
  gboolean found;
  gboolean set;
} SearchData;

static gboolean
tree_next_func (BobguiTreeModel *model,
                BobguiTreePath  *path,
                BobguiTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (search_data->found)
    {
      if (!tree_column_row_is_sensitive (search_data->combo, iter))
        return FALSE;

      search_data->set = TRUE;
      search_data->iter = *iter;

      return TRUE;
    }

  if (bobgui_tree_path_compare (path, search_data->path) == 0)
    search_data->found = TRUE;

  return FALSE;
}

static gboolean
tree_next (BobguiComboBox  *combo,
           BobguiTreeModel *model,
           BobguiTreeIter  *iter,
           BobguiTreeIter  *next)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = bobgui_tree_model_get_path (model, iter);
  search_data.found = FALSE;
  search_data.set = FALSE;

  bobgui_tree_model_foreach (model, tree_next_func, &search_data);

  *next = search_data.iter;

  bobgui_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_prev_func (BobguiTreeModel *model,
                BobguiTreePath  *path,
                BobguiTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (bobgui_tree_path_compare (path, search_data->path) == 0)
    {
      search_data->found = TRUE;
      return TRUE;
    }

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return FALSE;
}

static gboolean
tree_prev (BobguiComboBox  *combo,
           BobguiTreeModel *model,
           BobguiTreeIter  *iter,
           BobguiTreeIter  *prev)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = bobgui_tree_model_get_path (model, iter);
  search_data.found = FALSE;
  search_data.set = FALSE;

  bobgui_tree_model_foreach (model, tree_prev_func, &search_data);

  *prev = search_data.iter;

  bobgui_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_last_func (BobguiTreeModel *model,
                BobguiTreePath  *path,
                BobguiTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return FALSE;
}

static gboolean
tree_last (BobguiComboBox  *combo,
           BobguiTreeModel *model,
           BobguiTreeIter  *last)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.set = FALSE;

  bobgui_tree_model_foreach (model, tree_last_func, &search_data);

  *last = search_data.iter;

  return search_data.set;
}


static gboolean
tree_first_func (BobguiTreeModel *model,
                 BobguiTreePath  *path,
                 BobguiTreeIter  *iter,
                 gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return TRUE;
}

static gboolean
tree_first (BobguiComboBox  *combo,
            BobguiTreeModel *model,
            BobguiTreeIter  *first)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.set = FALSE;

  bobgui_tree_model_foreach (model, tree_first_func, &search_data);

  *first = search_data.iter;

  return search_data.set;
}

static gboolean
bobgui_combo_box_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                        double                    dx,
                                        double                    dy,
                                        BobguiComboBox              *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  gboolean found = FALSE;
  BobguiTreeIter iter;
  BobguiTreeIter new_iter;

  if (!bobgui_combo_box_get_active_iter (combo_box, &iter))
    return GDK_EVENT_PROPAGATE;

  if (dy < 0)
    found = tree_prev (combo_box, priv->model, &iter, &new_iter);
  else if (dy > 0)
    found = tree_next (combo_box, priv->model, &iter, &new_iter);

  if (found)
    bobgui_combo_box_set_active_iter (combo_box, &new_iter);

  return found;

}

/* callbacks */
static void
bobgui_combo_box_menu_activate (BobguiWidget   *menu,
                             const char *path,
                             BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeIter iter;

  if (bobgui_tree_model_get_iter_from_string (priv->model, &iter, path))
    bobgui_combo_box_set_active_iter (combo_box, &iter);

  g_object_set (combo_box,
                "editing-canceled", FALSE,
                NULL);
}


static void
bobgui_combo_box_update_sensitivity (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeIter iter;
  gboolean sensitive = TRUE; /* fool code checkers */

  if (!priv->button)
    return;

  switch (priv->button_sensitivity)
    {
      case BOBGUI_SENSITIVITY_ON:
        sensitive = TRUE;
        break;
      case BOBGUI_SENSITIVITY_OFF:
        sensitive = FALSE;
        break;
      case BOBGUI_SENSITIVITY_AUTO:
        sensitive = priv->model &&
                    bobgui_tree_model_get_iter_first (priv->model, &iter);
        break;
      default:
        g_assert_not_reached ();
        break;
    }

  bobgui_widget_set_sensitive (priv->button, sensitive);
}

static void
bobgui_combo_box_model_row_inserted (BobguiTreeModel     *model,
                                  BobguiTreePath      *path,
                                  BobguiTreeIter      *iter,
                                  gpointer          user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);

  bobgui_combo_box_update_sensitivity (combo_box);
}

static void
bobgui_combo_box_model_row_deleted (BobguiTreeModel     *model,
                                 BobguiTreePath      *path,
                                 gpointer          user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (!bobgui_tree_row_reference_valid (priv->active_row))
    {
      if (priv->cell_view)
        bobgui_cell_view_set_displayed_row (BOBGUI_CELL_VIEW (priv->cell_view), NULL);
      g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
    }

  bobgui_combo_box_update_sensitivity (combo_box);
}

static void
bobgui_combo_box_model_rows_reordered (BobguiTreeModel    *model,
                                    BobguiTreePath     *path,
                                    BobguiTreeIter     *iter,
                                    int             *new_order,
                                    gpointer         user_data)
{
  bobgui_tree_row_reference_reordered (G_OBJECT (user_data), path, iter, new_order);
}

static void
bobgui_combo_box_model_row_changed (BobguiTreeModel     *model,
                                 BobguiTreePath      *path,
                                 BobguiTreeIter      *iter,
                                 gpointer          user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreePath *active_path;

  /* FIXME this belongs to BobguiCellView */
  if (bobgui_tree_row_reference_valid (priv->active_row))
    {
      active_path = bobgui_tree_row_reference_get_path (priv->active_row);
      if (bobgui_tree_path_compare (path, active_path) == 0 &&
          priv->cell_view)
        bobgui_widget_queue_resize (BOBGUI_WIDGET (priv->cell_view));
      bobgui_tree_path_free (active_path);
    }
}

static gboolean
bobgui_combo_box_menu_key (BobguiEventControllerKey *key,
                        guint                  keyval,
                        guint                  keycode,
                        GdkModifierType        modifiers,
                        BobguiComboBox           *combo_box)
{
  bobgui_event_controller_key_forward (key, BOBGUI_WIDGET (combo_box));

  return TRUE;
}

/*
 * BobguiCellLayout implementation
 */
static BobguiCellArea *
bobgui_combo_box_cell_layout_get_area (BobguiCellLayout *cell_layout)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (cell_layout);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  return priv->area;
}

/*
 * public API
 */

/**
 * bobgui_combo_box_new:
 *
 * Creates a new empty `BobguiComboBox`.
 *
 * Returns: A new `BobguiComboBox`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_new (void)
{
  return g_object_new (BOBGUI_TYPE_COMBO_BOX, NULL);
}

/**
 * bobgui_combo_box_new_with_entry:
 *
 * Creates a new empty `BobguiComboBox` with an entry.
 *
 * In order to use a combo box with entry, you need to tell it
 * which column of the model contains the text for the entry
 * by calling [method@Bobgui.ComboBox.set_entry_text_column].
 *
 * Returns: A new `BobguiComboBox`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_new_with_entry (void)
{
  return g_object_new (BOBGUI_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);
}

/**
 * bobgui_combo_box_new_with_model:
 * @model: a `BobguiTreeModel`
 *
 * Creates a new `BobguiComboBox` with a model.
 *
 * Returns: A new `BobguiComboBox`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_new_with_model (BobguiTreeModel *model)
{
  BobguiComboBox *combo_box;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (model), NULL);

  combo_box = g_object_new (BOBGUI_TYPE_COMBO_BOX, "model", model, NULL);

  return BOBGUI_WIDGET (combo_box);
}

/**
 * bobgui_combo_box_new_with_model_and_entry:
 * @model: A `BobguiTreeModel`
 *
 * Creates a new empty `BobguiComboBox` with an entry and a model.
 *
 * See also [ctor@Bobgui.ComboBox.new_with_entry].
 *
 * Returns: A new `BobguiComboBox`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_new_with_model_and_entry (BobguiTreeModel *model)
{
  return g_object_new (BOBGUI_TYPE_COMBO_BOX,
                       "has-entry", TRUE,
                       "model", model,
                       NULL);
}

/**
 * bobgui_combo_box_get_active:
 * @combo_box: A `BobguiComboBox`
 *
 * Returns the index of the currently active item.
 *
 * If the model is a non-flat treemodel, and the active item is not
 * an immediate child of the root of the tree, this function returns
 * `bobgui_tree_path_get_indices (path)[0]`, where `path` is the
 * [struct@Bobgui.TreePath] of the active item.
 *
 * Returns: An integer which is the index of the currently active item,
 *   or -1 if there’s no active item
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
int
bobgui_combo_box_get_active (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  int result;

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), 0);

  if (bobgui_tree_row_reference_valid (priv->active_row))
    {
      BobguiTreePath *path;

      path = bobgui_tree_row_reference_get_path (priv->active_row);
      result = bobgui_tree_path_get_indices (path)[0];
      bobgui_tree_path_free (path);
    }
  else
    result = -1;

  return result;
}

/**
 * bobgui_combo_box_set_active:
 * @combo_box: a `BobguiComboBox`
 * @index_: An index in the model passed during construction,
 *   or -1 to have no active item
 *
 * Sets the active item of @combo_box to be the item at @index.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_active (BobguiComboBox *combo_box,
                          int          index_)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreePath *path = NULL;

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));
  g_return_if_fail (index_ >= -1);

  if (priv->model == NULL)
    {
      /* Save index, in case the model is set after the index */
      priv->active = index_;
      if (index_ != -1)
        return;
    }

  if (index_ != -1)
    path = bobgui_tree_path_new_from_indices (index_, -1);

  bobgui_combo_box_set_active_internal (combo_box, path);

  if (path)
    bobgui_tree_path_free (path);
}

static void
bobgui_combo_box_set_active_internal (BobguiComboBox *combo_box,
                                   BobguiTreePath *path)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreePath *active_path;
  int path_cmp;

  /* Remember whether the initially active row is valid. */
  gboolean is_valid_row_reference = bobgui_tree_row_reference_valid (priv->active_row);

  if (path && is_valid_row_reference)
    {
      active_path = bobgui_tree_row_reference_get_path (priv->active_row);
      path_cmp = bobgui_tree_path_compare (path, active_path);
      bobgui_tree_path_free (active_path);
      if (path_cmp == 0)
        return;
    }

  if (priv->active_row)
    {
      bobgui_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }

  if (!path)
    {
      bobgui_tree_popover_set_active (BOBGUI_TREE_POPOVER (priv->popup_widget), -1);

      if (priv->cell_view)
        bobgui_cell_view_set_displayed_row (BOBGUI_CELL_VIEW (priv->cell_view), NULL);

      /*
       *  Do not emit a "changed" signal when an already invalid selection was
       *  now set to invalid.
       */
      if (!is_valid_row_reference)
        return;
    }
  else
    {
      priv->active_row =
        bobgui_tree_row_reference_new (priv->model, path);

      bobgui_tree_popover_set_active (BOBGUI_TREE_POPOVER (priv->popup_widget),
                                   bobgui_tree_path_get_indices (path)[0]);

      if (priv->cell_view)
        bobgui_cell_view_set_displayed_row (BOBGUI_CELL_VIEW (priv->cell_view), path);
    }

  g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
  g_object_notify (G_OBJECT (combo_box), "active");
  if (priv->id_column >= 0)
    g_object_notify (G_OBJECT (combo_box), "active-id");
}


/**
 * bobgui_combo_box_get_active_iter:
 * @combo_box: A `BobguiComboBox`
 * @iter: (out): A `BobguiTreeIter`
 *
 * Sets @iter to point to the currently active item.
 *
 * If no item is active, @iter is left unchanged.
 *
 * Returns: %TRUE if @iter was set, %FALSE otherwise
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
gboolean
bobgui_combo_box_get_active_iter (BobguiComboBox     *combo_box,
                               BobguiTreeIter     *iter)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreePath *path;
  gboolean result;

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), FALSE);

  if (!bobgui_tree_row_reference_valid (priv->active_row))
    return FALSE;

  path = bobgui_tree_row_reference_get_path (priv->active_row);
  result = bobgui_tree_model_get_iter (priv->model, iter, path);
  bobgui_tree_path_free (path);

  return result;
}

/**
 * bobgui_combo_box_set_active_iter:
 * @combo_box: A `BobguiComboBox`
 * @iter: (nullable): The `BobguiTreeIter`
 *
 * Sets the current active item to be the one referenced by @iter.
 *
 * If @iter is %NULL, the active item is unset.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_active_iter (BobguiComboBox     *combo_box,
                               BobguiTreeIter     *iter)
{
  BobguiTreePath *path = NULL;

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (iter)
    path = bobgui_tree_model_get_path (bobgui_combo_box_get_model (combo_box), iter);

  bobgui_combo_box_set_active_internal (combo_box, path);
  bobgui_tree_path_free (path);
}

/**
 * bobgui_combo_box_set_model:
 * @combo_box: A `BobguiComboBox`
 * @model: (nullable): A `BobguiTreeModel`
 *
 * Sets the model used by @combo_box to be @model.
 *
 * Will unset a previously set model (if applicable). If model is %NULL,
 * then it will unset the model.
 *
 * Note that this function does not clear the cell renderers, you have to
 * call [method@Bobgui.CellLayout.clear] yourself if you need to set up different
 * cell renderers for the new model.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_model (BobguiComboBox  *combo_box,
                         BobguiTreeModel *model)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));
  g_return_if_fail (model == NULL || BOBGUI_IS_TREE_MODEL (model));

  if (model == priv->model)
    return;

  bobgui_combo_box_unset_model (combo_box);

  if (model == NULL)
    goto out;

  priv->model = model;
  g_object_ref (priv->model);

  g_signal_connect (priv->model, "row-inserted",
                    G_CALLBACK (bobgui_combo_box_model_row_inserted),
                    combo_box);
  g_signal_connect (priv->model, "row-deleted",
                    G_CALLBACK (bobgui_combo_box_model_row_deleted),
                    combo_box);
  g_signal_connect (priv->model, "rows-reordered",
                    G_CALLBACK (bobgui_combo_box_model_rows_reordered),
                    combo_box);
  g_signal_connect (priv->model, "row-changed",
                    G_CALLBACK (bobgui_combo_box_model_row_changed),
                    combo_box);

  bobgui_tree_popover_set_model (BOBGUI_TREE_POPOVER (priv->popup_widget), priv->model);

  if (priv->cell_view)
    bobgui_cell_view_set_model (BOBGUI_CELL_VIEW (priv->cell_view),
                             priv->model);

  if (priv->active != -1)
    {
      /* If an index was set in advance, apply it now */
      bobgui_combo_box_set_active (combo_box, priv->active);
      priv->active = -1;
    }

out:
  bobgui_combo_box_update_sensitivity (combo_box);

  g_object_notify (G_OBJECT (combo_box), "model");
}

/**
 * bobgui_combo_box_get_model:
 * @combo_box: A `BobguiComboBox`
 *
 * Returns the `BobguiTreeModel` of @combo_box.
 *
 * Returns: (nullable) (transfer none): A `BobguiTreeModel` which was passed
 *   during construction.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiTreeModel *
bobgui_combo_box_get_model (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), NULL);

  return priv->model;
}

static void
bobgui_combo_box_real_move_active (BobguiComboBox   *combo_box,
                                BobguiScrollType  scroll)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeIter iter;
  BobguiTreeIter new_iter;
  gboolean    active_iter;
  gboolean    found;

  if (!priv->model)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (combo_box));
      return;
    }

  active_iter = bobgui_combo_box_get_active_iter (combo_box, &iter);

  switch (scroll)
    {
    case BOBGUI_SCROLL_STEP_BACKWARD:
    case BOBGUI_SCROLL_STEP_UP:
    case BOBGUI_SCROLL_STEP_LEFT:
      if (active_iter)
        {
          found = tree_prev (combo_box, priv->model,
                             &iter, &new_iter);
          break;
        }
      G_GNUC_FALLTHROUGH;

    case BOBGUI_SCROLL_PAGE_FORWARD:
    case BOBGUI_SCROLL_PAGE_DOWN:
    case BOBGUI_SCROLL_PAGE_RIGHT:
    case BOBGUI_SCROLL_END:
      found = tree_last (combo_box, priv->model, &new_iter);
      break;

    case BOBGUI_SCROLL_STEP_FORWARD:
    case BOBGUI_SCROLL_STEP_DOWN:
    case BOBGUI_SCROLL_STEP_RIGHT:
      if (active_iter)
        {
          found = tree_next (combo_box, priv->model,
                             &iter, &new_iter);
          break;
        }
      G_GNUC_FALLTHROUGH;

    case BOBGUI_SCROLL_PAGE_BACKWARD:
    case BOBGUI_SCROLL_PAGE_UP:
    case BOBGUI_SCROLL_PAGE_LEFT:
    case BOBGUI_SCROLL_START:
      found = tree_first (combo_box, priv->model, &new_iter);
      break;

    case BOBGUI_SCROLL_NONE:
    case BOBGUI_SCROLL_JUMP:
    default:
      return;
    }

  if (found && active_iter)
    {
      BobguiTreePath *old_path;
      BobguiTreePath *new_path;

      old_path = bobgui_tree_model_get_path (priv->model, &iter);
      new_path = bobgui_tree_model_get_path (priv->model, &new_iter);

      if (bobgui_tree_path_compare (old_path, new_path) == 0)
        found = FALSE;

      bobgui_tree_path_free (old_path);
      bobgui_tree_path_free (new_path);
    }

  if (found)
    {
      bobgui_combo_box_set_active_iter (combo_box, &new_iter);
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (combo_box));
    }
}

static gboolean
bobgui_combo_box_mnemonic_activate (BobguiWidget *widget,
                                 gboolean   group_cycling)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (widget);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->has_entry)
    {
      if (priv->child)
        bobgui_widget_grab_focus (priv->child);
    }
  else
    bobgui_widget_mnemonic_activate (priv->button, group_cycling);

  return TRUE;
}

static gboolean
bobgui_combo_box_grab_focus (BobguiWidget *widget)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (widget);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->has_entry)
    {
      if (priv->child)
        return bobgui_widget_grab_focus (priv->child);
      else
        return FALSE;
    }
  else
    return bobgui_widget_grab_focus (priv->button);
}

static void
bobgui_combo_box_unmap (BobguiWidget *widget)
{
  bobgui_combo_box_popdown (BOBGUI_COMBO_BOX (widget));

  BOBGUI_WIDGET_CLASS (bobgui_combo_box_parent_class)->unmap (widget);
}

static void
bobgui_combo_box_entry_contents_changed (BobguiEntry *entry,
                                      gpointer  user_data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (user_data);

  /*
   *  Fixes regression reported in bug #574059. The old functionality relied on
   *  bug #572478.  As a bugfix, we now emit the "changed" signal ourselves
   *  when the selection was already set to -1.
   */
  if (bobgui_combo_box_get_active(combo_box) == -1)
    g_signal_emit_by_name (combo_box, "changed");
  else
    bobgui_combo_box_set_active (combo_box, -1);
}

static void
bobgui_combo_box_entry_active_changed (BobguiComboBox *combo_box,
                                    gpointer     user_data)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeModel *model;
  BobguiTreeIter iter;

  if (bobgui_combo_box_get_active_iter (combo_box, &iter))
    {
      BobguiEntry *entry = BOBGUI_ENTRY (priv->child);

      if (entry)
        {
          BobguiTreePath *path;
          char        *path_str;
          char        *text = NULL;

          model    = bobgui_combo_box_get_model (combo_box);
          path     = bobgui_tree_model_get_path (model, &iter);
          path_str = bobgui_tree_path_to_string (path);

          g_signal_handlers_block_by_func (entry,
                                           bobgui_combo_box_entry_contents_changed,
                                           combo_box);


          g_signal_emit (combo_box, combo_box_signals[FORMAT_ENTRY_TEXT], 0,
                         path_str, &text);

          bobgui_editable_set_text (BOBGUI_EDITABLE (entry), text);

          g_signal_handlers_unblock_by_func (entry,
                                             bobgui_combo_box_entry_contents_changed,
                                             combo_box);

          bobgui_tree_path_free (path);
          g_free (text);
          g_free (path_str);
        }
    }
}

static char *
bobgui_combo_box_format_entry_text (BobguiComboBox     *combo_box,
                                 const char      *path)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeModel       *model;
  BobguiTreeIter         iter;
  char               *text = NULL;

  if (priv->text_column >= 0)
    {
      model = bobgui_combo_box_get_model (combo_box);
      bobgui_tree_model_get_iter_from_string (model, &iter, path);

      bobgui_tree_model_get (model, &iter,
                          priv->text_column, &text,
                          -1);
    }

  return text;
}

static void
bobgui_combo_box_constructed (GObject *object)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (object);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  G_OBJECT_CLASS (bobgui_combo_box_parent_class)->constructed (object);

  bobgui_combo_box_create_child (combo_box);

  if (priv->has_entry)
    {
      priv->text_renderer = bobgui_cell_renderer_text_new ();
      bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combo_box),
                                  priv->text_renderer, TRUE);

      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo_box), -1);
    }
}

static void
bobgui_combo_box_dispose (GObject* object)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (object);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->popup_idle_id > 0)
    {
      g_source_remove (priv->popup_idle_id);
      priv->popup_idle_id = 0;
    }

  if (priv->box)
    {
      /* destroy things (unparent will kill the latest ref from us)
       * last unref on button will destroy the arrow
       */
      bobgui_widget_unparent (priv->box);
      priv->box = NULL;
      priv->button = NULL;
      priv->arrow = NULL;
      priv->child = NULL;
      priv->cell_view = NULL;
    }

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func = NULL;
  priv->row_separator_data = NULL;
  priv->row_separator_destroy = NULL;

  if (priv->popup_widget)
    {
      /* Stop menu destruction triggering toggle on a now-invalid button */
      g_signal_handlers_disconnect_by_func (priv->popup_widget,
                                            bobgui_combo_box_menu_hide,
                                            combo_box);
      g_clear_pointer (&priv->popup_widget, bobgui_widget_unparent);
    }

  bobgui_combo_box_unset_model (combo_box);

  G_OBJECT_CLASS (bobgui_combo_box_parent_class)->dispose (object);
}

static gboolean
bobgui_cell_editable_key_pressed (BobguiEventControllerKey *key,
                               guint                  keyval,
                               guint                  keycode,
                               GdkModifierType        modifiers,
                               BobguiComboBox           *combo_box)
{
  if (keyval == GDK_KEY_Escape)
    {
      g_object_set (combo_box,
                    "editing-canceled", TRUE,
                    NULL);
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (combo_box));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (combo_box));

      return TRUE;
    }
  else if (keyval == GDK_KEY_Return ||
           keyval == GDK_KEY_ISO_Enter ||
           keyval == GDK_KEY_KP_Enter)
    {
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (combo_box));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (combo_box));

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_combo_box_start_editing (BobguiCellEditable *cell_editable,
                             GdkEvent        *event)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (cell_editable);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiEventController *controller;

  priv->is_cell_renderer = TRUE;

  controller = bobgui_event_controller_key_new ();
  g_signal_connect_object (controller, "key-pressed",
                           G_CALLBACK (bobgui_cell_editable_key_pressed),
                           cell_editable, 0);

  if (priv->cell_view)
    {
      bobgui_widget_add_controller (priv->button, controller);
      bobgui_widget_grab_focus (priv->button);
    }
  else
    {
      bobgui_widget_add_controller (priv->child, controller);

      bobgui_widget_grab_focus (priv->child);
      bobgui_widget_set_can_focus (priv->button, FALSE);
    }
}

/**
 * bobgui_combo_box_set_popup_fixed_width:
 * @combo_box: a `BobguiComboBox`
 * @fixed: whether to use a fixed popup width
 *
 * Specifies whether the popup’s width should be a fixed width.
 *
 * If @fixed is %TRUE, the popup's width is set to match the
 * allocated width of the combo box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_popup_fixed_width (BobguiComboBox *combo_box,
                                     gboolean     fixed)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (priv->popup_fixed_width != fixed)
    {
      priv->popup_fixed_width = fixed;

      g_object_notify (G_OBJECT (combo_box), "popup-fixed-width");
    }
}

/**
 * bobgui_combo_box_get_popup_fixed_width:
 * @combo_box: a `BobguiComboBox`
 *
 * Gets whether the popup uses a fixed width.
 *
 * Returns: %TRUE if the popup uses a fixed width
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
gboolean
bobgui_combo_box_get_popup_fixed_width (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), FALSE);

  return priv->popup_fixed_width;
}

/**
 * bobgui_combo_box_get_row_separator_func: (skip)
 * @combo_box: a `BobguiComboBox`
 *
 * Returns the current row separator function.
 *
 * Returns: (nullable): the current row separator function.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiTreeViewRowSeparatorFunc
bobgui_combo_box_get_row_separator_func (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), NULL);

  return priv->row_separator_func;
}

/**
 * bobgui_combo_box_set_row_separator_func:
 * @combo_box: a `BobguiComboBox`
 * @func: (nullable): a `BobguiTreeViewRowSeparatorFunc`
 * @data: (nullable): user data to pass to @func
 * @destroy: (nullable): destroy notifier for @data
 *
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator.
 *
 * If the row separator function is %NULL, no separators are drawn.
 * This is the default value.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_row_separator_func (BobguiComboBox                 *combo_box,
                                      BobguiTreeViewRowSeparatorFunc  func,
                                      gpointer                     data,
                                      GDestroyNotify               destroy)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func = func;
  priv->row_separator_data = data;
  priv->row_separator_destroy = destroy;

  bobgui_tree_popover_set_row_separator_func (BOBGUI_TREE_POPOVER (priv->popup_widget),
                                           (BobguiTreeViewRowSeparatorFunc)bobgui_combo_box_row_separator_func,
                                           combo_box, NULL);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (combo_box));
}

/**
 * bobgui_combo_box_set_button_sensitivity:
 * @combo_box: a `BobguiComboBox`
 * @sensitivity: specify the sensitivity of the dropdown button
 *
 * Sets whether the dropdown button of the combo box should update
 * its sensitivity depending on the model contents.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_button_sensitivity (BobguiComboBox        *combo_box,
                                      BobguiSensitivityType  sensitivity)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (priv->button_sensitivity != sensitivity)
    {
      priv->button_sensitivity = sensitivity;
      bobgui_combo_box_update_sensitivity (combo_box);

      g_object_notify (G_OBJECT (combo_box), "button-sensitivity");
    }
}

/**
 * bobgui_combo_box_get_button_sensitivity:
 * @combo_box: a `BobguiComboBox`
 *
 * Returns whether the combo box sets the dropdown button
 * sensitive or not when there are no items in the model.
 *
 * Returns: %BOBGUI_SENSITIVITY_ON if the dropdown button
 *   is sensitive when the model is empty, %BOBGUI_SENSITIVITY_OFF
 *   if the button is always insensitive or %BOBGUI_SENSITIVITY_AUTO
 *   if it is only sensitive as long as the model has one item to
 *   be selected.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiSensitivityType
bobgui_combo_box_get_button_sensitivity (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), FALSE);

  return priv->button_sensitivity;
}


/**
 * bobgui_combo_box_get_has_entry:
 * @combo_box: a `BobguiComboBox`
 *
 * Returns whether the combo box has an entry.
 *
 * Returns: whether there is an entry in @combo_box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
gboolean
bobgui_combo_box_get_has_entry (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), FALSE);

  return priv->has_entry;
}

/**
 * bobgui_combo_box_set_entry_text_column:
 * @combo_box: A `BobguiComboBox`
 * @text_column: A column in @model to get the strings from for
 *   the internal entry
 *
 * Sets the model column which @combo_box should use to get strings
 * from to be @text_column.
 *
 * For this column no separate
 * [class@Bobgui.CellRenderer] is needed.
 *
 * The column @text_column in the model of @combo_box must be of
 * type %G_TYPE_STRING.
 *
 * This is only relevant if @combo_box has been created with
 * [property@Bobgui.ComboBox:has-entry] as %TRUE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_entry_text_column (BobguiComboBox *combo_box,
                                     int          text_column)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));
  g_return_if_fail (text_column >= 0);
  g_return_if_fail (priv->model == NULL || text_column < bobgui_tree_model_get_n_columns (priv->model));

  if (priv->text_column != text_column)
    {
      priv->text_column = text_column;

      if (priv->text_renderer != NULL)
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combo_box),
                                        priv->text_renderer,
                                        "text", text_column,
                                        NULL);

      g_object_notify (G_OBJECT (combo_box), "entry-text-column");
    }
}

/**
 * bobgui_combo_box_get_entry_text_column:
 * @combo_box: A `BobguiComboBox`
 *
 * Returns the column which @combo_box is using to get the strings
 * from to display in the internal entry.
 *
 * Returns: A column in the data source model of @combo_box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
int
bobgui_combo_box_get_entry_text_column (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), 0);

  return priv->text_column;
}

static void
bobgui_combo_box_buildable_add_child (BobguiBuildable *buildable,
                                   BobguiBuilder   *builder,
                                   GObject      *child,
                                   const char   *type)
{
  if (BOBGUI_IS_CELL_RENDERER (child))
    _bobgui_cell_layout_buildable_add_child (buildable, builder, child, type);
  else if (BOBGUI_IS_WIDGET (child))
    bobgui_combo_box_set_child (BOBGUI_COMBO_BOX (buildable), BOBGUI_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static gboolean
bobgui_combo_box_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                          BobguiBuilder         *builder,
                                          GObject            *child,
                                          const char         *tagname,
                                          BobguiBuildableParser *parser,
                                          gpointer           *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, data))
    return TRUE;

  return _bobgui_cell_layout_buildable_custom_tag_start (buildable, builder, child,
                                                      tagname, parser, data);
}

static void
bobgui_combo_box_buildable_custom_tag_end (BobguiBuildable *buildable,
                                        BobguiBuilder   *builder,
                                        GObject      *child,
                                        const char   *tagname,
                                        gpointer      data)
{
  if (!_bobgui_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data))
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname, data);
}

static GObject *
bobgui_combo_box_buildable_get_internal_child (BobguiBuildable *buildable,
                                            BobguiBuilder   *builder,
                                            const char   *childname)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (buildable);
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  if (priv->has_entry && strcmp (childname, "entry") == 0)
    return G_OBJECT (priv->child);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/**
 * bobgui_combo_box_set_id_column:
 * @combo_box: A `BobguiComboBox`
 * @id_column: A column in @model to get string IDs for values from
 *
 * Sets the model column which @combo_box should use to get string IDs
 * for values from.
 *
 * The column @id_column in the model of @combo_box must be of type
 * %G_TYPE_STRING.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_id_column (BobguiComboBox *combo_box,
                             int          id_column)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));

  if (id_column != priv->id_column)
    {
      g_return_if_fail (id_column >= 0);
      g_return_if_fail (priv->model == NULL || id_column < bobgui_tree_model_get_n_columns (priv->model));

      priv->id_column = id_column;

      g_object_notify (G_OBJECT (combo_box), "id-column");
      g_object_notify (G_OBJECT (combo_box), "active-id");
    }
}

/**
 * bobgui_combo_box_get_id_column:
 * @combo_box: A `BobguiComboBox`
 *
 * Returns the column which @combo_box is using to get string IDs
 * for values from.
 *
 * Returns: A column in the data source model of @combo_box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
int
bobgui_combo_box_get_id_column (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), 0);

  return priv->id_column;
}

/**
 * bobgui_combo_box_get_active_id:
 * @combo_box: a `BobguiComboBox`
 *
 * Returns the ID of the active row of @combo_box.
 *
 * This value is taken from the active row and the column specified
 * by the [property@Bobgui.ComboBox:id-column] property of @combo_box
 * (see [method@Bobgui.ComboBox.set_id_column]).
 *
 * The returned value is an interned string which means that you can
 * compare the pointer by value to other interned strings and that you
 * must not free it.
 *
 * If the [property@Bobgui.ComboBox:id-column] property of @combo_box is
 * not set, or if no row is active, or if the active row has a %NULL
 * ID value, then %NULL is returned.
 *
 * Returns: (nullable): the ID of the active row
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
const char *
bobgui_combo_box_get_active_id (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  int column;

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), NULL);

  column = priv->id_column;

  if (column < 0)
    return NULL;

  model = bobgui_combo_box_get_model (combo_box);
  g_return_val_if_fail (bobgui_tree_model_get_column_type (model, column) ==
                        G_TYPE_STRING, NULL);

  if (bobgui_combo_box_get_active_iter (combo_box, &iter))
    {
      const char *interned;
      char *id;

      bobgui_tree_model_get (model, &iter, column, &id, -1);
      interned = g_intern_string (id);
      g_free (id);

      return interned;
    }

  return NULL;
}

/**
 * bobgui_combo_box_set_active_id:
 * @combo_box: a `BobguiComboBox`
 * @active_id: (nullable): the ID of the row to select
 *
 * Changes the active row of @combo_box to the one that has an ID equal to
 * @active_id.
 *
 * If @active_id is %NULL, the active row is unset. Rows having
 * a %NULL ID string cannot be made active by this function.
 *
 * If the [property@Bobgui.ComboBox:id-column] property of @combo_box is
 * unset or if no row has the given ID then the function does nothing
 * and returns %FALSE.
 *
 * Returns: %TRUE if a row with a matching ID was found. If a %NULL
 *   @active_id was given to unset the active row, the function
 *   always returns %TRUE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
gboolean
bobgui_combo_box_set_active_id (BobguiComboBox *combo_box,
                             const char  *active_id)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  gboolean match = FALSE;
  int column;

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), FALSE);

  if (active_id == NULL)
    {
      bobgui_combo_box_set_active (combo_box, -1);
      return TRUE;  /* active row was successfully unset */
    }

  column = priv->id_column;

  if (column < 0)
    return FALSE;

  model = bobgui_combo_box_get_model (combo_box);
  g_return_val_if_fail (bobgui_tree_model_get_column_type (model, column) ==
                        G_TYPE_STRING, FALSE);

  if (bobgui_tree_model_get_iter_first (model, &iter))
    do {
      char *id;

      bobgui_tree_model_get (model, &iter, column, &id, -1);
      if (id != NULL)
        match = strcmp (id, active_id) == 0;
      g_free (id);

      if (match)
        {
          bobgui_combo_box_set_active_iter (combo_box, &iter);
          break;
        }
    } while (bobgui_tree_model_iter_next (model, &iter));

  g_object_notify (G_OBJECT (combo_box), "active-id");

  return match;
}

BobguiWidget *
bobgui_combo_box_get_popup (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  return priv->popup_widget;
}

/**
 * bobgui_combo_box_set_child:
 * @combo_box: a `BobguiComboBox`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @combo_box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_set_child (BobguiComboBox *combo_box,
                         BobguiWidget   *child)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_if_fail (BOBGUI_IS_COMBO_BOX (combo_box));
  g_return_if_fail (child == NULL || bobgui_widget_get_parent (child) == NULL);

  if (priv->child)
    bobgui_combo_box_remove (combo_box, priv->child);

  if (child)
    bobgui_combo_box_add (combo_box, child);

  g_object_notify (G_OBJECT (combo_box), "child");
}

/**
 * bobgui_combo_box_get_child:
 * @combo_box: a `BobguiComboBox`
 *
 * Gets the child widget of @combo_box.
 *
 * Returns: (nullable) (transfer none): the child widget of @combo_box
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_get_child (BobguiComboBox *combo_box)
{
  BobguiComboBoxPrivate *priv = bobgui_combo_box_get_instance_private (combo_box);

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX (combo_box), NULL);

  return priv->child;
}

