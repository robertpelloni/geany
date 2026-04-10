/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "bobguiapplicationprivate.h"
#include "bobguibuilder.h"
#include "bobguinative.h"
#import <gdk/macos/GdkMacosView.h>
#import <gdk/macos/GdkMacosWindow.h>
#import "bobguiapplication-quartz-private.h"

typedef struct
{
  guint cookie;
  BobguiApplicationInhibitFlags flags;
  char *reason;
  BobguiWindow *window;
} BobguiApplicationQuartzInhibitor;

static void
bobgui_application_quartz_inhibitor_free (BobguiApplicationQuartzInhibitor *inhibitor)
{
  g_free (inhibitor->reason);
  g_clear_object (&inhibitor->window);
  g_free (inhibitor);
}

typedef BobguiApplicationImplClass BobguiApplicationImplQuartzClass;

typedef struct
{
  BobguiApplicationImpl impl;

  BobguiActionMuxer *muxer;
  GMenu *combined;
  GMenuModel *standard_app_menu;

  GSList *inhibitors;
  int quit_inhibit;
  guint next_cookie;
  NSObject *delegate;
} BobguiApplicationImplQuartz;

G_DEFINE_TYPE (BobguiApplicationImplQuartz, bobgui_application_impl_quartz, BOBGUI_TYPE_APPLICATION_IMPL)

@interface BobguiApplicationQuartzDelegate : NSObject<NSApplicationDelegate>
{
  BobguiApplicationImplQuartz *quartz;
}

- (id)initWithImpl:(BobguiApplicationImplQuartz*)impl;
@end

@implementation BobguiApplicationQuartzDelegate
-(id)initWithImpl:(BobguiApplicationImplQuartz*)impl
{
  [super init];
  quartz = impl;
  return self;
}

-(NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{
  const gchar *quit_action_name = "quit";
  GActionGroup *action_group = G_ACTION_GROUP (quartz->impl.application);

  if (quartz->quit_inhibit != 0)
    return NSTerminateCancel;

  if (g_action_group_has_action (action_group, quit_action_name))
    {
      g_action_group_activate_action (action_group, quit_action_name, NULL);
      return NSTerminateCancel;
    }

  return NSTerminateNow;
}

-(void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
  GFile **files;
  int i;
  GApplicationFlags flags;

  flags = g_application_get_flags (G_APPLICATION (quartz->impl.application));

  if (~flags & G_APPLICATION_HANDLES_OPEN)
    {
      [theApplication replyToOpenOrPrint:NSApplicationDelegateReplyFailure];
      return;
    }

  files = g_new (GFile *, [filenames count]);

  for (i = 0; i < [filenames count]; i++)
    files[i] = g_file_new_for_path ([(NSString *)[filenames objectAtIndex:i] UTF8String]);

  g_application_open (G_APPLICATION (quartz->impl.application), files, [filenames count], "");

  for (i = 0; i < [filenames count]; i++)
    g_object_unref (files[i]);

  g_free (files);

  [theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
  return YES;
}
@end

@interface BobguiMacosContentView : GdkMacosView<NSMenuItemValidation>

/* In some cases BOBGUI pops up a native window, such as when opening or
 * saving a file. We map common actions such as undo, copy, paste, etc.
 * to selectors, so these actions can be activated in a native window.
 * As a concequence, we also need to implement them on our own view,
 * and activate the action to the focused widget.
 */

- (void) undo:(id)sender;
- (void) redo:(id)sender;

- (void) cut:(id)sender;
- (void) copy:(id)sender;
- (void) paste:(id)sender;

- (void) selectAll:(id)sender;
@end

@implementation BobguiMacosContentView

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
  if ([menuItem isKindOfClass:[GNSMenuItem class]])
    return [((GNSMenuItem *) menuItem) validateMenuItem:menuItem];
  return NO;
}

- (void)undo:(id)sender
{
  [self maybeActivateAction:"text.undo" sender:sender];
}

- (void)redo:(id)sender
{
  [self maybeActivateAction:"text.redo" sender:sender];
}

- (void)cut:(id)sender
{
  [self maybeActivateAction:"clipboard.cut" sender:sender];
}

- (void)copy:(id)sender
{
  [self maybeActivateAction:"clipboard.copy" sender:sender];
}

- (void)paste:(id)sender
{
  [self maybeActivateAction:"clipboard.paste" sender:sender];
}

- (void)selectAll:(id)sender
{
  [super selectAll:sender];
  [self maybeActivateAction:"selection.select-all" sender:sender];
}

-(void)maybeActivateAction:(const char*)actionName sender:(id)sender
{
  if ([sender isKindOfClass:[GNSMenuItem class]])
    [((GNSMenuItem *) sender) didSelectItem:sender];
  else
    g_warning ("%s: sender %s is not a GNSMenuItem", actionName, [[sender description] UTF8String]);
}

@end

static void
bobgui_application_impl_quartz_set_default_accels (BobguiApplicationImpl *impl)
{
  const char *pref_accel[] = {"<Meta>comma", NULL};
  const char *hide_others_accel[] = {"<Meta><Alt>h", NULL};
  const char *hide_accel[] = {"<Meta>h", NULL};
  const char *quit_accel[] = {"<Meta>q", NULL};
  const char *undo_accel[] = {"<Meta>z", NULL};
  const char *redo_accel[] = {"<Meta><Shift>z", NULL};
  const char *cut_accel[] = {"<Meta>x", NULL};
  const char *copy_accel[] = {"<Meta>c", NULL};
  const char *paste_accel[] = {"<Meta>v", NULL};
  const char *select_all_accel[] = {"<Meta>a", NULL};

  bobgui_application_set_accels_for_action (impl->application, "app.preferences", pref_accel);
  bobgui_application_set_accels_for_action (impl->application, "bobguiinternal.hide-others", hide_others_accel);
  bobgui_application_set_accels_for_action (impl->application, "bobguiinternal.hide", hide_accel);
  bobgui_application_set_accels_for_action (impl->application, "app.quit", quit_accel);
  bobgui_application_set_accels_for_action (impl->application, "text.undo", undo_accel);
  bobgui_application_set_accels_for_action (impl->application, "text.redo", redo_accel);
  bobgui_application_set_accels_for_action (impl->application, "clipboard.cut", cut_accel);
  bobgui_application_set_accels_for_action (impl->application, "clipboard.copy", copy_accel);
  bobgui_application_set_accels_for_action (impl->application, "clipboard.paste", paste_accel);
  bobgui_application_set_accels_for_action (impl->application, "selection.select-all", select_all_accel);
}

static void
bobgui_application_impl_quartz_hide (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
  [NSApp hide:NSApp];
}

static void
bobgui_application_impl_quartz_hide_others (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
  [NSApp hideOtherApplications:NSApp];
}

static void
bobgui_application_impl_quartz_show_all (GSimpleAction *action,
                                      GVariant      *parameter,
                                      gpointer       user_data)
{
  [NSApp unhideAllApplications:NSApp];
}

static GActionEntry bobgui_application_impl_quartz_actions[] = {
  { "hide",             bobgui_application_impl_quartz_hide        },
  { "hide-others",      bobgui_application_impl_quartz_hide_others },
  { "show-all",         bobgui_application_impl_quartz_show_all    }
};

static void
bobgui_application_impl_quartz_set_app_menu (BobguiApplicationImpl *impl,
                                          GMenuModel         *app_menu)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;

  /* If there are any items at all, then the first one is the app menu */
  if (g_menu_model_get_n_items (G_MENU_MODEL (quartz->combined)))
    g_menu_remove (quartz->combined, 0);

  if (app_menu)
    g_menu_prepend_submenu (quartz->combined, "Application", app_menu);
  else
    {
      GMenu *empty;

      /* We must preserve the rule that index 0 is the app menu */
      empty = g_menu_new ();
      g_menu_prepend_submenu (quartz->combined, "Application", G_MENU_MODEL (empty));
      g_object_unref (empty);
    }
}

static void
bobgui_application_impl_quartz_startup (BobguiApplicationImpl *impl,
                                     gboolean            support_save)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;
  GSimpleActionGroup *bobguiinternal;
  GMenuModel *menubar;

  quartz->delegate = [[BobguiApplicationQuartzDelegate alloc] initWithImpl:quartz];
  [NSApp setDelegate: (id<NSApplicationDelegate>)quartz->delegate];

  quartz->muxer = bobgui_action_muxer_new (NULL);
  bobgui_action_muxer_set_parent (quartz->muxer, bobgui_application_get_action_muxer (impl->application));

  bobgui_application_impl_quartz_set_default_accels (impl);

  /* and put code behind the 'special' accels */
  bobguiinternal = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (bobguiinternal), bobgui_application_impl_quartz_actions,
                                   G_N_ELEMENTS (bobgui_application_impl_quartz_actions), quartz);
  bobgui_application_insert_action_group (impl->application, "bobguiinternal", G_ACTION_GROUP (bobguiinternal));
  g_object_unref (bobguiinternal);

  /* now setup the menu */
  if (quartz->standard_app_menu == NULL)
    {
      BobguiBuilder *builder;

      /* If the user didn't fill in their own menu yet, add ours.
       *
       * The fact that we do this here ensures that we will always have the
       * app menu at index 0 in 'combined'.
       */
      builder = bobgui_builder_new_from_resource ("/org/bobgui/libbobgui/ui/bobguiapplication-quartz.ui");
      quartz->standard_app_menu = G_MENU_MODEL (g_object_ref (bobgui_builder_get_object (builder, "app-menu")));
      g_object_unref (builder);
    }

  bobgui_application_impl_quartz_set_app_menu (impl, quartz->standard_app_menu);

  /* This may or may not add an item to 'combined' */
  menubar = bobgui_application_get_menubar (impl->application);
  if (menubar == NULL)
    {
      BobguiBuilder *builder;

      /* Provide a fallback menu, so keyboard shortcuts work in native windows too.
       */
      builder = bobgui_builder_new_from_resource ("/org/bobgui/libbobgui/ui/bobguiapplication-quartz.ui");
      menubar = G_MENU_MODEL (g_object_ref (bobgui_builder_get_object (builder, "default-menu")));
      g_object_unref (builder);
    }

  bobgui_application_impl_set_menubar (impl, menubar);

  /* OK.  Now put it in the menu. */
  bobgui_application_impl_quartz_setup_menu (G_MENU_MODEL (quartz->combined), quartz->muxer);

  [NSApp finishLaunching];
}

static void
bobgui_application_impl_quartz_shutdown (BobguiApplicationImpl *impl)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;

  /* destroy our custom menubar */
  [NSApp setMainMenu:[[[NSMenu alloc] init] autorelease]];

  if (quartz->delegate)
    {
      [quartz->delegate release];
      quartz->delegate = NULL;
    }

  g_slist_free_full (quartz->inhibitors, (GDestroyNotify) bobgui_application_quartz_inhibitor_free);
  quartz->inhibitors = NULL;
}

static void
on_window_unmap_cb (BobguiApplicationImpl *impl,
                    BobguiWindow          *window)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;

  if ((GActionGroup *)window == bobgui_action_muxer_get_group (quartz->muxer, "win"))
    bobgui_action_muxer_remove (quartz->muxer, "win");
}

static void
bobgui_application_impl_quartz_active_window_changed (BobguiApplicationImpl *impl,
                                                   BobguiWindow          *window)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;

  /* Track unmapping of the window so we can clear the "win" field.
   * Without this, we might hold on to a reference of the window
   * preventing it from getting disposed.
   */
  if (window != NULL && !g_object_get_data (G_OBJECT (window), "quartz-muxer-unmap"))
    {
      gulong handler_id = g_signal_connect_object (window,
                                                   "unmap",
                                                   G_CALLBACK (on_window_unmap_cb),
                                                   impl,
                                                   G_CONNECT_SWAPPED);
      g_object_set_data (G_OBJECT (window),
                         "quartz-muxer-unmap",
                         GSIZE_TO_POINTER (handler_id));
    }

  bobgui_action_muxer_remove (quartz->muxer, "win");

  if (G_IS_ACTION_GROUP (window))
    bobgui_action_muxer_insert (quartz->muxer, "win", G_ACTION_GROUP (window));
}

static void
bobgui_application_impl_quartz_set_menubar (BobguiApplicationImpl *impl,
                                         GMenuModel         *menubar)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;

  /* If we have the menubar, it is a section at index '1' */
  if (g_menu_model_get_n_items (G_MENU_MODEL (quartz->combined)) > 1)
    g_menu_remove (quartz->combined, 1);

  if (menubar)
    g_menu_append_section (quartz->combined, NULL, menubar);
}

static guint
bobgui_application_impl_quartz_inhibit (BobguiApplicationImpl         *impl,
                                     BobguiWindow                  *window,
                                     BobguiApplicationInhibitFlags  flags,
                                     const char                 *reason)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;
  BobguiApplicationQuartzInhibitor *inhibitor;

  inhibitor = g_new (BobguiApplicationQuartzInhibitor, 1);
  inhibitor->cookie = ++quartz->next_cookie;
  inhibitor->flags = flags;
  inhibitor->reason = g_strdup (reason);
  inhibitor->window = window ? g_object_ref (window) : NULL;

  quartz->inhibitors = g_slist_prepend (quartz->inhibitors, inhibitor);

  if (flags & BOBGUI_APPLICATION_INHIBIT_LOGOUT)
    quartz->quit_inhibit++;

  return inhibitor->cookie;
}

static void
bobgui_application_impl_quartz_uninhibit (BobguiApplicationImpl *impl,
                                       guint               cookie)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) impl;
  GSList *iter;

  for (iter = quartz->inhibitors; iter; iter = iter->next)
    {
      BobguiApplicationQuartzInhibitor *inhibitor = iter->data;

      if (inhibitor->cookie == cookie)
        {
          if (inhibitor->flags & BOBGUI_APPLICATION_INHIBIT_LOGOUT)
            quartz->quit_inhibit--;
          bobgui_application_quartz_inhibitor_free (inhibitor);
          quartz->inhibitors = g_slist_delete_link (quartz->inhibitors, iter);
          return;
        }
    }

  g_warning ("Invalid inhibitor cookie");
}

static void
bobgui_application_impl_quartz_init (BobguiApplicationImplQuartz *quartz)
{
  /* This is required so that Cocoa is not going to parse the
     command line arguments by itself and generate OpenFile events.
     We already parse the command line ourselves, so this is needed
     to prevent opening files twice, etc. */
  [[NSUserDefaults standardUserDefaults] setObject:@"NO"
                                            forKey:@"NSTreatUnknownArgumentsAsOpen"];

  quartz->combined = g_menu_new ();
}

static void
bobgui_application_impl_quartz_finalize (GObject *object)
{
  BobguiApplicationImplQuartz *quartz = (BobguiApplicationImplQuartz *) object;

  g_clear_object (&quartz->combined);
  g_clear_object (&quartz->standard_app_menu);

  G_OBJECT_CLASS (bobgui_application_impl_quartz_parent_class)->finalize (object);
}

static void
bobgui_application_impl_quartz_class_init (BobguiApplicationImplClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  class->startup = bobgui_application_impl_quartz_startup;
  class->shutdown = bobgui_application_impl_quartz_shutdown;
  class->active_window_changed = bobgui_application_impl_quartz_active_window_changed;
  class->set_app_menu = bobgui_application_impl_quartz_set_app_menu;
  class->set_menubar = bobgui_application_impl_quartz_set_menubar;
  class->inhibit = bobgui_application_impl_quartz_inhibit;
  class->uninhibit = bobgui_application_impl_quartz_uninhibit;

  gobject_class->finalize = bobgui_application_impl_quartz_finalize;

  [GdkMacosWindow setContentViewClass:[BobguiMacosContentView class]];
}
