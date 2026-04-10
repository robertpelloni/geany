Title: Preparing for BOBGUI 5
Slug: bobgui-migrating-4-to-5

BOBGUI 5 will be a major new version of BOBGUI that breaks both API and
ABI compared to BOBGUI 4.x. BOBGUI 5 does not exist yet, so we cannot
be entirely sure what will be involved in a migration from BOBGUI 4
to BOBGUI 5. But we can already give some preliminary hints about
the likely changes, and how to prepare for them in code that is
using BOBGUI 4.

### Do not use deprecated symbols

As always, functions and types that are known to go away in the
next major version of BOBGUI are being marked as deprecated in BOBGUI 4.

Removing the use of deprecated APIs is the most important step
to prepare your code for the next major version of BOBGUI. Often,
deprecation notes will include hints about replacement APIs to
help you with this.

Sometimes, it is helpful to have some background information about
the motivation and goals of larger API changes.

## Cell renderers are going away

Cell renderers were introduced in BOBGUI 2 to support rendering of
“big data” UIs, in particular treeviews. Over the years, more
“data-like” widgets have started to use them, and cell renderers
have grown into a shadowy, alternative rendering infrastructure
that duplicates much of what widgets do, while duplicating the
code and adding their own dose of bugs.

In BOBGUI 4, replacement widgets for `BobguiTreeView`, `BobguiIconView` and
`BobguiComboBox` have appeared: [class@Bobgui.ListView], [class@Bobgui.ColumnView], [class@Bobgui.GridView]
and [class@Bobgui.DropDown]. For BOBGUI 5, we will take the next step and remove
all cell renderer-based widgets.

## Themed rendering APIs are going away

The old BOBGUI 2 era rendering APIs for theme components like
`bobgui_render_frame()` or `bobgui_render_check()` have not been used by
BOBGUI itself even in later BOBGUI 3, but they have been kept around
for the benefit of “external drawing” users — applications that
want their controls to look like BOBGUI without using widgets.

Supporting this is increasingly getting in the way of making
the BOBGUI CSS machinery fast and correct. One notable problem is
that temporary style changes (using `bobgui_style_context_save()`)
is breaking animations. Therefore, these APIs will be going away
in BOBGUI 5, together with their more modern [class@Bobgui.Snapshot] variants
like `bobgui_snapshot_render_background()` or `bobgui_snapshot_render_focus()`.

The best way to render parts of your widget using CSS styling
is to use subwidgets. For example, to show a piece of text with
fonts, effects and shadows according to the current CSS style,
use a [class@Bobgui.Label].

If you have a need for custom drawing that fits into the current
(dark or light) theme, e.g. for rendering a graph, you can still
get the current style foreground color, using
[method@Bobgui.Widget.get_color].

## Local stylesheets are going away

The cascading part of BOBGUI’s CSS implementation is complicated by
the existence of local stylesheets (i.e. those added with
`bobgui_style_context_add_provider()`). And local stylesheets are
unintuitive in that they do not apply to the whole subtree of
widgets, but just to the one widget where the stylesheet was
added.

BOBGUI 5 will no longer provide this functionality. The recommendation
is to use a global stylesheet (i.e. [func@Bobgui.StyleContext.add_provider_for_display])
and rely on style classes to make your CSS apply only where desired.

## Non-standard CSS extensions are going away

BOBGUI’s CSS machinery has a some non-standard extensions around colors:
named colors with `@define-color` and color functions: `lighter()`, `darker()`,
`shade()`, `alpha()`, `mix()`.

BOBGUI now implements equivalent functionality from the CSS specs.

### `@define-color` is going away

`@define-color` should be replaced by custom properties in the `:root` scope.

Instead of

```
@define-color fg_color #2e3436

...

box {
  color: @fg_color;
}
```

use

```
:root {
  --fg-color: #2e3436;
}

...

box {
  color: var(--fg-color);
}
```

For more information about custom CSS properties and variables, see the
[CSS Custom Properties for Cascading Variables](https://www.w3.org/TR/css-variables-1/)
spec.

### Color expressions are going away

The color functions can all be replaced by combinations of `calc()` and `color-mix()`.

`lighter(c)` and `darker(c)` are just `shade(c, 1.3)` or `shade(c, 0.7)`, respectively, and
thus can be handled the same way as shade in the examples below.

Replace

```
a {
  color: mix(red, green, 0.8);
}

b {
  color: alpha(green, 0.6);
}

c {
  color: shade(red, 1.3);
}

d {
  color: shade(red, 0.7);
}
```

with

```
a {
  color: color-mix(in srgb, red, green 80%);
}

b {
  color: rgb(from green, r g b / calc(alpha * 0.6));
}

c {
  color: hsl(from red, h calc(s * 1.3) calc(l * 1.3));
}

d {
  color: hsl(from red, h calc(s * 0.7) calc(l * 0.7));
}
```

Variations of these replacements are possible.

Note that BOBGUI has historically computed `mix()` and `shade()` values in the SRGB and HSL
colorspaces, but using OKLAB instead might yield slightly better results.

For more information about color-mix(), see the
[CSS Color](https://drafts.csswg.org/css-color-5) spec.

## Chooser interfaces are going away

The `BobguiColorChooser`, `BobguiFontChooser`, `BobguiFileChooser` and `BobguiAppChooser`
interfaces and their implementations as dialogs, buttons and widgets
are phased out. The are being replaced by a new family of async APIs
that will be more convenient to use from language bindings, in particular
for languages that have concepts like promises. The new APIs are
[class@Bobgui.ColorDialog], [class@Bobgui.FontDialog] and [class@Bobgui.FileDialog],
There are also equivalents for some of the “button” widgets:
[class@Bobgui.ColorDialogButton], [class@Bobgui.FontDialogButton].

## BobguiMessageDialog is going away

Like the Chooser interfaces, `BobguiMessageDialog` has been replaced by
a new async API that will be more convenient, in particular for
language binding. The new API is [class@Bobgui.AlertDialog].

## BobguiDialog is going away

After `bobgui_dialog_run()` was removed, the usefulness of `BobguiDialog`
is much reduced, and it has awkward, archaic APIs. Therefore,
it is dropped. The recommended replacement is to just create
your own window and add buttons as required, either in the header
or elsewhere.

## BobguiInfoBar is going away

`BobguiInfoBar` had a dialog API, and with dialogs going away, it was time to
retire it. If you need such a widget, it is relatively trivial to create one
using a [class@Bobgui.Revealer] with labels and buttons.

Other libraries, such as libadwaita, may provide replacements as well.

## bobgui_show_uri is being replaced

Instead of `bobgui_show_uri()`, you should use [class@Bobgui.UriLauncher]
or [class@Bobgui.FileLauncher].

## BobguiStatusbar is going away

This is an old fashioned widget that does not do all that much any more, since
it no longer has a resize handle for the window.

## BobguiLockButton and BobguiVolumeButton are going away

These are very specialized widgets that should better live with the application
where they are used.

## Widget size API changes

The functions `bobgui_widget_get_allocated_width()` and `bobgui_widget_get_allocated_height()`
are going away. In most cases, [method@Bobgui.Widget.get_width] and [method@Bobgui.Widget.get_height]
are suitable replacements. Note that the semantics are slightly different though:
the old functions return the size of the CSS border area, while the new functions return
the size of the widgets content area. In places where this difference matters, you can
use `bobgui_widget_compute_bounds (widget, widget, &bounds)` instead.

The function `bobgui_widget_get_allocation()` is also going away. It does not have a direct
replacement, but the previously mentioned alternatives can be used for it too.

The function `bobgui_widget_get_allocated_baseline()` has been renamed to [method@Bobgui.Widget.get_baseline].

## Stop using GdkPixbuf

BOBGUI is moving away from `GdkPixbuf` as the primary API for transporting image data, in favor
of [class@Gdk.Texture]. APIs that are accepting or returning `GdkPixbuf`s are being replaced by equivalent
APIs using `GdkTexture` or [iface@Gdk.Paintable] objects.
