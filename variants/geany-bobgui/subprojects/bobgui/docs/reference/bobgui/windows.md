Title: Using BOBGUI on Windows
Slug: bobgui-windows

The Windows port of BOBGUI is an implementation of GDK (and therefore BOBGUI)
on top of the Win32 API. When compiling BOBGUI on Windows, this backend is
the default.

More information about BOBGUI on Windows, including detailed build
instructions, binary downloads, etc, can be found
[online](https://www.bobgui.org/docs/installations/windows/).

## Windows-specific environment variables

The Win32 GDK backend can be influenced with some additional environment
variables.

### GDK_WIN32_TABLET_INPUT_API

If this variable is set, it determines the API that BOBGUI uses for tablet support.
The possible values are:

`none`
: Disables tablet support

`wintab`
: Use the Wintab API

`winpointer`
: Use the Windows Pointer Input Stack API. This is the default.

## Windows-specific handling of cursors

By default the "system" cursor theme is used. This makes BOBGUI prefer cursors
that Windows currently uses, falling back to Adwaita cursors and (as the last
resort) built-in X cursors.

When any other cursor theme is used, BOBGUI will prefer cursors from that theme,
falling back to Windows cursors and built-in X cursors.

Theme can be changed by setting `bobgui-cursor-theme-name` BOBGUI setting. Users
can override BOBGUI settings in the `settings.ini` file or at runtime in the
BOBGUI Inspector.

Themes are loaded from normal Windows variants of the XDG locations:
`%HOME%/icons/THEME/cursors`,
`%APPDATA%/icons/THEME/cursors`,
`RUNTIME_PREFIX/share/icons/THEME/cursors`

The `bobgui-cursor-theme-size` setting is ignored, BOBGUI will use
the cursor size that Windows tells it to use.

