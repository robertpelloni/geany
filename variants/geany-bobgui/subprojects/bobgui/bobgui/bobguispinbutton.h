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

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_SPIN_BUTTON                  (bobgui_spin_button_get_type ())
#define BOBGUI_SPIN_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SPIN_BUTTON, BobguiSpinButton))
#define BOBGUI_IS_SPIN_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SPIN_BUTTON))

/**
 * BOBGUI_INPUT_ERROR:
 *
 * Constant to return from a signal handler for the ::input
 * signal in case of conversion failure.
 *
 * See [signal@Bobgui.SpinButton::input].
 */
#define BOBGUI_INPUT_ERROR -1

/**
 * BobguiSpinButtonUpdatePolicy:
 * @BOBGUI_UPDATE_ALWAYS: When refreshing your `BobguiSpinButton`, the value is
 *   always displayed
 * @BOBGUI_UPDATE_IF_VALID: When refreshing your `BobguiSpinButton`, the value is
 *   only displayed if it is valid within the bounds of the spin button's
 *   adjustment
 *
 * Determines whether the spin button displays values outside the adjustment
 * bounds.
 *
 * See [method@Bobgui.SpinButton.set_update_policy].
 */
typedef enum
{
  BOBGUI_UPDATE_ALWAYS,
  BOBGUI_UPDATE_IF_VALID
} BobguiSpinButtonUpdatePolicy;

/**
 * BobguiSpinType:
 * @BOBGUI_SPIN_STEP_FORWARD: Increment by the adjustments step increment.
 * @BOBGUI_SPIN_STEP_BACKWARD: Decrement by the adjustments step increment.
 * @BOBGUI_SPIN_PAGE_FORWARD: Increment by the adjustments page increment.
 * @BOBGUI_SPIN_PAGE_BACKWARD: Decrement by the adjustments page increment.
 * @BOBGUI_SPIN_HOME: Go to the adjustments lower bound.
 * @BOBGUI_SPIN_END: Go to the adjustments upper bound.
 * @BOBGUI_SPIN_USER_DEFINED: Change by a specified amount.
 *
 * The values of the BobguiSpinType enumeration are used to specify the
 * change to make in bobgui_spin_button_spin().
 */
typedef enum
{
  BOBGUI_SPIN_STEP_FORWARD,
  BOBGUI_SPIN_STEP_BACKWARD,
  BOBGUI_SPIN_PAGE_FORWARD,
  BOBGUI_SPIN_PAGE_BACKWARD,
  BOBGUI_SPIN_HOME,
  BOBGUI_SPIN_END,
  BOBGUI_SPIN_USER_DEFINED
} BobguiSpinType;


typedef struct _BobguiSpinButton              BobguiSpinButton;

GDK_AVAILABLE_IN_ALL
GType           bobgui_spin_button_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_configure          (BobguiSpinButton  *spin_button,
                                                    BobguiAdjustment  *adjustment,
                                                    double          climb_rate,
                                                    guint           digits);

GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_spin_button_new                (BobguiAdjustment  *adjustment,
                                                    double          climb_rate,
                                                    guint           digits);

GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_spin_button_new_with_range     (double   min,
                                                    double   max,
                                                    double   step);

GDK_AVAILABLE_IN_4_14
void            bobgui_spin_button_set_activates_default (BobguiSpinButton *spin_button,
                                                       gboolean       activates_default);

GDK_AVAILABLE_IN_4_14
gboolean        bobgui_spin_button_get_activates_default (BobguiSpinButton *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_adjustment     (BobguiSpinButton  *spin_button,
                                                    BobguiAdjustment  *adjustment);

GDK_AVAILABLE_IN_ALL
BobguiAdjustment*  bobgui_spin_button_get_adjustment     (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_digits         (BobguiSpinButton  *spin_button,
                                                    guint           digits);
GDK_AVAILABLE_IN_ALL
guint           bobgui_spin_button_get_digits         (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_increments     (BobguiSpinButton  *spin_button,
                                                    double          step,
                                                    double          page);
GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_get_increments     (BobguiSpinButton  *spin_button,
                                                    double         *step,
                                                    double         *page);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_range          (BobguiSpinButton  *spin_button,
                                                    double          min,
                                                    double          max);
GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_get_range          (BobguiSpinButton  *spin_button,
                                                    double         *min,
                                                    double         *max);

GDK_AVAILABLE_IN_ALL
double          bobgui_spin_button_get_value          (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
int             bobgui_spin_button_get_value_as_int   (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_value          (BobguiSpinButton  *spin_button,
                                                    double          value);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_update_policy  (BobguiSpinButton  *spin_button,
                                                    BobguiSpinButtonUpdatePolicy  policy);
GDK_AVAILABLE_IN_ALL
BobguiSpinButtonUpdatePolicy bobgui_spin_button_get_update_policy (BobguiSpinButton *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_numeric        (BobguiSpinButton  *spin_button,
                                                    gboolean        numeric);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_spin_button_get_numeric        (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_spin               (BobguiSpinButton  *spin_button,
                                                    BobguiSpinType     direction,
                                                    double          increment);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_wrap           (BobguiSpinButton  *spin_button,
                                                    gboolean        wrap);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_spin_button_get_wrap           (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_snap_to_ticks  (BobguiSpinButton  *spin_button,
                                                    gboolean        snap_to_ticks);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_spin_button_get_snap_to_ticks  (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_set_climb_rate     (BobguiSpinButton  *spin_button,
                                                    double          climb_rate);
GDK_AVAILABLE_IN_ALL
double          bobgui_spin_button_get_climb_rate     (BobguiSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void            bobgui_spin_button_update             (BobguiSpinButton  *spin_button);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSpinButton, g_object_unref)

G_END_DECLS

