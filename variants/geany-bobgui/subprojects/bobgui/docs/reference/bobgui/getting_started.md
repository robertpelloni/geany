Title: Getting Started with BOBGUI
Slug: bobgui-getting-started

BOBGUI is a [widget toolkit](https://en.wikipedia.org/wiki/Widget_toolkit).
Each user interface created by BOBGUI consists of widgets. This is implemented
in C using [class@GObject.Object], an object-oriented framework for C. Widgets
are organized in a hierarchy. The window widget is the main container.
The user interface is then built by adding buttons, drop-down menus, input
fields, and other widgets to the window. If you are creating complex user
interfaces it is recommended to use BobguiBuilder and its BOBGUI-specific markup
description language, instead of assembling the interface manually.

BOBGUI is event-driven. The toolkit listens for events such as a click
on a button, and passes the event to your application.

This chapter contains some tutorial information to get you started with
BOBGUI programming. It assumes that you have BOBGUI, its dependencies and a C
compiler installed and ready to use. If you need to build BOBGUI itself first,
refer to the [Compiling the BOBGUI libraries](building.html) section in this
reference.

## Basics

To begin our introduction to BOBGUI, we'll start with a very simple
application. This program will create an empty 200 × 200 pixel
window.

![A window](window-default.png)

Create a new file with the following content named `example-0.c`.

```c
#include <bobgui/bobgui.h>

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
```

You can compile the program above with GCC using:

```
gcc $( pkg-config --cflags bobgui4 ) -o example-0 example-0.c $( pkg-config --libs bobgui4 )
```

For more information on how to compile a BOBGUI application, please
refer to the [Compiling BOBGUI Applications](compiling.html)
section in this reference.

All BOBGUI applications will, of course, include `bobgui/bobgui.h`, which declares
functions, types and macros required by BOBGUI applications.

Even if BOBGUI installs multiple header files, only the top-level `bobgui/bobgui.h`
header can be directly included by third-party code. The compiler will abort
with an error if any other BOBGUI header is directly included.

In a BOBGUI application, the purpose of the `main()` function is to create a
[class@Bobgui.Application] object and run it. In this example a [class@Bobgui.Application]
pointer named `app` is declared and then initialized using `bobgui_application_new()`.

When creating a [class@Bobgui.Application], you need to pick an application
identifier (a name) and pass it to [ctor@Bobgui.Application.new] as parameter. For
this example `org.bobgui.example` is used. For choosing an identifier for your
application, see [this guide](https://developer.gnome.org/documentation/tutorials/application-id.html).
Lastly, [ctor@Bobgui.Application.new] takes `GApplicationFlags` as input
for your application, if your application would have special needs.

Next the [activate signal](https://developer.gnome.org/documentation/tutorials/application.html) is
connected to the activate() function above the `main()` function. The `activate`
signal will be emitted when your application is launched with `g_application_run()`
on the line below. The `g_application_run()` call also takes as arguments the
command line arguments (the `argc` count and the `argv` string array).
Your application can override the command line handling, e.g. to open
files passed on the commandline.

Within `g_application_run()` the activate signal is sent and we then proceed
into the activate() function of the application. This is where we construct
our BOBGUI window, so that a window is shown when the application is launched.
The call to [ctor@Bobgui.ApplicationWindow.new] will create a new
[class@Bobgui.ApplicationWindow] and store a pointer to it in the `window` variable.
The window will have a frame, a title bar, and window controls depending on the
platform.

A window title is set using [`method@Bobgui.Window.set_title`]. This function
takes a `BobguiWindow` pointer and a string as input. As our `window` pointer
is a `BobguiWidget` pointer, we need to cast it to `BobguiWindow`; instead of
casting `window` via a typical C cast like `(BobguiWindow*)`, `window` can be
cast using the macro `BOBGUI_WINDOW()`. `BOBGUI_WINDOW()` will check if the
pointer is an instance of the `BobguiWindow` class, before casting, and emit a
warning if the check fails. More information about this convention can be
found [in the GObject documentation](https://docs.bobgui.org/gobject/concepts.html#conventions).

Finally the window size is set using [`method@Bobgui.Window.set_default_size`]
and the window is then shown by BOBGUI via [method@Bobgui.Window.present].

When you close the window, by (for example) pressing the X button, the
`g_application_run()` call returns with a number which is saved inside an
integer variable named `status`. Afterwards, the `BobguiApplication` object is
freed from memory with `g_object_unref()`. Finally the status integer is
returned and the application exits.

While the program is running, BOBGUI is receiving _events_. These are typically
input events caused by the user interacting with your program, but also things
like messages from the window manager or other applications. BOBGUI processes
these and as a result, _signals_ may be emitted on your widgets. Connecting
handlers for these signals is how you normally make your program do something
in response to user input.

The following example is slightly more complex, and tries to
showcase some of the capabilities of BOBGUI.

## Hello, World

In the long tradition of programming languages and libraries,
this example is called *Hello, World*.

![Hello, world](hello-world.png)

### Hello World in C

Create a new file with the following content named `example-1.c`.

```c
#include <bobgui/bobgui.h>

static void
print_hello (BobguiWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *button;
  BobguiWidget *box;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_halign (box, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_CENTER);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  button = bobgui_button_new_with_label ("Hello World");

  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (bobgui_window_destroy), window);

  bobgui_box_append (BOBGUI_BOX (box), button);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

```

You can compile the program above with GCC using:

```
gcc $( pkg-config --cflags bobgui4 ) -o example-1 example-1.c $( pkg-config --libs bobgui4 )
```

As seen above, `example-1.c` builds further upon `example-0.c` by adding a
button to our window, with the label "Hello World". Two new `BobguiWidget`
pointers are declared to accomplish this, `button` and `box`. The box
variable is created to store a [class@Bobgui.Box], which is one of BOBGUI's ways
of controlling the size and layout of widgets.

The `BobguiBox` widget is created with [ctor@Bobgui.Box.new], which takes a
[enum@Bobgui.Orientation] enumeration value as parameter. The buttons which
this box will contain can either be laid out horizontally or vertically.
This does not matter in this particular case, as we are dealing with only
one button. After initializing box with the newly created `BobguiBox`, the code
adds the box widget to the window widget using [`method@Bobgui.Window.set_child`].

Next the `button` variable is initialized in similar manner.
[`ctor@Bobgui.Button.new_with_label`] is called which returns a
[class@Bobgui.Button] to be stored in `button`. Afterwards `button` is added to
our `box`.

Using `g_signal_connect()`, the button is connected to a function in our app called
`print_hello()`, so that when the button is clicked, BOBGUI will call this function.
As the `print_hello()` function does not use any data as input, `NULL` is passed
to it. `print_hello()` calls `g_print()` with the string "Hello World" which will
print Hello World in a terminal if the BOBGUI application was started from one.

After connecting `print_hello()`, another signal is connected to the "clicked"
state of the button using `g_signal_connect_swapped()`. This functions is similar
to a `g_signal_connect()`, with the difference lying in how the callback function
is treated; `g_signal_connect_swapped()` allows you to specify what the callback
function should take as parameter by letting you pass it as data. In this case
the function being called back is [method@Bobgui.Window.destroy] and the `window` pointer
is passed to it. This has the effect that when the button is clicked, the whole
BOBGUI window is destroyed. In contrast if a normal `g_signal_connect()` were used
to connect the "clicked" signal with [method@Bobgui.Window.destroy], then the function
would be called on `button` (which would not go well, since the function expects
a `BobguiWindow` as argument).

The rest of the code in `example-1.c` is identical to `example-0.c`. The next
section will elaborate further on how to add several [class@Bobgui.Widget]s to your
BOBGUI application.

## Packing

When creating an application, you'll want to put more than one widget inside
a window. When you do so, it becomes important to control how each widget is
positioned and sized. This is where packing comes in.

BOBGUI comes with a large variety of _layout containers_ whose purpose it
is to control the layout of the child widgets that are added to them, like:

- [class@Bobgui.Box]
- [class@Bobgui.Grid]
- [class@Bobgui.Revealer]
- [class@Bobgui.Stack]
- [class@Bobgui.Overlay]
- [class@Bobgui.Paned]
- [class@Bobgui.Expander]
- [class@Bobgui.Fixed]

The following example shows how the [class@Bobgui.Grid] container lets you
arrange several buttons:

![Grid packing](grid-packing.png)

### Packing buttons

Create a new file with the following content named `example-2.c`.

```c
#include <bobgui/bobgui.h>

static void
print_hello (BobguiWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *button;

  /* create a new window, and set its title */
  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");

  /* Here we construct the container that is going pack our buttons */
  grid = bobgui_grid_new ();

  /* Pack the container in the window */
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  button = bobgui_button_new_with_label ("Button 1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the first button in the grid cell (0, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 0, 0, 1, 1);

  button = bobgui_button_new_with_label ("Button 2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the second button in the grid cell (1, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 1, 0, 1, 1);

  button = bobgui_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (bobgui_window_destroy), window);

  /* Place the Quit button in the grid cell (0, 1), and make it
   * span 2 columns.
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 0, 1, 2, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
```

You can compile the program above with GCC using:

```
gcc $( pkg-config --cflags bobgui4 ) -o example-2 example-2.c $( pkg-config --libs bobgui4 )
```

## Custom Drawing

Many widgets, like buttons, do all their drawing themselves. You just tell
them the label you want to see, and they figure out what font to use, draw
the button outline and focus rectangle, etc. Sometimes, it is necessary to
do some custom drawing. In that case, a [class@Bobgui.DrawingArea] might be the right
widget to use. It offers a canvas on which you can draw by setting its
draw function.

The contents of a widget often need to be partially or fully redrawn, e.g.
when another window is moved and uncovers part of the widget, or when the
window containing it is resized. It is also possible to explicitly cause a
widget to be redrawn, by calling [`method@Bobgui.Widget.queue_draw`]. BOBGUI takes
care of most of the details by providing a ready-to-use cairo context to the
draw function.

The following example shows how to use a draw function with [class@Bobgui.DrawingArea].
It is a bit more complicated than the previous examples, since it also
demonstrates input event handling with event controllers.

![Drawing](drawing.png)

### Drawing in response to input

Create a new file with the following content named `example-3.c`.

```c
#include <bobgui/bobgui.h>

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static void
resize_cb (BobguiWidget *widget,
           int        width,
           int        height,
           gpointer   data)
{
  if (surface)
    {
      cairo_surface_destroy (surface);
      surface = NULL;
    }

  if (bobgui_native_get_surface (bobgui_widget_get_native (widget)))
    {
      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            bobgui_widget_get_width (widget),
                                            bobgui_widget_get_height (widget));

      /* Initialize the surface to white */
      clear_surface ();
    }
}

/* Redraw the screen from the surface. Note that the draw
 * callback receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static void
draw_cb (BobguiDrawingArea *drawing_area,
         cairo_t        *cr,
         int             width,
         int             height,
         gpointer        data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (BobguiWidget *widget,
            double     x,
            double     y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  cairo_rectangle (cr, x - 3, y - 3, 6, 6);
  cairo_fill (cr);

  cairo_destroy (cr);

  /* Now invalidate the drawing area. */
  bobgui_widget_queue_draw (widget);
}

static double start_x;
static double start_y;

static void
drag_begin (BobguiGestureDrag *gesture,
            double          x,
            double          y,
            BobguiWidget      *area)
{
  start_x = x;
  start_y = y;

  draw_brush (area, x, y);
}

static void
drag_update (BobguiGestureDrag *gesture,
             double          x,
             double          y,
             BobguiWidget      *area)
{
  draw_brush (area, start_x + x, start_y + y);
}

static void
drag_end (BobguiGestureDrag *gesture,
          double          x,
          double          y,
          BobguiWidget      *area)
{
  draw_brush (area, start_x + x, start_y + y);
}

static void
pressed (BobguiGestureClick *gesture,
         int              n_press,
         double           x,
         double           y,
         BobguiWidget       *area)
{
  clear_surface ();
  bobgui_widget_queue_draw (area);
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy (surface);
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *frame;
  BobguiWidget *drawing_area;
  BobguiGesture *drag;
  BobguiGesture *press;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Drawing Area");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  frame = bobgui_frame_new (NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), frame);

  drawing_area = bobgui_drawing_area_new ();
  /* set a minimum size */
  bobgui_widget_set_size_request (drawing_area, 100, 100);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), drawing_area);

  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (drawing_area), draw_cb, NULL, NULL);

  g_signal_connect_after (drawing_area, "resize", G_CALLBACK (resize_cb), NULL);

  drag = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (drag), GDK_BUTTON_PRIMARY);
  bobgui_widget_add_controller (drawing_area, BOBGUI_EVENT_CONTROLLER (drag));
  g_signal_connect (drag, "drag-begin", G_CALLBACK (drag_begin), drawing_area);
  g_signal_connect (drag, "drag-update", G_CALLBACK (drag_update), drawing_area);
  g_signal_connect (drag, "drag-end", G_CALLBACK (drag_end), drawing_area);

  press = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (press), GDK_BUTTON_SECONDARY);
  bobgui_widget_add_controller (drawing_area, BOBGUI_EVENT_CONTROLLER (press));

  g_signal_connect (press, "pressed", G_CALLBACK (pressed), drawing_area);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
```

You can compile the program above with GCC using:

```
gcc $( pkg-config --cflags bobgui4 ) -o example-3 example-3.c $( pkg-config --libs bobgui4 )
```

## Building user interfaces

When constructing a more complicated user interface, with dozens
or hundreds of widgets, doing all the setup work in C code is
cumbersome, and making changes becomes next to impossible.

Thankfully, BOBGUI supports the separation of user interface
layout from your business logic, by using UI descriptions in an
XML format that can be parsed by the [class@Bobgui.Builder] class.

### Packing buttons with BobguiBuilder

Create a new file with the following content named `example-4.c`.

```c
#include <bobgui/bobgui.h>
#include <glib/gstdio.h>

static void
print_hello (BobguiWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
quit_cb (BobguiWindow *window)
{
  bobgui_window_close (window);
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  /* Construct a BobguiBuilder instance and load our UI description */
  BobguiBuilder *builder = bobgui_builder_new ();
  bobgui_builder_add_from_file (builder, "builder.ui", NULL);

  /* Connect signal handlers to the constructed widgets. */
  GObject *window = bobgui_builder_get_object (builder, "window");
  bobgui_window_set_application (BOBGUI_WINDOW (window), app);

  GObject *button = bobgui_builder_get_object (builder, "button1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = bobgui_builder_get_object (builder, "button2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = bobgui_builder_get_object (builder, "quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (quit_cb), window);

  bobgui_widget_set_visible (BOBGUI_WIDGET (window), TRUE);

  /* We do not need the builder any more */
  g_object_unref (builder);
}

int
main (int   argc,
      char *argv[])
{
#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  BobguiApplication *app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
```

Create a new file with the following content named `builder.ui`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <object id="window" class="BobguiWindow">
    <property name="title">Grid</property>
    <child>
      <object id="grid" class="BobguiGrid">
        <child>
          <object id="button1" class="BobguiButton">
            <property name="label">Button 1</property>
            <layout>
              <property name="column">0</property>
              <property name="row">0</property>
            </layout>
          </object>
        </child>
        <child>
          <object id="button2" class="BobguiButton">
            <property name="label">Button 2</property>
            <layout>
              <property name="column">1</property>
              <property name="row">0</property>
            </layout>
          </object>
        </child>
        <child>
          <object id="quit" class="BobguiButton">
            <property name="label">Quit</property>
            <layout>
              <property name="column">0</property>
              <property name="row">1</property>
              <property name="column-span">2</property>
            </layout>
          </object>
        </child>
      </object>
    </child>
  </object>
</interface>
```

You can compile the program above with GCC using:

```
gcc $( pkg-config --cflags bobgui4 ) -o example-4 example-4.c $( pkg-config --libs bobgui4 )
```

Note that `BobguiBuilder` can also be used to construct objects that are
not widgets, such as tree models, adjustments, etc. That is the reason
the method we use here is called [`method@Bobgui.Builder.get_object`] and
returns a `GObject` instead of a `BobguiWidget`.

Normally, you would pass a full path to [`method@Bobgui.Builder.add_from_file`] to
make the execution of your program independent of the current directory.
A common location to install UI descriptions and similar data is
`/usr/share/appname`.

It is also possible to embed the UI description in the source code as a
string and use [`method@Bobgui.Builder.add_from_string`] to load it. But keeping
the UI description in a separate file has several advantages:

- it is possible to make minor adjustments to the UI without recompiling your
  program
- it is easier to isolate the UI code from the business logic of your
  application
- it is easier to restructure your UI into separate classes using composite
  widget templates

Using [GResource](https://docs.bobgui.org/gio/struct.Resource.html) it is possible
to combine the best of both worlds: you can keep the UI definition files
separate inside your source code repository, and then ship them embedded into
your application.

## Building applications

An application consists of a number of files:

The binary
 : This gets installed in `/usr/bin`.

A desktop file
 : The desktop file provides important information about the application to
   the desktop shell, such as its name, icon, D-Bus name, commandline to launch
   it, etc. It is installed in `/usr/share/applications`.

An icon
 : The icon gets installed in `/usr/share/icons/hicolor/48x48/apps`, where it
   will be found regardless of the current theme.

A settings schema
 : If the application uses GSettings, it will install its schema in
   `/usr/share/glib-2.0/schemas`, so that tools like dconf-editor can find it.

Other resources
 : Other files, such as BobguiBuilder ui files, are best loaded from
   resources stored in the application binary itself. This eliminates the
   need for most of the files that would traditionally be installed in
   an application-specific location in `/usr/share`.

BOBGUI includes application support that is built on top of `GApplication`. In this
tutorial we'll build a simple application by starting from scratch, adding more
and more pieces over time. Along the way, we'll learn about [class@Bobgui.Application],
templates, resources, application menus, settings, [class@Bobgui.HeaderBar], [class@Bobgui.Stack],
[class@Bobgui.SearchBar], [class@Bobgui.ListBox], and more.

The full, buildable sources for these examples can be found in the
`examples` directory of the BOBGUI source distribution, or
[online](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples) in the BOBGUI
source code repository. You can build each example separately by using make
with the `Makefile.example` file. For more information, see the `README`
included in the examples directory.

### A trivial application

When using `BobguiApplication`, the `main()` function can be very simple. We just call
`g_application_run()` and give it an instance of our application class.

```c
#include <bobgui/bobgui.h>

#include "exampleapp.h"

int
main (int argc, char *argv[])
{
  return g_application_run (G_APPLICATION (example_app_new ()), argc, argv);
}
```

All the application logic is in the application class, which is a subclass of
`BobguiApplication`. Our example does not yet have any interesting functionality.
All it does is open a window when it is activated without arguments, and open
the files it is given, if it is started with arguments.

To handle these two cases, we override the `activate()` vfunc, which gets
called when the application is launched without commandline arguments, and
the `open()` virtual function, which gets called when the application is
launched with commandline arguments.

To learn more about `GApplication` entry points, consult the GIO
[documentation](https://docs.bobgui.org/gio/class.Application.html).

```c
#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleApp
{
  BobguiApplication parent;
};

G_DEFINE_TYPE(ExampleApp, example_app, BOBGUI_TYPE_APPLICATION);

static void
example_app_init (ExampleApp *app)
{
}

static void
example_app_activate (GApplication *app)
{
  ExampleAppWindow *win;

  win = example_app_window_new (EXAMPLE_APP (app));
  bobgui_window_present (BOBGUI_WINDOW (win));
}

static void
example_app_open (GApplication  *app,
                  GFile        **files,
                  int            n_files,
                  const char    *hint)
{
  GList *windows;
  ExampleAppWindow *win;
  int i;

  windows = bobgui_application_get_windows (BOBGUI_APPLICATION (app));
  if (windows)
    win = EXAMPLE_APP_WINDOW (windows->data);
  else
    win = example_app_window_new (EXAMPLE_APP (app));

  for (i = 0; i < n_files; i++)
    example_app_window_open (win, files[i]);

  bobgui_window_present (BOBGUI_WINDOW (win));
}

static void
example_app_class_init (ExampleAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = example_app_activate;
  G_APPLICATION_CLASS (class)->open = example_app_open;
}

ExampleApp *
example_app_new (void)
{
  return g_object_new (EXAMPLE_APP_TYPE,
                       "application-id", "org.bobgui.exampleapp",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
```

Another important class that is part of the application support in BOBGUI is
[class@Bobgui.ApplicationWindow]. It is typically subclassed as well. Our
subclass does not do anything yet, so we will just get an empty window.

```c
#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  BobguiApplicationWindow parent;
};

G_DEFINE_TYPE(ExampleAppWindow, example_app_window, BOBGUI_TYPE_APPLICATION_WINDOW);

static void
example_app_window_init (ExampleAppWindow *app)
{
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
}
```

As part of the initial setup of our application, we also
create an icon and a desktop file.

![An icon](exampleapp.png)

```
[Desktop Entry]
Type=Application
Name=Example
Icon=exampleapp
StartupNotify=true
Exec=@bindir@/exampleapp
```

Note that `@bindir@` needs to be replaced with the actual path to the binary
before this desktop file can be used.

Here is what we've achieved so far:

![An application](getting-started-app1.png)

This does not look very impressive yet, but our application is already
presenting itself on the session bus, it has single-instance semantics,
and it accepts files as commandline arguments.

### Populating the window

In this step, we use a `BobguiBuilder` template to associate a
`BobguiBuilder` ui file with our application window class.

Our simple ui file gives the window a title, and puts a `BobguiStack`
widget as the main content.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <template class="ExampleAppWindow" parent="BobguiApplicationWindow">
    <property name="title" translatable="yes">Example Application</property>
    <property name="default-width">600</property>
    <property name="default-height">400</property>
    <child>
      <object class="BobguiBox" id="content_box">
        <property name="orientation">vertical</property>
        <child>
          <object class="BobguiStack" id="stack"/>
        </child>
      </object>
    </child>
  </template>
</interface>
```

To make use of this file in our application, we revisit our
`BobguiApplicationWindow` subclass, and call
[`method@Bobgui.WidgetClass.set_template_from_resource`] from the class init
function to set the ui file as template for this class. We also
add a call to [`method@Bobgui.Widget.init_template`] in the instance init
function to instantiate the template for each instance of our
class.

```c
 ...

static void
example_app_window_init (ExampleAppWindow *win)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (win));
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/window.ui");
}

 ...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application2/exampleappwin.c))

You may have noticed that we used the `_from_resource()` variant of the function
that sets a template. Now we need to use
[GLib's resource functionality](https://docs.bobgui.org/gio/struct.Resource.html)
to include the ui file in the binary. This is commonly done by listing all resources
in a `.gresource.xml` file, such as this:

```c
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/bobgui/exampleapp">
    <file preprocess="xml-stripblanks">window.ui</file>
  </gresource>
</gresources>
```

This file has to be converted into a C source file that will be compiled and linked
into the application together with the other source files. To do so, we use the
`glib-compile-resources` utility:

```
glib-compile-resources exampleapp.gresource.xml --target=resources.c --generate-source
```

The gnome module of the [Meson build system](https://mesonbuild.com)
provides the [`gnome.compile_resources()`](https://mesonbuild.com/Gnome-module.html#gnomecompile_resources)
method for this task.

Our application now looks like this:

![The application](getting-started-app2.png)

### Opening files

In this step, we make our application show the content of all the files
that it is given on the commandline.

**Note: Providing filenames (e.g. `./exampleapp examplewin.c examplewin.h`) at
the command line is a requirement for example apps 3-9 to display as shown in
the screenshots below.**

To this end, we add a member to the struct of our application window subclass
and keep a reference to the `BobguiStack` there. The first member of the struct
should be the parent type from which the class is derived. Here,
`ExampleAppWindow` is derived from `BobguiApplicationWindow`. The
[`func@Bobgui.widget_class_bind_template_child`] function arranges things so that after
instantiating the template, the `stack` member of the struct will point to the
widget of the same name from the template.

```c
...

struct _ExampleAppWindow
{
  BobguiApplicationWindow parent;

  BobguiWidget *stack;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, BOBGUI_TYPE_APPLICATION_WINDOW)

...

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/window.ui");
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, stack);
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application3/exampleappwin.c))

Now we revisit the `example_app_window_open()` function that is called for each
commandline argument, and construct a BobguiTextView that we then add as a page
to the stack:

```c
...

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
  char *basename;
  BobguiWidget *scrolled, *view;
  char *contents;
  gsize length;

  basename = g_file_get_basename (file);

  scrolled = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (scrolled, TRUE);
  bobgui_widget_set_vexpand (scrolled, TRUE);
  view = bobgui_text_view_new ();
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_text_view_set_cursor_visible (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), view);
  bobgui_stack_add_titled (BOBGUI_STACK (win->stack), scrolled, basename, basename);

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      BobguiTextBuffer *buffer;

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  g_free (basename);
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application3/exampleappwin.c))

Lastly, we add a [class@Bobgui.StackSwitcher] to the titlebar area in the UI file, and we
tell it to display information about our stack.

The stack switcher gets all its information it needs to display tabs from
the stack that it belongs to. Here, we are passing the label to show for
each file as the last argument to the [`method@Bobgui.Stack.add_titled`]
function.

Our application is beginning to take shape:

![Application window](getting-started-app3.png)

### A menu

The menu is shown at the right side of the headerbar. It is meant to collect
infrequently used actions that affect the whole application.

Just like the window template, we specify our menu in a ui file, and add it
as a resource to our binary.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <menu id="menu">
    <section>
      <item>
        <attribute name="label" translatable="yes">_Preferences</attribute>
        <attribute name="action">app.preferences</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="label" translatable="yes">_Quit</attribute>
        <attribute name="action">app.quit</attribute>
      </item>
    </section>
  </menu>
</interface>
```

To make the menu appear, we have to load the ui file and associate the
resulting menu model with the menu button that we've added to the headerbar.
Since menus work by activating GActions, we also have to add a suitable set
of actions to our application.

Adding the actions is best done in the `startup()` vfunc, which is guaranteed
to be called once for each primary application instance:

```c
...

static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
  g_application_quit (G_APPLICATION (app));
}

static GActionEntry app_entries[] =
{
  { "preferences", preferences_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};

static void
example_app_startup (GApplication *app)
{
  BobguiBuilder *builder;
  GMenuModel *app_menu;
  const char *quit_accels[2] = { "&lt;Ctrl&gt;Q", NULL };

  G_APPLICATION_CLASS (example_app_parent_class)->startup (app);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  bobgui_application_set_accels_for_action (BOBGUI_APPLICATION (app),
                                         "app.quit",
                                         quit_accels);
}

static void
example_app_class_init (ExampleAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = example_app_startup;
  ...
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application4/exampleapp.c))

Our preferences menu item does not do anything yet, but the Quit menu item
is fully functional. Note that it can also be activated by the usual Ctrl-Q
shortcut. The shortcut was added with [`method@Bobgui.Application.set_accels_for_action`].

The application menu looks like this:

![Application window](getting-started-app4.png)

### A preference dialog

A typical application will have a some preferences that should be remembered
from one run to the next. Even for our simple example application, we may
want to change the font that is used for the content.

We are going to use [class@Gio.Settings] to store our preferences. `GSettings` requires
a schema that describes our settings:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<schemalist>
  <schema path="/org/bobgui/exampleapp/" id="org.bobgui.exampleapp">
    <key name="font" type="s">
      <default>'Monospace 12'</default>
      <summary>Font</summary>
      <description>The font to be used for content.</description>
    </key>
    <key name="transition" type="s">
      <choices>
        <choice value='none'/>
        <choice value='crossfade'/>
        <choice value='slide-left-right'/>
      </choices>
      <default>'none'</default>
      <summary>Transition</summary>
      <description>The transition to use when switching tabs.</description>
    </key>
  </schema>
</schemalist>
```

Before we can make use of this schema in our application, we need to compile
it into the binary form that GSettings expects. GIO provides macros to do
this in Autotools-based projects, and the gnome module of the Meson build
system provides the [`gnome.compile_schemas()`](https://mesonbuild.com/Gnome-module.html#gnomecompile_schemas)
method for this task.

Next, we need to connect our settings to the widgets that they are supposed
to control. One convenient way to do this is to use `GSettings` bind
functionality to bind settings keys to object properties, as we do here
for the transition setting.

```c
...

static void
example_app_window_init (ExampleAppWindow *win)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (win));
  win->settings = g_settings_new ("org.bobgui.exampleapp");

  g_settings_bind (win->settings, "transition",
                   win->stack, "transition-type",
                   G_SETTINGS_BIND_DEFAULT);
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application5/exampleappwin.c))

The code to connect the font setting is a little more involved, since there
is no simple object property that it corresponds to, so we are not going to
go into that here.

At this point, the application will already react if you change one of the
settings, e.g. using the `gsettings` command line tool. Of course, we expect
the application to provide a preference dialog for these. So lets do that
now. Our preference dialog will be a subclass of [class@Bobgui.Dialog], and
we'll use the same techniques that we've already seen: templates, private
structs, settings bindings.

Lets start with the template.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <template class="ExampleAppPrefs" parent="BobguiDialog">
    <property name="title" translatable="yes">Preferences</property>
    <property name="resizable">0</property>
    <property name="modal">1</property>
    <child internal-child="content_area">
      <object class="BobguiBox" id="content_area">
        <child>
          <object class="BobguiGrid" id="grid">
            <property name="margin-start">12</property>
            <property name="margin-end">12</property>
            <property name="margin-top">12</property>
            <property name="margin-bottom">12</property>
            <property name="row-spacing">12</property>
            <property name="column-spacing">12</property>
            <child>
              <object class="BobguiLabel" id="fontlabel">
                <property name="label">_Font:</property>
                <property name="use-underline">1</property>
                <property name="mnemonic-widget">font</property>
                <property name="xalign">1</property>
                <layout>
                  <property name="column">0</property>
                  <property name="row">0</property>
                </layout>
              </object>
            </child>
            <child>
              <object class="BobguiFontButton" id="font">
                <layout>
                  <property name="column">1</property>
                  <property name="row">0</property>
                </layout>
              </object>
            </child>
            <child>
              <object class="BobguiLabel" id="transitionlabel">
                <property name="label">_Transition:</property>
                <property name="use-underline">1</property>
                <property name="mnemonic-widget">transition</property>
                <property name="xalign">1</property>
                <layout>
                  <property name="column">0</property>
                  <property name="row">1</property>
                </layout>
              </object>
            </child>
            <child>
              <object class="BobguiComboBoxText" id="transition">
                <items>
                  <item translatable="yes" id="none">None</item>
                  <item translatable="yes" id="crossfade">Fade</item>
                  <item translatable="yes" id="slide-left-right">Slide</item>
                </items>
                <layout>
                  <property name="column">1</property>
                  <property name="row">1</property>
                </layout>
              </object>
            </child>
          </object>
        </child>
      </object>
    </child>
  </template>
</interface>
```

Next comes the dialog subclass.

```c
#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"
#include "exampleappprefs.h"

struct _ExampleAppPrefs
{
  BobguiDialog parent;

  GSettings *settings;
  BobguiWidget *font;
  BobguiWidget *transition;
};

G_DEFINE_TYPE (ExampleAppPrefs, example_app_prefs, BOBGUI_TYPE_DIALOG)

static void
example_app_prefs_init (ExampleAppPrefs *prefs)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (prefs));
  prefs->settings = g_settings_new ("org.bobgui.exampleapp");

  g_settings_bind (prefs->settings, "font",
                   prefs->font, "font",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs->settings, "transition",
                   prefs->transition, "active-id",
                   G_SETTINGS_BIND_DEFAULT);
}

static void
example_app_prefs_dispose (GObject *object)
{
  ExampleAppPrefs *prefs;

  prefs = EXAMPLE_APP_PREFS (object);

  g_clear_object (&prefs->settings);

  G_OBJECT_CLASS (example_app_prefs_parent_class)->dispose (object);
}

static void
example_app_prefs_class_init (ExampleAppPrefsClass *class)
{
  G_OBJECT_CLASS (class)->dispose = example_app_prefs_dispose;

  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/prefs.ui");
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppPrefs, font);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppPrefs, transition);
}

ExampleAppPrefs *
example_app_prefs_new (ExampleAppWindow *win)
{
  return g_object_new (EXAMPLE_APP_PREFS_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
```

Now we revisit the `preferences_activated()` function in our application
class, and make it open a new preference dialog.

```c
...

static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
  ExampleAppPrefs *prefs;
  BobguiWindow *win;

  win = bobgui_application_get_active_window (BOBGUI_APPLICATION (app));
  prefs = example_app_prefs_new (EXAMPLE_APP_WINDOW (win));
  bobgui_window_present (BOBGUI_WINDOW (prefs));
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application6/exampleapp.c))

After all this work, our application can now show a preference dialog
like this:

![Preference dialog](getting-started-app6.png)

### Adding a search bar

We continue to flesh out the functionality of our application. For now, we
add search. BOBGUI supports this with [class@Bobgui.SearchEntry] and
[class@Bobgui.SearchBar]. The search bar is a widget that can slide in from the
top to present a search entry.

We add a toggle button to the header bar, which can be used to slide out
the search bar below the header bar.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <template class="ExampleAppWindow" parent="BobguiApplicationWindow">
    <property name="title" translatable="yes">Example Application</property>
    <property name="default-width">600</property>
    <property name="default-height">400</property>
    <child type="titlebar">
      <object class="BobguiHeaderBar" id="header">
        <child type="title">
          <object class="BobguiStackSwitcher" id="tabs">
            <property name="stack">stack</property>
          </object>
        </child>
        <child type="end">
          <object class="BobguiMenuButton" id="gears">
            <property name="direction">none</property>
          </object>
        </child>
        <child type="end">
          <object class="BobguiToggleButton" id="search">
            <property name="sensitive">0</property>
            <property name="icon-name">edit-find-symbolic</property>
          </object>
        </child>
      </object>
    </child>
    <child>
      <object class="BobguiBox" id="content_box">
        <property name="orientation">vertical</property>
        <child>
          <object class="BobguiSearchBar" id="searchbar">
            <child>
              <object class="BobguiSearchEntry" id="searchentry">
                <signal name="search-changed" handler="search_text_changed"/>
              </object>
            </child>
          </object>
        </child>
        <child>
          <object class="BobguiStack" id="stack">
            <signal name="notify::visible-child" handler="visible_child_changed"/>
          </object>
        </child>
      </object>
    </child>
  </template>
</interface>
```

Implementing the search needs quite a few code changes that we are not
going to completely go over here. The central piece of the search
implementation is a signal handler that listens for text changes in
the search entry.

```c
...

static void
search_text_changed (BobguiEntry         *entry,
                     ExampleAppWindow *win)
{
  const char *text;
  BobguiWidget *tab;
  BobguiWidget *view;
  BobguiTextBuffer *buffer;
  BobguiTextIter start, match_start, match_end;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  if (text[0] == '\0')
    return;

  tab = bobgui_stack_get_visible_child (BOBGUI_STACK (win->stack));
  view = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (tab));
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));

  /* Very simple-minded search implementation */
  bobgui_text_buffer_get_start_iter (buffer, &start);
  if (bobgui_text_iter_forward_search (&start, text, BOBGUI_TEXT_SEARCH_CASE_INSENSITIVE,
                                    &match_start, &match_end, NULL))
    {
      bobgui_text_buffer_select_range (buffer, &match_start, &match_end);
      bobgui_text_view_scroll_to_iter (BOBGUI_TEXT_VIEW (view), &match_start,
                                    0.0, FALSE, 0.0, 0.0);
    }
}

static void
example_app_window_init (ExampleAppWindow *win)
{

...

  bobgui_widget_class_bind_template_callback (BOBGUI_WIDGET_CLASS (class), search_text_changed);

...

}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application7/exampleappwin.c))

With the search bar, our application now looks like this:

![A search bar](getting-started-app7.png)

### Adding a side bar

As another piece of functionality, we are adding a sidebar, which demonstrates
[class@Bobgui.MenuButton], [class@Bobgui.Revealer] and [class@Bobgui.ListBox].

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <template class="ExampleAppWindow" parent="BobguiApplicationWindow">
    <property name="title" translatable="yes">Example Application</property>
    <property name="default-width">600</property>
    <property name="default-height">400</property>
    <child type="titlebar">
      <object class="BobguiHeaderBar" id="header">
        <child type="title">
          <object class="BobguiStackSwitcher" id="tabs">
            <property name="stack">stack</property>
          </object>
        </child>
        <child type="end">
          <object class="BobguiToggleButton" id="search">
            <property name="sensitive">0</property>
            <property name="icon-name">edit-find-symbolic</property>
          </object>
        </child>
        <child type="end">
          <object class="BobguiMenuButton" id="gears">
            <property name="direction">none</property>
          </object>
        </child>
      </object>
    </child>
    <child>
      <object class="BobguiBox" id="content_box">
        <property name="orientation">vertical</property>
        <child>
          <object class="BobguiSearchBar" id="searchbar">
            <child>
              <object class="BobguiSearchEntry" id="searchentry">
                <signal name="search-changed" handler="search_text_changed"/>
              </object>
            </child>
          </object>
        </child>
        <child>
          <object class="BobguiBox" id="hbox">
            <child>
              <object class="BobguiRevealer" id="sidebar">
                <property name="transition-type">slide-right</property>
                <child>
                  <object class="BobguiScrolledWindow" id="sidebar-sw">
                    <property name="hscrollbar-policy">never</property>
                    <child>
                      <object class="BobguiListBox" id="words">
                        <property name="selection-mode">none</property>
                      </object>
                    </child>
                  </object>
                </child>
              </object>
            </child>
            <child>
              <object class="BobguiStack" id="stack">
                <signal name="notify::visible-child" handler="visible_child_changed"/>
              </object>
            </child>
          </object>
        </child>
      </object>
    </child>
  </template>
</interface>
```

The code to populate the sidebar with buttons for the words found in each
file is a little too involved to go into here. But we'll look at the code
to add a checkbutton for the new feature to the menu.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <menu id="menu">
    <section>
      <item>
        <attribute name="label" translatable="yes">_Words</attribute>
        <attribute name="action">win.show-words</attribute>
      </item>
      <item>
        <attribute name="label" translatable="yes">_Preferences</attribute>
        <attribute name="action">app.preferences</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="label" translatable="yes">_Quit</attribute>
        <attribute name="action">app.quit</attribute>
      </item>
    </section>
  </menu>
</interface>
```

To connect the menuitem to the show-words setting, we use
a `GAction` corresponding to the given `GSettings` key.

```c
...

static void
example_app_window_init (ExampleAppWindow *win)
{

...

  builder = bobgui_builder_new_from_resource ("/org/bobgui/exampleapp/gears-menu.ui");
  menu = G_MENU_MODEL (bobgui_builder_get_object (builder, "menu"));
  bobgui_menu_button_set_menu_model (BOBGUI_MENU_BUTTON (priv->gears), menu);
  g_object_unref (builder);

  action = g_settings_create_action (priv->settings, "show-words");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application8/exampleappwin.c))

What our application looks like now:

![A sidebar](getting-started-app8.png)

### Properties

Widgets and other objects have many useful properties.

Here we show some ways to use them in new and flexible ways, by wrapping
them in actions with [class@Gio.PropertyAction] or by binding them with
[class@GObject.Binding].

To set this up, we add two labels to the header bar in our window template,
named `lines_label` and `lines`, and bind them to struct members in the
private struct, as we've seen a couple of times by now.

We add a new "Lines" menu item to the gears menu, which triggers the
show-lines action:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <menu id="menu">
    <section>
      <item>
        <attribute name="label" translatable="yes">_Words</attribute>
        <attribute name="action">win.show-words</attribute>
      </item>
      <item>
        <attribute name="label" translatable="yes">_Lines</attribute>
        <attribute name="action">win.show-lines</attribute>
      </item>
      <item>
        <attribute name="label" translatable="yes">_Preferences</attribute>
        <attribute name="action">app.preferences</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="label" translatable="yes">_Quit</attribute>
        <attribute name="action">app.quit</attribute>
      </item>
    </section>
  </menu>
</interface>
```

To make this menu item do something, we create a property action for the
visible property of the `lines` label, and add it to the actions of the
window. The effect of this is that the visibility of the label gets toggled
every time the action is activated.

Since we want both labels to appear and disappear together, we bind
the visible property of the `lines_label` widget to the same property
of the `lines` widget.

```c
...

static void
example_app_window_init (ExampleAppWindow *win)
{
  ...

  action = (GAction*) g_property_action_new ("show-lines", win->lines, "visible");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);

  g_object_bind_property (win->lines, "visible",
                          win->lines_label, "visible",
                          G_BINDING_DEFAULT);
}

...
```

([full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application9/exampleappwin.c))

We also need a function that counts the lines of the currently active tab,
and updates the `lines` label. See the [full source](https://gitlab.gnome.org/GNOME/bobgui/blob/main/examples/application9/exampleappwin.c)
if you are interested in the details.

This brings our example application to this appearance:

![Full application](getting-started-app9.png)
