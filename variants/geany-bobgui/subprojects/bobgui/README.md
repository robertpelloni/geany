BOBGUI — The BOBGUI toolkit
=====================

[![Build status](https://gitlab.gnome.org/GNOME/bobgui/badges/main/pipeline.svg)](https://gitlab.gnome.org/GNOME/bobgui/-/commits/main)

General information
-------------------

BOBGUI is a multi-platform toolkit for creating graphical user interfaces.
Offering a complete set of widgets, BOBGUI is suitable for projects ranging
from small one-off projects to complete application suites.

BOBGUI is a free and open-source software project. The licensing terms
for BOBGUI, the GNU LGPL, allow it to be used by all developers, including those
developing proprietary software, without any license fees or royalties.

BOBGUI is hosted by the GNOME project (thanks!) and used by a wide variety
of applications and projects.

The official download location

  - https://download.gnome.org/sources/bobgui/

The official web site

  - https://www.bobgui.org

The official developers blog

  - https://blog.bobgui.org

Discussion forum

  - https://discourse.gnome.org/c/platform/core/

Nightly documentation can be found at
  - Bobgui: https://gnome.pages.gitlab.gnome.org/bobgui/bobgui4/
  - Gdk: https://gnome.pages.gitlab.gnome.org/bobgui/gdk4/
  - Gsk: https://gnome.pages.gitlab.gnome.org/bobgui/gsk4/

Nightly flatpaks of our demos can be installed from the
[GNOME Nightly](https://nightly.gnome.org/) repository:

```sh
flatpak remote-add --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
flatpak install gnome-nightly org.bobgui.Demo4
flatpak install gnome-nightly org.bobgui.WidgetFactory4
flatpak install gnome-nightly org.bobgui.IconBrowser4
```

Building and installing
-----------------------

In order to build BOBGUI you will need:

  - [a C11 compatible compiler](https://gitlab.gnome.org/GNOME/glib/-/blob/main/docs/toolchain-requirements.md)
  - [Python 3](https://www.python.org/)
  - [Meson](http://mesonbuild.com)
  - [Ninja](https://ninja-build.org)

You will also need various dependencies, based on the platform you are
building for:

  - [GLib](https://download.gnome.org/sources/glib/)
  - [GdkPixbuf](https://download.gnome.org/sources/gdk-pixbuf/)
  - [GObject-Introspection](https://download.gnome.org/sources/gobject-introspection/)
  - [Cairo](https://www.cairographics.org/)
  - [Pango](https://download.gnome.org/sources/pango/)
  - [Epoxy](https://github.com/anholt/libepoxy)
  - [Graphene](https://github.com/ebassi/graphene)
  - [Xkb-common](https://github.com/xkbcommon/libxkbcommon)

If you are building the Wayland backend, you will also need:

  - Wayland-client
  - Wayland-protocols
  - Wayland-cursor
  - Wayland-EGL

If you are building the X11 backend, you will also need:

  - Xlib, and the following X extensions:
    - xrandr
    - xrender
    - xi
    - xext
    - xfixes
    - xcursor
    - xdamage
    - xcomposite

Once you have all the necessary dependencies, you can build BOBGUI by using
Meson:

```sh
$ meson setup _build
$ meson compile -C_build
```

You can run the test suite using:

```sh
$ meson test -C_build
```

And, finally, you can install BOBGUI using:

```
$ sudo meson install -C_build
```

Complete information about installing BOBGUI and related libraries
can be found in the file:

```
docs/reference/bobgui/html/bobgui-building.html
```

Or [online](https://docs.bobgui.org/bobgui4/building.html)

Building from git
-----------------

The BOBGUI sources are hosted on [gitlab.gnome.org](http://gitlab.gnome.org). The main
development branch is called `main`, and stable branches are named after their minor
version, for example `bobgui-4-10`.

How to report bugs
------------------

Bugs should be reported on the [issues page](https://gitlab.gnome.org/GNOME/bobgui/issues/).

In the bug report please include:

* Information about your system. For instance:

   - which version of BOBGUI you are using
   - what operating system and version
   - what windowing system (X11 or Wayland)
   - what graphics driver / mesa version
   - for Linux, which distribution
   - if you built BOBGUI, the list of options used to configure the build

  Most of this information can be found in the BOBGUI inspector.

  And anything else you think is relevant.

* How to reproduce the bug.

  If you can reproduce it with one of the demo applications that are
  built in the demos/ subdirectory, on one of the test programs that
  are built in the tests/ subdirectory, that will be most convenient.
  Otherwise, please include a short test program that exhibits the
  behavior. As a last resort, you can also provide a pointer to a
  larger piece of software that can be downloaded.

* If the bug was a crash, the exact text that was printed out
  when the crash occurred.

* Further information such as stack traces may be useful, but
  is not necessary.

Contributing to BOBGUI
-------------------

Please, follow the [contribution guide](./CONTRIBUTING.md) to know how to
start contributing to BOBGUI.

If you want to support BOBGUI financially, please consider donating to
the GNOME project, which runs the infrastructure hosting BOBGUI.

Release notes
-------------

The release notes for BOBGUI are part of the migration guide in the API
reference. See:

 - [3.x release notes](https://developer.gnome.org/bobgui3/stable/bobgui-migrating-2-to-3.html)
 - [4.x release notes](https://docs.bobgui.org/bobgui4/migrating-3to4.html)

Licensing terms
---------------

BOBGUI is released under the terms of the GNU Lesser General Public License,
version 2.1 or, at your option, any later version, as published by the Free
Software Foundation.

Please, see the [`COPYING`](./COPYING) file for further information.

BOBGUI includes a small number of source files under the Apache license:
- A fork of the roaring bitmaps implementation in [bobgui/roaring](./bobgui/roaring)
- An adaptation of timsort from python in [bobgui/timsort](./bobgui/timsort)
