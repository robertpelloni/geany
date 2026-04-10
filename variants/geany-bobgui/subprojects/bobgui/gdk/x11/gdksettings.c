/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI+ at ftp://ftp.bobgui.org/pub/bobgui/.
 */

static const struct {
  const char *xname, *gdkname;
} gdk_settings_map[] = {
  {"Net/DoubleClickTime",     "bobgui-double-click-time"},
  {"Net/DoubleClickDistance", "bobgui-double-click-distance"},
  {"Net/DndDragThreshold",    "bobgui-dnd-drag-threshold"},
  {"Net/CursorBlink",         "bobgui-cursor-blink"},
  {"Net/CursorBlinkTime",     "bobgui-cursor-blink-time"},
  {"Net/ThemeName",           "bobgui-theme-name"},
  {"Net/IconThemeName",       "bobgui-icon-theme-name"},
  {"Bobgui/ColorPalette",        "bobgui-color-palette"},
  {"Bobgui/FontName",            "bobgui-font-name"},
  {"Bobgui/KeyThemeName",        "bobgui-key-theme-name"},
  {"Bobgui/CursorThemeName",     "bobgui-cursor-theme-name"},
  {"Bobgui/CursorThemeSize",     "bobgui-cursor-theme-size"},
  {"Bobgui/ColorScheme",         "bobgui-color-scheme"},
  {"Bobgui/EnableAnimations",    "bobgui-enable-animations"},
  {"Bobgui/ShowStatusStates",    "bobgui-show-status-shapes"},
  {"Xft/Antialias",           "bobgui-xft-antialias"},
  {"Xft/Hinting",             "bobgui-xft-hinting"},
  {"Xft/HintStyle",           "bobgui-xft-hintstyle"},
  {"Xft/RGBA",                "bobgui-xft-rgba"},
  {"Xft/DPI",                 "bobgui-xft-dpi"},
  {"Bobgui/EnableAccels",        "bobgui-enable-accels"},
  {"Bobgui/ScrolledWindowPlacement", "bobgui-scrolled-window-placement"},
  {"Bobgui/IMModule",            "bobgui-im-module"},
  {"Fontconfig/Timestamp",    "bobgui-fontconfig-timestamp"},
  {"Net/SoundThemeName",      "bobgui-sound-theme-name"},
  {"Net/EnableInputFeedbackSounds", "bobgui-enable-input-feedback-sounds"},
  {"Net/EnableEventSounds",   "bobgui-enable-event-sounds"},
  {"Bobgui/CursorBlinkTimeout",  "bobgui-cursor-blink-timeout"},
  {"Bobgui/ShellShowsAppMenu",   "bobgui-shell-shows-app-menu"},
  {"Bobgui/ShellShowsMenubar",   "bobgui-shell-shows-menubar"},
  {"Bobgui/ShellShowsDesktop",   "bobgui-shell-shows-desktop"},
  {"Bobgui/SessionBusId",        "bobgui-session-bus-id"},
  {"Bobgui/DecorationLayout",    "bobgui-decoration-layout"},
  {"Bobgui/TitlebarDoubleClick", "bobgui-titlebar-double-click"},
  {"Bobgui/TitlebarMiddleClick", "bobgui-titlebar-middle-click"},
  {"Bobgui/TitlebarRightClick", "bobgui-titlebar-right-click"},
  {"Bobgui/DialogsUseHeader",    "bobgui-dialogs-use-header"},
  {"Bobgui/EnablePrimaryPaste",  "bobgui-enable-primary-paste"},
  {"Bobgui/PrimaryButtonWarpsSlider", "bobgui-primary-button-warps-slider"},
  {"Bobgui/RecentFilesMaxAge",   "bobgui-recent-files-max-age"},
  {"Bobgui/RecentFilesEnabled",  "bobgui-recent-files-enabled"},
  {"Bobgui/KeynavUseCaret",      "bobgui-keynav-use-caret"},
  {"Bobgui/OverlayScrolling",    "bobgui-overlay-scrolling"},

  /* These are here in order to be recognized, but are not sent to
     bobgui as they are handled internally by gdk: */
  {"Gdk/WindowScalingFactor", "gdk-window-scaling-factor"},
  {"Gdk/UnscaledDPI",         "gdk-unscaled-dpi"}
};

static const char *
gdk_from_xsettings_name (const char *xname)
{
  static GHashTable *hash = NULL;
  guint i;

  if (G_UNLIKELY (hash == NULL))
  {
    hash = g_hash_table_new (g_str_hash, g_str_equal);

    for (i = 0; i < G_N_ELEMENTS (gdk_settings_map); i++)
      g_hash_table_insert (hash, (gpointer)gdk_settings_map[i].xname,
                                 (gpointer)gdk_settings_map[i].gdkname);
  }

  return g_hash_table_lookup (hash, xname);
}

