/*
 *      gtkcompat.h - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2012 The Geany contributors
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Compatibility header for the UI toolkit.
 *
 * All direct toolkit includes should flow through this file so the BTK
 * migration can be staged centrally instead of touching every source file
 * repeatedly.  Geany still targets the GTK3 API surface today, so this header
 * currently exposes GTK while acting as the integration seam for future
 * BTK-backed compatibility work.
 *
 * First-wave facade helpers live here for the highest-friction cross-toolkit
 * seams already touched by recent work:
 * - notebook/tab positioning
 * - widget CSS naming
 * - application CSS provider attachment/removal
 */

#ifndef GTK_COMPAT_H
#define GTK_COMPAT_H 1

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms-compat.h>

#define GEANY_TAB_POS_LEFT GTK_POS_LEFT
#define GEANY_TAB_POS_RIGHT GTK_POS_RIGHT
#define GEANY_TAB_POS_TOP GTK_POS_TOP
#define GEANY_TAB_POS_BOTTOM GTK_POS_BOTTOM

static inline void geany_notebook_set_tab_position(GtkNotebook *notebook, gint position)
{
	gtk_notebook_set_tab_pos(notebook, (GtkPositionType) position);
}


static inline void geany_widget_set_css_name(GtkWidget *widget, const gchar *name)
{
	gtk_widget_set_name(widget, name);
}


static inline GtkCssProvider *geany_css_provider_new(void)
{
	return gtk_css_provider_new();
}


static inline gboolean geany_css_provider_load_from_path(GtkCssProvider *provider,
	const gchar *path, GError **error)
{
	return gtk_css_provider_load_from_path(provider, path, error);
}


static inline void geany_style_context_add_provider_for_default_screen(
	GtkStyleProvider *provider, guint priority)
{
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), provider, priority);
}


static inline void geany_style_context_remove_provider_for_default_screen(
	GtkStyleProvider *provider)
{
	gtk_style_context_remove_provider_for_screen(gdk_screen_get_default(), provider);
}

#endif /* GTK_COMPAT_H */
