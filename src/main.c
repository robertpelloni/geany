/*
 *      main.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2005 The Geany contributors
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

/* See libmain.c for the real entry-point code. */

#include "main.h"
#include "Application_C_Bridge.h"

int main(int argc, char **argv)
{
	// Initialize the modernized C++ Application core managers first.
	// This ensures our Go backend (geany-go), UI submodules, and plugins
	// are orchestrated via the new object-oriented foundation.
	GeanyApplicationHandle app = geany_application_new();
	geany_application_initialize(app, argc, argv);

	// Start Go and Rust backends
	GeanyGo_Initialize();
	GeanyRust_Initialize();

	// Hand off execution to the C++ core execution block.
	// For now, it returns immediately to allow the legacy C application
	// boot process to continue to setup the GTK event loop.
	geany_application_run(app);

	// Run legacy C initialization and GTK loop.
	int res = main_lib(argc, argv);

	// Once the GTK loop shuts down, perform C++ core tear down gracefully.
	geany_application_quit(app);

	// Shut down Go and Rust backends
	GeanyGo_Shutdown();
	GeanyRust_Shutdown();

	geany_application_free(app);

	return res;
}
