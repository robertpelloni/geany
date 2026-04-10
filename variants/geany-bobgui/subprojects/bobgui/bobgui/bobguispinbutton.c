/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BobguiSpinButton widget for BOBGUI
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguispinbutton.h"

#include "bobguispinbuttonprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiaccessiblerange.h"
#include "bobguiadjustmentprivate.h"
#include "bobguibutton.h"
#include "bobguibuttonprivate.h"
#include "bobguieditable.h"
#include "deprecated/bobguicelleditable.h"
#include "bobguitext.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguigestureclick.h"
#include "bobguigestureswipe.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiboxlayout.h"
#include "bobguitextprivate.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>

#define MAX_TIMER_CALLS       5
#define EPSILON               1e-10
#define MAX_DIGITS            20
#define TIMEOUT_INITIAL       500
#define TIMEOUT_REPEAT        50

/**
 * BobguiSpinButton:
 *
 * Allows to enter or change numeric values.
 *
 * <picture>
 *   <source srcset="spinbutton-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSpinButton" src="spinbutton.png">
 * </picture>
 *
 * Rather than having to directly type a number into a `BobguiEntry`,
 * `BobguiSpinButton` allows the user to click on one of two arrows
 * to increment or decrement the displayed value. A value can still be
 * typed in, with the bonus that it can be checked to ensure it is in a
 * given range.
 *
 * The main properties of a `BobguiSpinButton` are through an adjustment.
 * See the [class@Bobgui.Adjustment] documentation for more details about
 * an adjustment's properties.
 *
 * Note that `BobguiSpinButton` will by default make its entry large enough
 * to accommodate the lower and upper bounds of the adjustment. If this
 * is not desired, the automatic sizing can be turned off by explicitly
 * setting [property@Bobgui.Editable:width-chars] to a value != -1.
 *
 * ## Using a BobguiSpinButton to get an integer
 *
 * ```c
 * // Provides a function to retrieve an integer value from a BobguiSpinButton
 * // and creates a spin button to model percentage values.
 *
 * int
 * grab_int_value (BobguiSpinButton *button,
 *                 gpointer       user_data)
 * {
 *   return bobgui_spin_button_get_value_as_int (button);
 * }
 *
 * void
 * create_integer_spin_button (void)
 * {
 *
 *   BobguiWidget *window, *button;
 *   BobguiAdjustment *adjustment;
 *
 *   adjustment = bobgui_adjustment_new (50.0, 0.0, 100.0, 1.0, 5.0, 0.0);
 *
 *   window = bobgui_window_new ();
 *
 *   // creates the spinbutton, with no decimal places
 *   button = bobgui_spin_button_new (adjustment, 1.0, 0);
 *   bobgui_window_set_child (BOBGUI_WINDOW (window), button);
 *
 *   bobgui_window_present (BOBGUI_WINDOW (window));
 * }
 * ```
 *
 * ## Using a BobguiSpinButton to get a floating point value
 *
 * ```c
 * // Provides a function to retrieve a floating point value from a
 * // BobguiSpinButton, and creates a high precision spin button.
 *
 * float
 * grab_float_value (BobguiSpinButton *button,
 *                   gpointer       user_data)
 * {
 *   return bobgui_spin_button_get_value (button);
 * }
 *
 * void
 * create_floating_spin_button (void)
 * {
 *   BobguiWidget *window, *button;
 *   BobguiAdjustment *adjustment;
 *
 *   adjustment = bobgui_adjustment_new (2.500, 0.0, 5.0, 0.001, 0.1, 0.0);
 *
 *   window = bobgui_window_new ();
 *
 *   // creates the spinbutton, with three decimal places
 *   button = bobgui_spin_button_new (adjustment, 0.001, 3);
 *   bobgui_window_set_child (BOBGUI_WINDOW (window), button);
 *
 *   bobgui_window_present (BOBGUI_WINDOW (window));
 * }
 * ```
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.SpinButton::change-value]
 *
 * # CSS nodes
 *
 * ```
 * spinbutton.horizontal
 * ├── text
 * │    ├── undershoot.left
 * │    ╰── undershoot.right
 * ├── button.down
 * ╰── button.up
 * ```
 *
 * ```
 * spinbutton.vertical
 * ├── button.up
 * ├── text
 * │    ├── undershoot.left
 * │    ╰── undershoot.right
 * ╰── button.down
 * ```
 *
 * `BobguiSpinButton`s main CSS node has the name spinbutton. It creates subnodes
 * for the entry and the two buttons, with these names. The button nodes have
 * the style classes .up and .down. The `BobguiText` subnodes (if present) are put
 * below the text node. The orientation of the spin button is reflected in
 * the .vertical or .horizontal style class on the main node.
 *
 * # Accessibility
 *
 * `BobguiSpinButton` uses the [enum@Bobgui.AccessibleRole.spin_button] role.
 */

typedef struct _BobguiSpinButton      BobguiSpinButton;
typedef struct _BobguiSpinButtonClass BobguiSpinButtonClass;

struct _BobguiSpinButton
{
  BobguiWidget parent_instance;

  BobguiAdjustment *adjustment;

  BobguiWidget     *entry;

  BobguiWidget     *up_button;
  BobguiWidget     *down_button;

  BobguiWidget     *click_child;

  guint32        timer;

  BobguiSpinButtonUpdatePolicy update_policy;

  double         climb_rate;
  double         timer_step;
  double         swipe_remainder;

  int            width_chars;

  BobguiOrientation orientation;

  guint          digits        : 10;
  guint          need_timer    : 1;
  guint          numeric       : 1;
  guint          snap_to_ticks : 1;
  guint          timer_calls   : 3;
  guint          wrap          : 1;
  guint          editing_canceled : 1;
  guint          edited : 1;
  guint          activates_default : 1;
};

struct _BobguiSpinButtonClass
{
  BobguiWidgetClass parent_class;

  int (*input)  (BobguiSpinButton *spin_button,
                 double        *new_value);
  int (*output) (BobguiSpinButton *spin_button);
  void (*value_changed) (BobguiSpinButton *spin_button);
  void (*activate)      (BobguiSpinButton *spin_button);

  /* Action signals for keybindings, do not connect to these */
  void (*change_value) (BobguiSpinButton *spin_button,
                        BobguiScrollType  scroll);

  void (*wrapped) (BobguiSpinButton *spin_button);
};

enum {
  PROP_0,
  PROP_ACTIVATES_DEFAULT,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS,
  PROP_SNAP_TO_TICKS,
  PROP_NUMERIC,
  PROP_WRAP,
  PROP_UPDATE_POLICY,
  PROP_VALUE,
  NUM_SPINBUTTON_PROPS,
  PROP_ORIENTATION = NUM_SPINBUTTON_PROPS,
  PROP_EDITING_CANCELED
};

/* Signals */
enum
{
  INPUT_,
  OUTPUT,
  VALUE_CHANGED,
  ACTIVATE,
  CHANGE_VALUE,
  WRAPPED,
  LAST_SIGNAL
};

static void bobgui_spin_button_editable_init  (BobguiEditableInterface *iface);
static void bobgui_spin_button_cell_editable_init  (BobguiCellEditableIface *iface);
static void bobgui_spin_button_finalize       (GObject            *object);
static void bobgui_spin_button_dispose        (GObject            *object);
static void bobgui_spin_button_set_property   (GObject         *object,
                                            guint            prop_id,
                                            const GValue    *value,
                                            GParamSpec      *pspec);
static void bobgui_spin_button_get_property   (GObject         *object,
                                            guint            prop_id,
                                            GValue          *value,
                                            GParamSpec      *pspec);
static void bobgui_spin_button_realize        (BobguiWidget          *widget);
static void bobgui_spin_button_state_flags_changed  (BobguiWidget     *widget,
                                                  BobguiStateFlags  previous_state);
static gboolean bobgui_spin_button_timer          (BobguiSpinButton      *spin_button);
static gboolean bobgui_spin_button_stop_spinning  (BobguiSpinButton      *spin);
static void bobgui_spin_button_value_changed  (BobguiAdjustment      *adjustment,
                                            BobguiSpinButton      *spin_button);

static void bobgui_spin_button_changed (BobguiEditable   *editable,
                                     BobguiSpinButton *spin_button);
static void bobgui_spin_button_real_activate (BobguiSpinButton *spin_button);
static void bobgui_spin_button_activate       (BobguiText            *entry,
                                            gpointer            user_data);
static void bobgui_spin_button_unset_adjustment (BobguiSpinButton *spin_button);
static void bobgui_spin_button_set_orientation (BobguiSpinButton     *spin_button,
                                             BobguiOrientation     orientation);
static void bobgui_spin_button_snap           (BobguiSpinButton      *spin_button,
                                            double              val);
static void bobgui_spin_button_insert_text    (BobguiEditable        *editable,
                                            const char         *new_text,
                                            int                 new_text_length,
                                            int                *position,
                                            gpointer            data);
static void bobgui_spin_button_real_spin      (BobguiSpinButton      *spin_button,
                                            double              step);
static void bobgui_spin_button_real_change_value (BobguiSpinButton   *spin,
                                               BobguiScrollType    scroll);

static int bobgui_spin_button_default_input   (BobguiSpinButton      *spin_button,
                                            double             *new_val);
static void bobgui_spin_button_default_output (BobguiSpinButton      *spin_button);

static void bobgui_spin_button_update_width_chars (BobguiSpinButton *spin_button);

static void bobgui_spin_button_accessible_init (BobguiAccessibleInterface *iface);

static void bobgui_spin_button_accessible_range_init (BobguiAccessibleRangeInterface *iface);

static guint spinbutton_signals[LAST_SIGNAL] = {0};
static GParamSpec *spinbutton_props[NUM_SPINBUTTON_PROPS] = {NULL, };

G_DEFINE_TYPE_WITH_CODE (BobguiSpinButton, bobgui_spin_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_spin_button_accessible_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE,
                                                bobgui_spin_button_accessible_range_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE,
                                                bobgui_spin_button_editable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_EDITABLE,
                                                bobgui_spin_button_cell_editable_init))

#define add_spin_binding(widget_class, keyval, mask, scroll)            \
  bobgui_widget_class_add_binding_signal (widget_class, keyval, mask,      \
                                       "change-value",                  \
                                       "(i)", scroll)


static gboolean
bobgui_spin_button_grab_focus (BobguiWidget *widget)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (widget);

  return bobgui_widget_grab_focus (spin_button->entry);
}

static gboolean
bobgui_spin_button_mnemonic_activate (BobguiWidget *widget,
                                   gboolean   group_cycling)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (widget);

  return bobgui_widget_grab_focus (spin_button->entry);
}

static void
bobgui_spin_button_class_init (BobguiSpinButtonClass *class)
{
  GObjectClass     *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass   *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->finalize = bobgui_spin_button_finalize;
  gobject_class->dispose = bobgui_spin_button_dispose;
  gobject_class->set_property = bobgui_spin_button_set_property;
  gobject_class->get_property = bobgui_spin_button_get_property;

  widget_class->realize = bobgui_spin_button_realize;
  widget_class->state_flags_changed = bobgui_spin_button_state_flags_changed;
  widget_class->mnemonic_activate = bobgui_spin_button_mnemonic_activate;
  widget_class->grab_focus = bobgui_spin_button_grab_focus;
  widget_class->focus = bobgui_widget_focus_child;

  class->input = NULL;
  class->output = NULL;
  class->change_value = bobgui_spin_button_real_change_value;
  class->activate = bobgui_spin_button_real_activate;

  /**
   * BobguiSpinButton:activates-default:
   *
   * Whether to activate the default widget when the spin button is activated.
   *
   * See [signal@Bobgui.SpinButton::activate] for what counts as activation.
   *
   * Since: 4.14
   */
  spinbutton_props[PROP_ACTIVATES_DEFAULT] =
    g_param_spec_boolean ("activates-default", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:adjustment:
   *
   * The adjustment that holds the value of the spin button.
   */
  spinbutton_props[PROP_ADJUSTMENT] =
    g_param_spec_object ("adjustment", NULL, NULL,
                         BOBGUI_TYPE_ADJUSTMENT,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:climb-rate:
   *
   * The acceleration rate when you hold down a button or key.
   */
  spinbutton_props[PROP_CLIMB_RATE] =
    g_param_spec_double ("climb-rate", NULL, NULL,
                         0.0, G_MAXDOUBLE, 0.0,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:digits:
   *
   * The number of decimal places to display.
   */
  spinbutton_props[PROP_DIGITS] =
    g_param_spec_uint ("digits", NULL, NULL,
                       0, MAX_DIGITS, 0,
                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:snap-to-ticks:
   *
   * Whether erroneous values are automatically changed to the spin buttons
   * nearest step increment.
   */
  spinbutton_props[PROP_SNAP_TO_TICKS] =
    g_param_spec_boolean ("snap-to-ticks", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:numeric:
   *
   * Whether non-numeric characters should be ignored.
   */
  spinbutton_props[PROP_NUMERIC] =
    g_param_spec_boolean ("numeric", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:wrap:
   *
   * Whether a spin button should wrap upon reaching its limits.
   */
  spinbutton_props[PROP_WRAP] =
    g_param_spec_boolean ("wrap", NULL, NULL,
                          FALSE,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:update-policy:
   *
   * Whether the spin button should update always, or only when the value
   * is acceptable.
   */
  spinbutton_props[PROP_UPDATE_POLICY] =
    g_param_spec_enum ("update-policy", NULL, NULL,
                       BOBGUI_TYPE_SPIN_BUTTON_UPDATE_POLICY,
                       BOBGUI_UPDATE_ALWAYS,
                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSpinButton:value:
   *
   * The current value.
   */
  spinbutton_props[PROP_VALUE] =
    g_param_spec_double ("value", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_SPINBUTTON_PROPS, spinbutton_props);
  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");
  g_object_class_override_property (gobject_class, PROP_EDITING_CANCELED, "editing-canceled");
  bobgui_editable_install_properties (gobject_class, PROP_EDITING_CANCELED + 1);

  /**
   * BobguiSpinButton::input:
   * @spin_button: the object on which the signal was emitted
   * @new_value: (out) (type double): return location for the new value
   *
   * Emitted to convert the users input into a double value.
   *
   * The signal handler is expected to use [method@Bobgui.Editable.get_text]
   * to retrieve the text of the spinbutton and set @new_value to the
   * new value.
   *
   * The default conversion uses g_strtod().
   *
   * Returns: %TRUE for a successful conversion, %FALSE if the input
   *   was not handled, and %BOBGUI_INPUT_ERROR if the conversion failed.
   */
  spinbutton_signals[INPUT_] =
    g_signal_new (I_("input"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, input),
                  NULL, NULL,
                  _bobgui_marshal_INT__POINTER,
                  G_TYPE_INT, 1,
                  G_TYPE_POINTER);
  g_signal_set_va_marshaller (spinbutton_signals[INPUT_],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_INT__POINTERv);

  /**
   * BobguiSpinButton::output:
   * @spin_button: the object on which the signal was emitted
   *
   * Emitted to tweak the formatting of the value for display.
   *
   * ```c
   * // show leading zeros
   * static gboolean
   * on_output (BobguiSpinButton *spin,
   *            gpointer       data)
   * {
   *    char *text;
   *    int value;
   *
   *    value = bobgui_spin_button_get_value_as_int (spin);
   *    text = g_strdup_printf ("%02d", value);
   *    bobgui_editable_set_text (BOBGUI_EDITABLE (spin), text):
   *    g_free (text);
   *
   *    return TRUE;
   * }
   * ```
   *
   * Returns: %TRUE if the value has been displayed
   */
  spinbutton_signals[OUTPUT] =
    g_signal_new (I_("output"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, output),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (spinbutton_signals[OUTPUT],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /**
   * BobguiSpinButton::value-changed:
   * @spin_button: the object on which the signal was emitted
   *
   * Emitted when the value is changed.
   *
   * Also see the [signal@Bobgui.SpinButton::output] signal.
   */
  spinbutton_signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, value_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSpinButton::activate:
   * @self: The widget on which the signal is emitted
   *
   * Emitted when the spin button is activated.
   *
   * The keybindings for this signal are all forms of the <kbd>Enter</kbd> key.
   *
   * If the <kbd>Enter</kbd> key results in the value being committed to the
   * spin button, then activation does not occur until <kbd>Enter</kbd> is
   * pressed again.
   *
   * Since: 4.14
   */
  spinbutton_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, spinbutton_signals[ACTIVATE]);

  /**
   * BobguiSpinButton::wrapped:
   * @spin_button: the object on which the signal was emitted
   *
   * Emitted right after the spinbutton wraps from its maximum
   * to its minimum value or vice-versa.
   */
  spinbutton_signals[WRAPPED] =
    g_signal_new (I_("wrapped"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, wrapped),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /* Action signals */
  /**
   * BobguiSpinButton::change-value:
   * @spin_button: the object on which the signal was emitted
   * @scroll: a `BobguiScrollType` to specify the speed and amount of change
   *
   * Emitted when the user initiates a value change.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal are Up/Down and PageUp/PageDown.
   */
  spinbutton_signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSpinButtonClass, change_value),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_SCROLL_TYPE);

  add_spin_binding (widget_class, GDK_KEY_Up, 0, BOBGUI_SCROLL_STEP_UP);
  add_spin_binding (widget_class, GDK_KEY_KP_Up, 0, BOBGUI_SCROLL_STEP_UP);
  add_spin_binding (widget_class, GDK_KEY_Down, 0, BOBGUI_SCROLL_STEP_DOWN);
  add_spin_binding (widget_class, GDK_KEY_KP_Down, 0, BOBGUI_SCROLL_STEP_DOWN);
  add_spin_binding (widget_class, GDK_KEY_Page_Up, 0, BOBGUI_SCROLL_PAGE_UP);
  add_spin_binding (widget_class, GDK_KEY_Page_Down, 0, BOBGUI_SCROLL_PAGE_DOWN);
  add_spin_binding (widget_class, GDK_KEY_End, GDK_CONTROL_MASK, BOBGUI_SCROLL_END);
  add_spin_binding (widget_class, GDK_KEY_Home, GDK_CONTROL_MASK, BOBGUI_SCROLL_START);
  add_spin_binding (widget_class, GDK_KEY_Page_Up, GDK_CONTROL_MASK, BOBGUI_SCROLL_END);
  add_spin_binding (widget_class, GDK_KEY_Page_Down, GDK_CONTROL_MASK, BOBGUI_SCROLL_START);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("spinbutton"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON);
}

static BobguiEditable *
bobgui_spin_button_get_delegate (BobguiEditable *editable)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (editable);

  return BOBGUI_EDITABLE (spin_button->entry);
}

static void
bobgui_spin_button_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = bobgui_spin_button_get_delegate;
}

static gboolean
bobgui_spin_button_accessible_get_platform_state (BobguiAccessible              *self,
                                               BobguiAccessiblePlatformState  state)
{
  return bobgui_editable_delegate_get_accessible_platform_state (BOBGUI_EDITABLE (self), state);
}

static void
bobgui_spin_button_accessible_init (BobguiAccessibleInterface *iface)
{
  BobguiAccessibleInterface *parent_iface = g_type_interface_peek_parent (iface);

  iface->get_at_context = parent_iface->get_at_context;
  iface->get_platform_state = bobgui_spin_button_accessible_get_platform_state;
}

static gboolean
accessible_range_set_current_value (BobguiAccessibleRange *accessible_range,
                                    double              value)
{
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (accessible_range), value);
  return TRUE;
}

static void
bobgui_spin_button_accessible_range_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
bobgui_cell_editable_spin_button_activated (BobguiText *text, BobguiSpinButton *spin)
{
  g_object_ref (spin);
  bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (spin));
  bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (spin));
  g_object_unref (spin);
}

static gboolean
bobgui_cell_editable_spin_button_key_pressed (BobguiEventControllerKey *key,
                                           guint                  keyval,
                                           guint                  keycode,
                                           GdkModifierType        modifiers,
                                           BobguiSpinButton         *spin)
{
  if (keyval == GDK_KEY_Escape)
    {
      spin->editing_canceled = TRUE;

      g_object_ref (spin);
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (spin));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (spin));
      g_object_unref (spin);

      return GDK_EVENT_STOP;
    }

  /* override focus */
  if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
    {
      g_object_ref (spin);
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (spin));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (spin));
      g_object_unref (spin);

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
bobgui_spin_button_start_editing (BobguiCellEditable *cell_editable,
                               GdkEvent        *event)
{
  BobguiSpinButton *spin = BOBGUI_SPIN_BUTTON (cell_editable);

  g_signal_connect (spin->entry, "activate",
                    G_CALLBACK (bobgui_cell_editable_spin_button_activated), cell_editable);
  g_signal_connect (bobgui_text_get_key_controller (BOBGUI_TEXT (spin->entry)), "key-pressed",
                    G_CALLBACK (bobgui_cell_editable_spin_button_key_pressed), cell_editable);
}

static void
bobgui_spin_button_cell_editable_init (BobguiCellEditableIface *iface)
{
  iface->start_editing = bobgui_spin_button_start_editing;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
bobgui_spin_button_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (object);

  if (prop_id == PROP_EDITING_CANCELED + 1 + BOBGUI_EDITABLE_PROP_WIDTH_CHARS)
    {
      spin_button->width_chars = g_value_get_int (value);
      bobgui_spin_button_update_width_chars (spin_button);
      return;
    }

  if (bobgui_editable_delegate_set_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
      BobguiAdjustment *adjustment;

    case PROP_ACTIVATES_DEFAULT:
      bobgui_spin_button_set_activates_default (spin_button, g_value_get_boolean (value));
      break;
    case PROP_ADJUSTMENT:
      adjustment = BOBGUI_ADJUSTMENT (g_value_get_object (value));
      bobgui_spin_button_set_adjustment (spin_button, adjustment);
      break;
    case PROP_CLIMB_RATE:
      bobgui_spin_button_configure (spin_button,
                                 spin_button->adjustment,
                                 g_value_get_double (value),
                                 spin_button->digits);
      break;
    case PROP_DIGITS:
      bobgui_spin_button_configure (spin_button,
                                 spin_button->adjustment,
                                 spin_button->climb_rate,
                                 g_value_get_uint (value));
      break;
    case PROP_SNAP_TO_TICKS:
      bobgui_spin_button_set_snap_to_ticks (spin_button, g_value_get_boolean (value));
      break;
    case PROP_NUMERIC:
      bobgui_spin_button_set_numeric (spin_button, g_value_get_boolean (value));
      break;
    case PROP_WRAP:
      bobgui_spin_button_set_wrap (spin_button, g_value_get_boolean (value));
      break;
    case PROP_UPDATE_POLICY:
      bobgui_spin_button_set_update_policy (spin_button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      bobgui_spin_button_set_value (spin_button, g_value_get_double (value));
      break;
    case PROP_ORIENTATION:
      bobgui_spin_button_set_orientation (spin_button, g_value_get_enum (value));
      break;
    case PROP_EDITING_CANCELED:
      if (spin_button->editing_canceled != g_value_get_boolean (value))
        {
          spin_button->editing_canceled = g_value_get_boolean (value);
          g_object_notify (object, "editing-canceled");
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_spin_button_get_property (GObject      *object,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (object);

  if (prop_id == PROP_EDITING_CANCELED + 1 + BOBGUI_EDITABLE_PROP_WIDTH_CHARS)
    {
      g_value_set_int (value, spin_button->width_chars);
      return;
    }
  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, spin_button->activates_default);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, spin_button->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, spin_button->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, spin_button->digits);
      break;
    case PROP_SNAP_TO_TICKS:
      g_value_set_boolean (value, spin_button->snap_to_ticks);
      break;
    case PROP_NUMERIC:
      g_value_set_boolean (value, spin_button->numeric);
      break;
    case PROP_WRAP:
      g_value_set_boolean (value, spin_button->wrap);
      break;
    case PROP_UPDATE_POLICY:
      g_value_set_enum (value, spin_button->update_policy);
      break;
     case PROP_VALUE:
       g_value_set_double (value, bobgui_adjustment_get_value (spin_button->adjustment));
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (spin_button)))));
      break;
    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value, spin_button->editing_canceled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
swipe_gesture_begin (BobguiGesture       *gesture,
                     GdkEventSequence *sequence,
                     BobguiSpinButton    *spin_button)
{
  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  bobgui_widget_grab_focus (BOBGUI_WIDGET (spin_button));
  spin_button->swipe_remainder = 0;
}

static void
swipe_gesture_update (BobguiGesture       *gesture,
                      GdkEventSequence *sequence,
                      BobguiSpinButton    *spin_button)
{
  double vel_y, step;

  bobgui_gesture_swipe_get_velocity (BOBGUI_GESTURE_SWIPE (gesture), NULL, &vel_y);
  step = (-vel_y / 20) + spin_button->swipe_remainder;
  spin_button->swipe_remainder = fmod (step, bobgui_adjustment_get_step_increment (spin_button->adjustment));
  bobgui_spin_button_real_spin (spin_button, step - spin_button->swipe_remainder);
}

static gboolean
scroll_controller_scroll (BobguiEventControllerScroll *Scroll,
                          double                    dx,
                          double                    dy,
                          BobguiWidget                *widget)
{
  BobguiSpinButton *spin = BOBGUI_SPIN_BUTTON (widget);

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);
  bobgui_spin_button_real_spin (spin, -dy * bobgui_adjustment_get_step_increment (spin->adjustment));

  return GDK_EVENT_STOP;
}

static gboolean
bobgui_spin_button_stop_spinning (BobguiSpinButton *spin)
{
  gboolean did_spin = FALSE;

  if (spin->timer)
    {
      g_source_remove (spin->timer);
      spin->timer = 0;
      spin->need_timer = FALSE;

      did_spin = TRUE;
    }

  spin->timer_step = bobgui_adjustment_get_step_increment (spin->adjustment);
  spin->timer_calls = 0;

  spin->click_child = NULL;

  return did_spin;
}

static void
start_spinning (BobguiSpinButton *spin,
                BobguiWidget     *click_child,
                double         step)
{
  spin->click_child = click_child;

  if (!spin->timer)
    {
      spin->timer_step = step;
      spin->need_timer = TRUE;
      spin->timer = g_timeout_add (TIMEOUT_INITIAL,
                                   (GSourceFunc) bobgui_spin_button_timer,
                                   (gpointer) spin);
      gdk_source_set_static_name_by_id (spin->timer, "[bobgui] bobgui_spin_button_timer");
    }
  bobgui_spin_button_real_spin (spin, click_child == spin->up_button ? step : -step);
}

static void
button_pressed_cb (BobguiGestureClick *gesture,
                   int              n_pressses,
                   double           x,
                   double           y,
                   gpointer         user_data)
{
  BobguiSpinButton *spin_button = user_data;
  BobguiWidget *pressed_button = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_widget_grab_focus (BOBGUI_WIDGET (spin_button));

  if (bobgui_editable_get_editable (BOBGUI_EDITABLE (spin_button->entry)))
    {
      int button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
      bobgui_spin_button_update (spin_button);

      if (button == GDK_BUTTON_PRIMARY)
        start_spinning (spin_button, pressed_button, bobgui_adjustment_get_step_increment (spin_button->adjustment));
      else if (button == GDK_BUTTON_MIDDLE)
        start_spinning (spin_button, pressed_button, bobgui_adjustment_get_page_increment (spin_button->adjustment));

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (spin_button));
    }
}

static void
button_released_cb (BobguiGestureClick *gesture,
                    int              n_pressses,
                    double           x,
                    double           y,
                    gpointer         user_data)
{
  BobguiSpinButton *spin_button = user_data;
  int button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  bobgui_spin_button_stop_spinning (spin_button);

  if (button == GDK_BUTTON_SECONDARY)
    {
      BobguiWidget *button_widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
      double diff;
      if (button_widget == spin_button->down_button)
        {
          diff = bobgui_adjustment_get_value (spin_button->adjustment) - bobgui_adjustment_get_lower (spin_button->adjustment);
          if (diff > EPSILON)
            bobgui_spin_button_real_spin (spin_button, -diff);
        }
      else if (button_widget == spin_button->up_button)
        {
          diff = bobgui_adjustment_get_upper (spin_button->adjustment) - bobgui_adjustment_get_value (spin_button->adjustment);
          if (diff > EPSILON)
            bobgui_spin_button_real_spin (spin_button, diff);
        }
    }
}

static void
button_cancel_cb (BobguiGesture       *gesture,
                  GdkEventSequence *sequence,
                  BobguiSpinButton    *spin_button)
{
  bobgui_spin_button_stop_spinning (spin_button);
}

static void
key_controller_key_released (BobguiEventControllerKey *key,
                             guint                  keyval,
                             guint                  keycode,
                             GdkModifierType        modifiers,
                             BobguiSpinButton         *spin_button)
{
  spin_button->timer_step = bobgui_adjustment_get_step_increment (spin_button->adjustment);
  spin_button->timer_calls = 0;
}

static void
key_controller_focus_out (BobguiEventController   *controller,
                          BobguiSpinButton        *spin_button)
{
  if (bobgui_editable_get_editable (BOBGUI_EDITABLE (spin_button->entry)))
    bobgui_spin_button_update (spin_button);
}

static void
bobgui_spin_button_init (BobguiSpinButton *spin_button)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;

  spin_button->activates_default = FALSE;
  spin_button->adjustment = NULL;
  spin_button->timer = 0;
  spin_button->climb_rate = 0.0;
  spin_button->timer_step = 0.0;
  spin_button->update_policy = BOBGUI_UPDATE_ALWAYS;
  spin_button->click_child = NULL;
  spin_button->need_timer = FALSE;
  spin_button->timer_calls = 0;
  spin_button->digits = 0;
  spin_button->numeric = FALSE;
  spin_button->wrap = FALSE;
  spin_button->edited = FALSE;
  spin_button->snap_to_ticks = FALSE;
  spin_button->width_chars = -1;

  bobgui_widget_update_orientation (BOBGUI_WIDGET (spin_button), BOBGUI_ORIENTATION_HORIZONTAL);

  spin_button->entry = bobgui_text_new ();
  bobgui_editable_init_delegate (BOBGUI_EDITABLE (spin_button));
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (spin_button->entry), 0);
  bobgui_editable_set_max_width_chars (BOBGUI_EDITABLE (spin_button->entry), 0);
  bobgui_widget_set_hexpand (spin_button->entry, TRUE);
  bobgui_widget_set_vexpand (spin_button->entry, TRUE);
  g_signal_connect (spin_button->entry, "activate", G_CALLBACK (bobgui_spin_button_activate), spin_button);
  g_signal_connect (spin_button->entry, "changed", G_CALLBACK (bobgui_spin_button_changed), spin_button);
  g_signal_connect (spin_button->entry, "insert-text", G_CALLBACK (bobgui_spin_button_insert_text), spin_button);
  bobgui_widget_set_parent (spin_button->entry, BOBGUI_WIDGET (spin_button));

  spin_button->down_button = g_object_new (BOBGUI_TYPE_BUTTON,
                                           "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                           "icon-name", "value-decrease-symbolic",
                                           NULL);
  bobgui_widget_add_css_class (spin_button->down_button, "down");
  bobgui_widget_set_can_focus (spin_button->down_button, FALSE);
  bobgui_widget_set_parent (spin_button->down_button, BOBGUI_WIDGET (spin_button));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (gesture, "pressed", G_CALLBACK (button_pressed_cb), spin_button);
  g_signal_connect (gesture, "released", G_CALLBACK (button_released_cb), spin_button);
  g_signal_connect (gesture, "cancel", G_CALLBACK (button_cancel_cb), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button->down_button), BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_gesture_group (bobgui_button_get_gesture (BOBGUI_BUTTON (spin_button->down_button)), gesture);

  spin_button->up_button = g_object_new (BOBGUI_TYPE_BUTTON,
                                         "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                         "icon-name", "value-increase-symbolic",
                                         NULL);
  bobgui_widget_add_css_class (spin_button->up_button, "up");
  bobgui_widget_set_can_focus (spin_button->up_button, FALSE);
  bobgui_widget_set_parent (spin_button->up_button, BOBGUI_WIDGET (spin_button));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (gesture, "pressed", G_CALLBACK (button_pressed_cb), spin_button);
  g_signal_connect (gesture, "released", G_CALLBACK (button_released_cb), spin_button);
  g_signal_connect (gesture, "cancel", G_CALLBACK (button_cancel_cb), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button->up_button), BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_gesture_group (bobgui_button_get_gesture (BOBGUI_BUTTON (spin_button->up_button)), gesture);

  bobgui_spin_button_set_adjustment (spin_button, NULL);

  gesture = bobgui_gesture_swipe_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (gesture, "begin",
                    G_CALLBACK (swipe_gesture_begin), spin_button);
  g_signal_connect (gesture, "update",
                    G_CALLBACK (swipe_gesture_update), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button->entry),
                             BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL |
                                                BOBGUI_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (scroll_controller_scroll), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button), controller);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-released",
                    G_CALLBACK (key_controller_key_released), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button), controller);
  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "leave",
                    G_CALLBACK (key_controller_focus_out), spin_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (spin_button), controller);
}

static void
bobgui_spin_button_finalize (GObject *object)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (object);

  bobgui_spin_button_unset_adjustment (spin_button);

  bobgui_editable_finish_delegate (BOBGUI_EDITABLE (spin_button));

  bobgui_widget_unparent (spin_button->entry);
  bobgui_widget_unparent (spin_button->up_button);
  bobgui_widget_unparent (spin_button->down_button);

  G_OBJECT_CLASS (bobgui_spin_button_parent_class)->finalize (object);
}

static void
bobgui_spin_button_dispose (GObject *object)
{
  bobgui_spin_button_stop_spinning (BOBGUI_SPIN_BUTTON (object));

  G_OBJECT_CLASS (bobgui_spin_button_parent_class)->dispose (object);
}

static void
bobgui_spin_button_realize (BobguiWidget *widget)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (widget);
  gboolean return_val;
  const char *text;

  BOBGUI_WIDGET_CLASS (bobgui_spin_button_parent_class)->realize (widget);

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);

  /* If output wasn't processed explicitly by the method connected to the
   * 'output' signal; and if we don't have any explicit 'text' set initially,
   * fallback to the default output.
   */
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button->entry));
  if (!return_val && (spin_button->numeric || text == NULL || *text == '\0'))
    bobgui_spin_button_default_output (spin_button);
}

/* This is called when :value, :wrap, or the bounds of the adjustment change,
 * as the combination of those determines if our up|down_button are sensitive
 */
static void
update_buttons_sensitivity (BobguiSpinButton *spin_button)
{
  const double lower = bobgui_adjustment_get_lower (spin_button->adjustment);
  const double upper = bobgui_adjustment_get_upper (spin_button->adjustment);
  const double value = bobgui_adjustment_get_value (spin_button->adjustment);

  bobgui_widget_set_sensitive (spin_button->up_button,
                            spin_button->wrap || upper - value > EPSILON);
  bobgui_widget_set_sensitive (spin_button->down_button,
                            spin_button->wrap || value - lower > EPSILON);
}

/* Callback used when the spin button's adjustment changes.
 * We need to reevaluate our size request & up|down_button sensitivity.
 */
static void
adjustment_changed_cb (BobguiAdjustment *adjustment, gpointer data)
{
  BobguiSpinButton *spin_button = BOBGUI_SPIN_BUTTON (data);

  spin_button->timer_step = bobgui_adjustment_get_step_increment (adjustment);

  update_buttons_sensitivity (spin_button);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (spin_button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (adjustment) -
                                                                     bobgui_adjustment_get_page_size (adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                  -1);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (spin_button));
}

static void
bobgui_spin_button_unset_adjustment (BobguiSpinButton *spin_button)
{
  if (spin_button->adjustment)
    {
      g_signal_handlers_disconnect_by_func (spin_button->adjustment,
                                            bobgui_spin_button_value_changed,
                                            spin_button);
      g_signal_handlers_disconnect_by_func (spin_button->adjustment,
                                            adjustment_changed_cb,
                                            spin_button);
      g_clear_object (&spin_button->adjustment);
    }
}

static void
bobgui_spin_button_set_orientation (BobguiSpinButton  *spin,
                                 BobguiOrientation  orientation)
{
  BobguiBoxLayout *layout_manager;
  BobguiEditable *editable = BOBGUI_EDITABLE (spin->entry);

  if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (spin)) == orientation)
    return;

  layout_manager = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (spin)));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout_manager), orientation);

  bobgui_widget_update_orientation (BOBGUI_WIDGET (spin), orientation);

  /* change alignment if it's the default */
  if (orientation == BOBGUI_ORIENTATION_VERTICAL &&
      bobgui_editable_get_alignment (editable) == 0.0)
    bobgui_editable_set_alignment (editable, 0.5);
  else if (orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
           bobgui_editable_get_alignment (editable) == 0.5)
    bobgui_editable_set_alignment (editable, 0.0);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      /* Current orientation of the box is vertical! */
      bobgui_widget_insert_after (spin->up_button, BOBGUI_WIDGET (spin), spin->down_button);
      bobgui_box_layout_set_baseline_child (layout_manager, 0);
    }
  else
    {
      /* Current orientation of the box is horizontal! */
      bobgui_widget_insert_before (spin->up_button, BOBGUI_WIDGET (spin), spin->entry);
      bobgui_box_layout_set_baseline_child (layout_manager, 1);
    }

  g_object_notify (G_OBJECT (spin), "orientation");
}

static char *
weed_out_neg_zero (char *str,
                   int    digits)
{
  if (str[0] == '-')
    {
      char neg_zero[8];
      g_snprintf (neg_zero, 8, "%0.*f", digits, -0.0);
      if (strcmp (neg_zero, str) == 0)
        memmove (str, str + 1, strlen (str));
    }
  return str;
}

static char *
bobgui_spin_button_format_for_value (BobguiSpinButton *spin_button,
                                  double         value)
{
  char *buf = g_strdup_printf ("%0.*f", spin_button->digits, value);

  return weed_out_neg_zero (buf, spin_button->digits);
}

static void
bobgui_spin_button_update_width_chars (BobguiSpinButton *spin_button)
{
  char *str;
  double value;
  int width_chars, c;

  if (spin_button->width_chars == -1)
    {
      width_chars = 0;

      value = bobgui_adjustment_get_lower (spin_button->adjustment);
      str = bobgui_spin_button_format_for_value (spin_button, value);
      c = g_utf8_strlen (str, -1);
      g_free (str);

      width_chars = MAX (width_chars, c);

      value = bobgui_adjustment_get_upper (spin_button->adjustment);
      str = bobgui_spin_button_format_for_value (spin_button, value);
      c = g_utf8_strlen (str, -1);
      g_free (str);

      width_chars = MAX (width_chars, c);

      width_chars = MIN (width_chars, 10);
    }
  else
    width_chars = spin_button->width_chars;

  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (spin_button->entry), width_chars);
}

static void
bobgui_spin_button_state_flags_changed (BobguiWidget     *widget,
                                     BobguiStateFlags  previous_state)
{
  BobguiSpinButton *spin = BOBGUI_SPIN_BUTTON (widget);

  if (!bobgui_widget_is_sensitive (widget))
    bobgui_spin_button_stop_spinning (spin);

  BOBGUI_WIDGET_CLASS (bobgui_spin_button_parent_class)->state_flags_changed (widget, previous_state);
}

static int
bobgui_spin_button_timer (BobguiSpinButton *spin_button)
{
  gboolean retval = FALSE;

  if (spin_button->timer)
    {
      if (spin_button->click_child == spin_button->up_button)
        bobgui_spin_button_real_spin (spin_button, spin_button->timer_step);
      else
        bobgui_spin_button_real_spin (spin_button, -spin_button->timer_step);

      if (spin_button->need_timer)
        {
          spin_button->need_timer = FALSE;
          spin_button->timer = g_timeout_add (TIMEOUT_REPEAT,
                                       (GSourceFunc) bobgui_spin_button_timer,
                                       spin_button);
          gdk_source_set_static_name_by_id (spin_button->timer, "[bobgui] bobgui_spin_button_timer");
        }
      else
        {
          if (spin_button->climb_rate > 0.0 && spin_button->timer_step
              < bobgui_adjustment_get_page_increment (spin_button->adjustment))
            {
              if (spin_button->timer_calls < MAX_TIMER_CALLS)
                spin_button->timer_calls++;
              else
                {
                  spin_button->timer_calls = 0;
                  spin_button->timer_step += spin_button->climb_rate;
                }
            }
          retval = TRUE;
        }
    }

  return retval;
}

static void
bobgui_spin_button_value_changed (BobguiAdjustment *adjustment,
                               BobguiSpinButton *spin_button)
{
  gboolean return_val;

  g_return_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment));

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
  if (return_val == FALSE)
    bobgui_spin_button_default_output (spin_button);

  g_signal_emit (spin_button, spinbutton_signals[VALUE_CHANGED], 0);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (spin_button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                  -1);

  update_buttons_sensitivity (spin_button);

  g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_VALUE]);

  spin_button->edited = FALSE;
}

static void
bobgui_spin_button_real_change_value (BobguiSpinButton *spin,
                                   BobguiScrollType  scroll)
{
  double old_value;

  if (!bobgui_editable_get_editable (BOBGUI_EDITABLE (spin->entry)))
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (spin));
      return;
    }

  /* When the key binding is activated, there may be an outstanding
   * value, so we first have to commit what is currently written in
   * the spin buttons text entry. See #106574
   */
  bobgui_spin_button_update (spin);

  old_value = bobgui_adjustment_get_value (spin->adjustment);

  switch (scroll)
    {
    case BOBGUI_SCROLL_STEP_BACKWARD:
    case BOBGUI_SCROLL_STEP_DOWN:
    case BOBGUI_SCROLL_STEP_LEFT:
      bobgui_spin_button_real_spin (spin, -spin->timer_step);

      if (spin->climb_rate > 0.0 &&
          spin->timer_step < bobgui_adjustment_get_page_increment (spin->adjustment))
        {
          if (spin->timer_calls < MAX_TIMER_CALLS)
            spin->timer_calls++;
          else
            {
              spin->timer_calls = 0;
              spin->timer_step += spin->climb_rate;
            }
        }
      break;

    case BOBGUI_SCROLL_STEP_FORWARD:
    case BOBGUI_SCROLL_STEP_UP:
    case BOBGUI_SCROLL_STEP_RIGHT:
      bobgui_spin_button_real_spin (spin, spin->timer_step);

      if (spin->climb_rate > 0.0 &&
          spin->timer_step < bobgui_adjustment_get_page_increment (spin->adjustment))
        {
          if (spin->timer_calls < MAX_TIMER_CALLS)
            spin->timer_calls++;
          else
            {
              spin->timer_calls = 0;
              spin->timer_step += spin->climb_rate;
            }
        }
      break;

    case BOBGUI_SCROLL_PAGE_BACKWARD:
    case BOBGUI_SCROLL_PAGE_DOWN:
    case BOBGUI_SCROLL_PAGE_LEFT:
      bobgui_spin_button_real_spin (spin, -bobgui_adjustment_get_page_increment (spin->adjustment));
      break;

    case BOBGUI_SCROLL_PAGE_FORWARD:
    case BOBGUI_SCROLL_PAGE_UP:
    case BOBGUI_SCROLL_PAGE_RIGHT:
      bobgui_spin_button_real_spin (spin, bobgui_adjustment_get_page_increment (spin->adjustment));
      break;

    case BOBGUI_SCROLL_START:
      {
        double diff = bobgui_adjustment_get_value (spin->adjustment) - bobgui_adjustment_get_lower (spin->adjustment);
        if (diff > EPSILON)
          bobgui_spin_button_real_spin (spin, -diff);
        break;
      }

    case BOBGUI_SCROLL_END:
      {
        double diff = bobgui_adjustment_get_upper (spin->adjustment) - bobgui_adjustment_get_value (spin->adjustment);
        if (diff > EPSILON)
          bobgui_spin_button_real_spin (spin, diff);
        break;
      }

    case BOBGUI_SCROLL_NONE:
    case BOBGUI_SCROLL_JUMP:
    default:
      g_warning ("Invalid scroll type %d for BobguiSpinButton::change-value", scroll);
      break;
    }

  bobgui_spin_button_update (spin);

  if (bobgui_adjustment_get_value (spin->adjustment) == old_value)
    bobgui_widget_error_bell (BOBGUI_WIDGET (spin));
}

static void
bobgui_spin_button_snap (BobguiSpinButton *spin_button,
                      double         val)
{
  double inc;

  inc = bobgui_adjustment_get_step_increment (spin_button->adjustment);
  if (inc != 0)
    {
      double tmp;

      tmp = (val - bobgui_adjustment_get_lower (spin_button->adjustment)) / inc;
      if (tmp - floor (tmp) < ceil (tmp) - tmp)
        val = bobgui_adjustment_get_lower (spin_button->adjustment) + floor (tmp) * inc;
      else
        val = bobgui_adjustment_get_lower (spin_button->adjustment) + ceil (tmp) * inc;
    }

  bobgui_spin_button_set_value (spin_button, val);
}

static void
bobgui_spin_button_changed (BobguiEditable   *editable,
                         BobguiSpinButton *spin_button)
{
  spin_button->edited = TRUE;
}

static void
bobgui_spin_button_real_activate (BobguiSpinButton *spin_button)
{
  if (spin_button->activates_default)
    bobgui_widget_activate_default (BOBGUI_WIDGET (spin_button));
}

static void
bobgui_spin_button_activate (BobguiText *entry,
                          gpointer  user_data)
{
  BobguiSpinButton *spin_button = user_data;

  if (!bobgui_editable_get_editable (BOBGUI_EDITABLE (spin_button->entry)))
    return;

  const gboolean was_edited = spin_button->edited;

  bobgui_spin_button_update (spin_button); /* resets edited */

  if (!was_edited)
    g_signal_emit (spin_button, spinbutton_signals[ACTIVATE], 0);
}

static void
bobgui_spin_button_insert_text (BobguiEditable *editable,
                             const char  *new_text,
                             int          new_text_length,
                             int         *position,
                             gpointer     data)
{
  BobguiSpinButton *spin = BOBGUI_SPIN_BUTTON (data);

  g_signal_stop_emission_by_name (editable, "insert-text");

  if (spin->numeric)
    {
      struct lconv *lc;
      gboolean sign;
      int dotpos = -1;
      int i;
      int entry_length;
      const char *entry_text;

      entry_text = bobgui_editable_get_text (BOBGUI_EDITABLE (spin->entry));
      entry_length = g_utf8_strlen (entry_text, -1);

      lc = localeconv ();

      for (sign = FALSE, i = 0; i<entry_length; i++)
        if (entry_text[i] == '-' || entry_text[i] == '+')
          {
            sign = TRUE;
            break;
          }

      if (sign && !(*position))
        return;

      for (dotpos = -1, i = 0; i<entry_length; i++)
        if (entry_text[i] == *(lc->decimal_point))
          {
            dotpos = i;
            break;
          }

      if (dotpos > -1 && *position > dotpos &&
          (int)spin->digits - entry_length
            + dotpos - new_text_length + 1 < 0)
        return;

      for (i = 0; i < new_text_length; i++)
        {
          if (new_text[i] == '-' || new_text[i] == '+')
            {
              if (sign || (*position) || i)
                return;
              sign = TRUE;
            }
          else if (new_text[i] == *(lc->decimal_point))
            {
              if (!spin->digits || dotpos > -1 ||
                  (new_text_length - 1 - i + entry_length - *position > (int)spin->digits))
                return;
              dotpos = *position + i;
            }
          else if (new_text[i] < 0x30 || new_text[i] > 0x39)
            return;
        }
    }

  g_signal_handlers_block_by_func (editable, bobgui_spin_button_insert_text, data);

  bobgui_editable_insert_text (BOBGUI_EDITABLE (spin->entry),
                            new_text, new_text_length, position);

  g_signal_handlers_unblock_by_func (editable, bobgui_spin_button_insert_text, data);
}

static void
bobgui_spin_button_real_spin (BobguiSpinButton *spin_button,
                           double         increment)
{
  BobguiAdjustment *adjustment;
  double new_value = 0.0;
  gboolean wrapped = FALSE;

  adjustment = spin_button->adjustment;

  new_value = bobgui_adjustment_get_value (adjustment) + increment;

  if (increment > 0)
    {
      if (spin_button->wrap)
        {
          if (fabs (bobgui_adjustment_get_value (adjustment) - bobgui_adjustment_get_upper (adjustment)) < EPSILON)
            {
              new_value = bobgui_adjustment_get_lower (adjustment);
              wrapped = TRUE;
            }
          else if (new_value > bobgui_adjustment_get_upper (adjustment))
            new_value = bobgui_adjustment_get_upper (adjustment);
        }
      else
        new_value = MIN (new_value, bobgui_adjustment_get_upper (adjustment));
    }
  else if (increment < 0)
    {
      if (spin_button->wrap)
        {
          if (fabs (bobgui_adjustment_get_value (adjustment) - bobgui_adjustment_get_lower (adjustment)) < EPSILON)
            {
              new_value = bobgui_adjustment_get_upper (adjustment);
              wrapped = TRUE;
            }
          else if (new_value < bobgui_adjustment_get_lower (adjustment))
            new_value = bobgui_adjustment_get_lower (adjustment);
        }
      else
        new_value = MAX (new_value, bobgui_adjustment_get_lower (adjustment));
    }

  if (fabs (new_value - bobgui_adjustment_get_value (adjustment)) > EPSILON)
    bobgui_adjustment_set_value (adjustment, new_value);

  if (wrapped)
    g_signal_emit (spin_button, spinbutton_signals[WRAPPED], 0);
}

static int
bobgui_spin_button_default_input (BobguiSpinButton *spin_button,
                               double        *new_val)
{
  char *err = NULL;
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button->entry));

  *new_val = g_strtod (text, &err);
  if (*err)
    {
      /* try to convert with local digits */
      gint64 val = 0;
      int sign = 1;
      const char *p;

      for (p = text; *p; p = g_utf8_next_char (p))
        {
          gunichar ch = g_utf8_get_char (p);

          if (p == text && ch == '-')
            {
              sign = -1;
              continue;
            }

          if (!g_unichar_isdigit (ch))
            break;

          val = val * 10 + g_unichar_digit_value (ch);
        }

      if (*p)
        return BOBGUI_INPUT_ERROR;

      *new_val = sign * val;
    }

  return FALSE;
}

static void
bobgui_spin_button_default_output (BobguiSpinButton *spin_button)
{
  char *buf = bobgui_spin_button_format_for_value (spin_button,
                                                 bobgui_adjustment_get_value (spin_button->adjustment));

  if (strcmp (buf, bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button->entry))))
    bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button->entry), buf);

  g_free (buf);
}


/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/

/**
 * bobgui_spin_button_configure:
 * @spin_button: a `BobguiSpinButton`
 * @adjustment: (nullable): a `BobguiAdjustment` to replace the spin button’s
 *   existing adjustment, or %NULL to leave its current adjustment unchanged
 * @climb_rate: the new climb rate
 * @digits: the number of decimal places to display in the spin button
 *
 * Changes the properties of an existing spin button.
 *
 * The adjustment, climb rate, and number of decimal places
 * are updated accordingly.
 */
void
bobgui_spin_button_configure (BobguiSpinButton *spin_button,
                           BobguiAdjustment *adjustment,
                           double         climb_rate,
                           guint          digits)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (!adjustment)
    adjustment = spin_button->adjustment;

  g_object_freeze_notify (G_OBJECT (spin_button));

  if (spin_button->adjustment != adjustment)
    {
      bobgui_spin_button_unset_adjustment (spin_button);

      spin_button->adjustment = adjustment;
      g_object_ref_sink (adjustment);
      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (bobgui_spin_button_value_changed),
                        spin_button);
      g_signal_connect (adjustment, "changed",
                        G_CALLBACK (adjustment_changed_cb),
                        spin_button);
      spin_button->timer_step = bobgui_adjustment_get_step_increment (spin_button->adjustment);

      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_ADJUSTMENT]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (spin_button));
    }

  if (spin_button->digits != digits)
    {
      spin_button->digits = digits;
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_DIGITS]);
    }

  if (spin_button->climb_rate != climb_rate)
    {
      spin_button->climb_rate = climb_rate;
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_CLIMB_RATE]);
    }

  bobgui_spin_button_update_width_chars (spin_button);

  g_object_thaw_notify (G_OBJECT (spin_button));

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (spin_button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (adjustment) -
                                                                     bobgui_adjustment_get_page_size (adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                  -1);

  bobgui_spin_button_value_changed (adjustment, spin_button);
}

/**
 * bobgui_spin_button_new:
 * @adjustment: (nullable): the `BobguiAdjustment` that this spin button should use
 * @climb_rate: specifies by how much the rate of change in the value will
 *   accelerate if you continue to hold down an up/down button or arrow key
 * @digits: the number of decimal places to display
 *
 * Creates a new `BobguiSpinButton`.
 *
 * Returns: The new `BobguiSpinButton`
 */
BobguiWidget *
bobgui_spin_button_new (BobguiAdjustment *adjustment,
                     double         climb_rate,
                     guint          digits)
{
  BobguiSpinButton *spin;

  if (adjustment)
    g_return_val_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment), NULL);

  spin = g_object_new (BOBGUI_TYPE_SPIN_BUTTON, NULL);

  bobgui_spin_button_configure (spin, adjustment, climb_rate, digits);

  return BOBGUI_WIDGET (spin);
}

/**
 * bobgui_spin_button_new_with_range:
 * @min: Minimum allowable value
 * @max: Maximum allowable value
 * @step: Increment added or subtracted by spinning the widget
 *
 * Creates a new `BobguiSpinButton` with the given properties.
 *
 * This is a convenience constructor that allows creation
 * of a numeric `BobguiSpinButton` without manually creating
 * an adjustment. The value is initially set to the minimum
 * value and a page increment of 10 * @step is the default.
 * The precision of the spin button is equivalent to the
 * precision of @step.
 *
 * Note that the way in which the precision is derived works
 * best if @step is a power of ten. If the resulting precision
 * is not suitable for your needs, use
 * [method@Bobgui.SpinButton.set_digits] to correct it.
 *
 * Returns: The new `BobguiSpinButton`
 */
BobguiWidget *
bobgui_spin_button_new_with_range (double min,
                                double max,
                                double step)
{
  BobguiAdjustment *adjustment;
  BobguiSpinButton *spin;
  int digits;

  g_return_val_if_fail (min <= max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  spin = g_object_new (BOBGUI_TYPE_SPIN_BUTTON, NULL);

  adjustment = bobgui_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    digits = 0;
  else {
    digits = abs ((int) floor (log10 (fabs (step))));
    if (digits > MAX_DIGITS)
      digits = MAX_DIGITS;
  }

  bobgui_spin_button_configure (spin, adjustment, step, digits);

  bobgui_spin_button_set_numeric (spin, TRUE);

  return BOBGUI_WIDGET (spin);
}

/**
 * bobgui_spin_button_set_activates_default:
 * @spin_button: a `BobguiSpinButton`
 * @activates_default: %TRUE to activate window’s default widget on activation
 *
 * Sets whether activating the spin button will activate the default
 * widget for the window containing the spin button.
 *
 * See [signal@Bobgui.SpinButton::activate] for what counts as activation.
 *
 * Since: 4.14
 */
void
bobgui_spin_button_set_activates_default (BobguiSpinButton *spin_button,
                                       gboolean       activates_default)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  activates_default = !!activates_default;

  if (activates_default != spin_button->activates_default)
    {
      spin_button->activates_default = activates_default;
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_ACTIVATES_DEFAULT]);
    }
}

/**
 * bobgui_spin_button_get_activates_default:
 * @spin_button: a `BobguiSpinButton`
 *
 * Retrieves the value set by [method@Bobgui.SpinButton.set_activates_default].
 *
 * Returns: %TRUE if the spin button will activate the default widget
 *
 * Since: 4.14
 */
gboolean
bobgui_spin_button_get_activates_default (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->activates_default;
}

/**
 * bobgui_spin_button_set_adjustment:
 * @spin_button: a `BobguiSpinButton`
 * @adjustment: a `BobguiAdjustment` to replace the existing adjustment
 *
 * Replaces the `BobguiAdjustment` associated with @spin_button.
 */
void
bobgui_spin_button_set_adjustment (BobguiSpinButton *spin_button,
                                BobguiAdjustment *adjustment)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else if (spin_button->adjustment == adjustment)
    return;

  bobgui_spin_button_configure (spin_button,
                             adjustment,
                             spin_button->climb_rate,
                             spin_button->digits);
}

/**
 * bobgui_spin_button_get_adjustment:
 * @spin_button: a `BobguiSpinButton`
 *
 * Get the adjustment associated with a `BobguiSpinButton`.
 *
 * Returns: (transfer none): the `BobguiAdjustment` of @spin_button
 */
BobguiAdjustment *
bobgui_spin_button_get_adjustment (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), NULL);

  return spin_button->adjustment;
}

/**
 * bobgui_spin_button_set_digits:
 * @spin_button: a `BobguiSpinButton`
 * @digits: the number of digits after the decimal point to be
 *   displayed for the spin button’s value
 *
 * Set the precision to be displayed by @spin_button.
 *
 * Up to 20 digit precision is allowed.
 */
void
bobgui_spin_button_set_digits (BobguiSpinButton *spin_button,
                            guint          digits)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (spin_button->digits != digits)
    {
      spin_button->digits = digits;
      bobgui_spin_button_value_changed (spin_button->adjustment, spin_button);
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_DIGITS]);

      /* since lower/upper may have changed */
      bobgui_widget_queue_resize (BOBGUI_WIDGET (spin_button));
    }
}

/**
 * bobgui_spin_button_get_digits:
 * @spin_button: a `BobguiSpinButton`
 *
 * Fetches the precision of @spin_button.
 *
 * Returns: the current precision
 **/
guint
bobgui_spin_button_get_digits (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), 0);

  return spin_button->digits;
}

/**
 * bobgui_spin_button_set_increments:
 * @spin_button: a `BobguiSpinButton`
 * @step: increment applied for a button 1 press.
 * @page: increment applied for a button 2 press.
 *
 * Sets the step and page increments for spin_button.
 *
 * This affects how quickly the value changes when
 * the spin button’s arrows are activated.
 */
void
bobgui_spin_button_set_increments (BobguiSpinButton *spin_button,
                                double         step,
                                double         page)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  bobgui_adjustment_configure (spin_button->adjustment,
                            bobgui_adjustment_get_value (spin_button->adjustment),
                            bobgui_adjustment_get_lower (spin_button->adjustment),
                            bobgui_adjustment_get_upper (spin_button->adjustment),
                            step,
                            page,
                            bobgui_adjustment_get_page_size (spin_button->adjustment));
}

/**
 * bobgui_spin_button_get_increments:
 * @spin_button: a `BobguiSpinButton`
 * @step: (out) (optional): location to store step increment
 * @page: (out) (optional): location to store page increment
 *
 * Gets the current step and page the increments
 * used by @spin_button.
 *
 * See [method@Bobgui.SpinButton.set_increments].
 */
void
bobgui_spin_button_get_increments (BobguiSpinButton *spin_button,
                                double        *step,
                                double        *page)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (step)
    *step = bobgui_adjustment_get_step_increment (spin_button->adjustment);
  if (page)
    *page = bobgui_adjustment_get_page_increment (spin_button->adjustment);
}

/**
 * bobgui_spin_button_set_range:
 * @spin_button: a `BobguiSpinButton`
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * Sets the minimum and maximum allowable values for @spin_button.
 *
 * If the current value is outside this range, it will be adjusted
 * to fit within the range, otherwise it will remain unchanged.
 */
void
bobgui_spin_button_set_range (BobguiSpinButton *spin_button,
                           double         min,
                           double         max)
{
  BobguiAdjustment *adjustment;

  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  adjustment = spin_button->adjustment;

  bobgui_adjustment_configure (adjustment,
                            CLAMP (bobgui_adjustment_get_value (adjustment), min, max),
                            min,
                            max,
                            bobgui_adjustment_get_step_increment (adjustment),
                            bobgui_adjustment_get_page_increment (adjustment),
                            bobgui_adjustment_get_page_size (adjustment));
}

/**
 * bobgui_spin_button_get_range:
 * @spin_button: a `BobguiSpinButton`
 * @min: (out) (optional): location to store minimum allowed value
 * @max: (out) (optional): location to store maximum allowed value
 *
 * Gets the range allowed for @spin_button.
 *
 * See [method@Bobgui.SpinButton.set_range].
 */
void
bobgui_spin_button_get_range (BobguiSpinButton *spin_button,
                           double        *min,
                           double        *max)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (min)
    *min = bobgui_adjustment_get_lower (spin_button->adjustment);
  if (max)
    *max = bobgui_adjustment_get_upper (spin_button->adjustment);
}

/**
 * bobgui_spin_button_get_value:
 * @spin_button: a `BobguiSpinButton`
 *
 * Get the value in the @spin_button.
 *
 * Returns: the value of @spin_button
 */
double
bobgui_spin_button_get_value (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), 0.0);

  return bobgui_adjustment_get_value (spin_button->adjustment);
}

/**
 * bobgui_spin_button_get_value_as_int:
 * @spin_button: a `BobguiSpinButton`
 *
 * Get the value @spin_button represented as an integer.
 *
 * Returns: the value of @spin_button
 */
int
bobgui_spin_button_get_value_as_int (BobguiSpinButton *spin_button)
{
  double val;

  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), 0);

  val = bobgui_adjustment_get_value (spin_button->adjustment);
  if (val - floor (val) < ceil (val) - val)
    return floor (val);
  else
    return ceil (val);
}

/**
 * bobgui_spin_button_set_value:
 * @spin_button: a `BobguiSpinButton`
 * @value: the new value
 *
 * Sets the value of @spin_button.
 */
void
bobgui_spin_button_set_value (BobguiSpinButton *spin_button,
                           double         value)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (fabs (value - bobgui_adjustment_get_value (spin_button->adjustment)) > EPSILON ||
      value < bobgui_adjustment_get_lower (spin_button->adjustment) ||
      value > bobgui_adjustment_get_bounded_upper (spin_button->adjustment))
    bobgui_adjustment_set_value (spin_button->adjustment, value);
  else
    {
      int return_val = FALSE;
      g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
      if (!return_val)
        bobgui_spin_button_default_output (spin_button);
    }

  spin_button->edited = FALSE;
}

/**
 * bobgui_spin_button_set_update_policy:
 * @spin_button: a `BobguiSpinButton`
 * @policy: a `BobguiSpinButtonUpdatePolicy` value
 *
 * Sets the update behavior of a spin button.
 *
 * This determines whether the spin button is always
 * updated or only when a valid value is set.
 */
void
bobgui_spin_button_set_update_policy (BobguiSpinButton             *spin_button,
                                   BobguiSpinButtonUpdatePolicy  policy)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  if (spin_button->update_policy != policy)
    {
      spin_button->update_policy = policy;
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_UPDATE_POLICY]);
    }
}

/**
 * bobgui_spin_button_get_update_policy:
 * @spin_button: a `BobguiSpinButton`
 *
 * Gets the update behavior of a spin button.
 *
 * See [method@Bobgui.SpinButton.set_update_policy].
 *
 * Returns: the current update policy
 */
BobguiSpinButtonUpdatePolicy
bobgui_spin_button_get_update_policy (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), BOBGUI_UPDATE_ALWAYS);

  return spin_button->update_policy;
}

/**
 * bobgui_spin_button_set_numeric:
 * @spin_button: a `BobguiSpinButton`
 * @numeric: flag indicating if only numeric entry is allowed
 *
 * Sets the flag that determines if non-numeric text can be typed
 * into the spin button.
 */
void
bobgui_spin_button_set_numeric (BobguiSpinButton *spin_button,
                             gboolean       numeric)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  numeric = numeric != FALSE;

  if (spin_button->numeric != numeric)
    {
      spin_button->numeric = numeric;
      bobgui_text_set_input_purpose (BOBGUI_TEXT (spin_button->entry),
                                  numeric ? BOBGUI_INPUT_PURPOSE_NUMBER: BOBGUI_INPUT_PURPOSE_FREE_FORM);
      bobgui_text_set_input_hints (BOBGUI_TEXT (spin_button->entry),
                                numeric ? BOBGUI_INPUT_HINT_NO_EMOJI : BOBGUI_INPUT_HINT_NONE);

      if (numeric)
        bobgui_widget_set_direction (spin_button->entry, BOBGUI_TEXT_DIR_LTR);

      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_NUMERIC]);
    }
}

/**
 * bobgui_spin_button_get_numeric:
 * @spin_button: a `BobguiSpinButton`
 *
 * Returns whether non-numeric text can be typed into the spin button.
 *
 * Returns: %TRUE if only numeric text can be entered
 */
gboolean
bobgui_spin_button_get_numeric (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->numeric;
}

/**
 * bobgui_spin_button_set_wrap:
 * @spin_button: a `BobguiSpinButton`
 * @wrap: a flag indicating if wrapping behavior is performed
 *
 * Sets the flag that determines if a spin button value wraps
 * around to the opposite limit when the upper or lower limit
 * of the range is exceeded.
 */
void
bobgui_spin_button_set_wrap (BobguiSpinButton  *spin_button,
                          gboolean        wrap)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  wrap = wrap != FALSE;

  if (spin_button->wrap != wrap)
    {
      spin_button->wrap = wrap;
      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_WRAP]);

      update_buttons_sensitivity (spin_button);
    }
}

/**
 * bobgui_spin_button_get_wrap:
 * @spin_button: a `BobguiSpinButton`
 *
 * Returns whether the spin button’s value wraps around to the
 * opposite limit when the upper or lower limit of the range is
 * exceeded.
 *
 * Returns: %TRUE if the spin button wraps around
 */
gboolean
bobgui_spin_button_get_wrap (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->wrap;
}

/**
 * bobgui_spin_button_set_snap_to_ticks:
 * @spin_button: a `BobguiSpinButton`
 * @snap_to_ticks: a flag indicating if invalid values should be corrected
 *
 * Sets the policy as to whether values are corrected to the
 * nearest step increment when a spin button is activated after
 * providing an invalid value.
 */
void
bobgui_spin_button_set_snap_to_ticks (BobguiSpinButton *spin_button,
                                   gboolean       snap_to_ticks)
{
  guint new_val;

  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  new_val = (snap_to_ticks != 0);

  if (new_val != spin_button->snap_to_ticks)
    {
      spin_button->snap_to_ticks = new_val;
      if (new_val && bobgui_editable_get_editable (BOBGUI_EDITABLE (spin_button->entry)))
        bobgui_spin_button_update (spin_button);

      g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_SNAP_TO_TICKS]);
    }
}

/**
 * bobgui_spin_button_get_snap_to_ticks:
 * @spin_button: a `BobguiSpinButton`
 *
 * Returns whether the values are corrected to the nearest step.
 *
 * Returns: %TRUE if values are snapped to the nearest step
 */
gboolean
bobgui_spin_button_get_snap_to_ticks (BobguiSpinButton *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->snap_to_ticks;
}

/**
 * bobgui_spin_button_set_climb_rate:
 * @spin_button: a `BobguiSpinButton`
 * @climb_rate: the rate of acceleration, must be >= 0
 *
 * Sets the acceleration rate for repeated changes when you
 * hold down a button or key.
 */
void
bobgui_spin_button_set_climb_rate (BobguiSpinButton  *spin_button,
                                double          climb_rate)
{
  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));
  g_return_if_fail (0.0 <= climb_rate);

  if (spin_button->climb_rate == climb_rate)
    return;

  spin_button->climb_rate = climb_rate;

  g_object_notify_by_pspec (G_OBJECT (spin_button), spinbutton_props[PROP_CLIMB_RATE]);
}

/**
 * bobgui_spin_button_get_climb_rate:
 * @spin_button: a `BobguiSpinButton`
 *
 * Returns the acceleration rate for repeated changes.
 *
 * Returns: the acceleration rate
 */
double
bobgui_spin_button_get_climb_rate (BobguiSpinButton  *spin_button)
{
  g_return_val_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button), 0.0);

  return spin_button->climb_rate;
}

/**
 * bobgui_spin_button_spin:
 * @spin_button: a `BobguiSpinButton`
 * @direction: a `BobguiSpinType` indicating the direction to spin
 * @increment: step increment to apply in the specified direction
 *
 * Increment or decrement a spin button’s value in a specified
 * direction by a specified amount.
 */
void
bobgui_spin_button_spin (BobguiSpinButton *spin_button,
                      BobguiSpinType    direction,
                      double         increment)
{
  BobguiAdjustment *adjustment;
  double diff;

  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  adjustment = spin_button->adjustment;

  /* for compatibility with the 1.0.x version of this function */
  if (increment != 0 && increment != bobgui_adjustment_get_step_increment (adjustment) &&
      (direction == BOBGUI_SPIN_STEP_FORWARD ||
       direction == BOBGUI_SPIN_STEP_BACKWARD))
    {
      if (direction == BOBGUI_SPIN_STEP_BACKWARD && increment > 0)
        increment = -increment;
      direction = BOBGUI_SPIN_USER_DEFINED;
    }

  switch (direction)
    {
    case BOBGUI_SPIN_STEP_FORWARD:

      bobgui_spin_button_real_spin (spin_button, bobgui_adjustment_get_step_increment (adjustment));
      break;

    case BOBGUI_SPIN_STEP_BACKWARD:

      bobgui_spin_button_real_spin (spin_button, -bobgui_adjustment_get_step_increment (adjustment));
      break;

    case BOBGUI_SPIN_PAGE_FORWARD:

      bobgui_spin_button_real_spin (spin_button, bobgui_adjustment_get_page_increment (adjustment));
      break;

    case BOBGUI_SPIN_PAGE_BACKWARD:

      bobgui_spin_button_real_spin (spin_button, -bobgui_adjustment_get_page_increment (adjustment));
      break;

    case BOBGUI_SPIN_HOME:

      diff = bobgui_adjustment_get_value (adjustment) - bobgui_adjustment_get_lower (adjustment);
      if (diff > EPSILON)
        bobgui_spin_button_real_spin (spin_button, -diff);
      break;

    case BOBGUI_SPIN_END:

      diff = bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_value (adjustment);
      if (diff > EPSILON)
        bobgui_spin_button_real_spin (spin_button, diff);
      break;

    case BOBGUI_SPIN_USER_DEFINED:

      if (increment != 0)
        bobgui_spin_button_real_spin (spin_button, increment);
      break;

    default:
      break;
    }
}

/**
 * bobgui_spin_button_update:
 * @spin_button: a `BobguiSpinButton`
 *
 * Manually force an update of the spin button.
 */
void
bobgui_spin_button_update (BobguiSpinButton *spin_button)
{
  double val;
  int error = 0;
  int return_val;

  g_return_if_fail (BOBGUI_IS_SPIN_BUTTON (spin_button));

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[INPUT_], 0, &val, &return_val);
  if (return_val == FALSE)
    {
      return_val = bobgui_spin_button_default_input (spin_button, &val);
      error = (return_val == BOBGUI_INPUT_ERROR);
    }
  else if (return_val == BOBGUI_INPUT_ERROR)
    error = 1;

  if (spin_button->update_policy == BOBGUI_UPDATE_ALWAYS)
    {
      if (val < bobgui_adjustment_get_lower (spin_button->adjustment))
        val = bobgui_adjustment_get_lower (spin_button->adjustment);
      else if (val > bobgui_adjustment_get_upper (spin_button->adjustment))
        val = bobgui_adjustment_get_upper (spin_button->adjustment);
    }
  else if ((spin_button->update_policy == BOBGUI_UPDATE_IF_VALID) &&
           (error ||
            val < bobgui_adjustment_get_lower (spin_button->adjustment) ||
            val > bobgui_adjustment_get_upper (spin_button->adjustment)))
    {
      bobgui_spin_button_value_changed (spin_button->adjustment, spin_button);
      return;
    }

  if (spin_button->snap_to_ticks)
    bobgui_spin_button_snap (spin_button, val);
  else
    bobgui_spin_button_set_value (spin_button, val);
}

BobguiText *
bobgui_spin_button_get_text_widget (BobguiSpinButton *spin_button)
{
  return BOBGUI_TEXT (spin_button->entry);
}

