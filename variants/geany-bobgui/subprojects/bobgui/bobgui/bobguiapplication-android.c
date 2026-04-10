/*
 * Copyright (c) 2024 Florian "sp1rit" <sp1rit@disroot.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Florian "sp1rit" <sp1rit@disroot.org>
 */

#include "config.h"

#include "bobguiapplicationprivate.h"

typedef BobguiApplicationImplClass BobguiApplicationImplAndroidClass;

typedef struct {
  BobguiApplicationImpl impl;
} BobguiApplicationImplAndroid;

G_DEFINE_TYPE (BobguiApplicationImplAndroid, bobgui_application_impl_android, BOBGUI_TYPE_APPLICATION_IMPL)

static guint
bobgui_application_impl_android_inhibit (BobguiApplicationImpl         *impl,
                                      BobguiWindow                  *window,
                                      BobguiApplicationInhibitFlags  flags,
                                      const char                 *reason)
{
  // if (flags & BOBGUI_APPLICATION_INHIBIT_SUSPEND)
    // TODO: iterate over all active surfaces and call toplevel_inhibit_suspend for all toplevels
    // potentionally for BOBGUI_APPLICATION_INHIBIT_SWITCH lockTask mode?
  return 0;
}

static void
bobgui_application_impl_android_uninhibit (BobguiApplicationImpl *impl, guint cookie)
{}

static void
bobgui_application_impl_android_init (BobguiApplicationImplAndroid *self)
{}

static void
bobgui_application_impl_android_class_init (BobguiApplicationImplClass *klass)
{
  klass->inhibit = bobgui_application_impl_android_inhibit;
  klass->uninhibit = bobgui_application_impl_android_uninhibit;
}
