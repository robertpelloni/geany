Title: Dialogs
Slug: bobgui-dialogs

Dialogs in BOBGUI are asynchronous APIs to obtain certain objects, such as
files or fonts, or to initiate certain actions such as printing or to
provide information to the user. The commonality behind these is that
they are all high-level tasks that usually require user interaction.

## API

All dialogs follow the async/finish pattern with a [iface@Gio.AsyncResult]
object. This pattern is extensively used in GIO and in other places.

The dialog object itself functions as the _source_ object of the async
operation, and holds state that is needed to set up the operation. But
once the operation is started, the dialog object can be safely reused
or dropped.

Each async operation is provided as a pair of functions, one to _begin_
the async operation, and one to obtain the results. The second function
is by convention named _finish_, and it must be called from a callback
that the caller provides to the first function.

Other pieces that are by convention passed to the begin function include
an optional parent window (to attach dialog windows to) and a _cancellable_
object that can be used to programmatically cancel an ongoing async operation.

The callback that is passed to the begin function must have the signature
of a [callback@Gio.AsyncReadyCallback]:

    void callback (GObject      *source,
                   GAsyncResult *result,
                   gpointer      user_data)

## Example

Here is an example of the dialog API flow, for opening a file:

```
dialog = bobgui_file_dialog_new ();

/* Set up dialog here ... */

bobgui_file_dialog_open (dialog, window, NULL, file_selected, data);

g_object_unref (dialog);

/* Return to the mainloop to give the async op a chance to run */
```

The callback will look like this:

```
static void
file_selected (GObject      *source,
               GAsyncResult *result,
               gpointer      user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GError *error = NULL;
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, &error);
  if (!file)
    {
      /* Check if the user chose not to select a file */
      if (!g_error_matches (error, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED))
        {
          BobguiAlertDialog *alert;

          alert = bobgui_alert_dialog_new ("Something went wrong");
          bobgui_alert_dialog_set_detail (alert, error->message);
          bobgui_alert_dialog_show (alert, window);

          g_object_unref (alert);
        }

      g_error_free (error);
      return;
    }

  /* Do something with the file here ... */

  g_object_unref (file);
}
```

## Errors

There are many possible causes for async operations to fail. The ones
that are specifically related to this being a dialog API are captured
by the [error@Bobgui.DialogError] enumeration.

- A dialog can be cancelled programmatically (using the cancellable).
  In this case, the [error@Bobgui.DialogError.CANCELLED] error is raised
- A dialog can be dismissed by the user without selecting the requested
  object. In this case, the [error@Bobgui.DialogError.DISMISSED] error
  is raised

It is appropriate to handle the first with an explanation for why the
cancellation happened (e.g. "This took too long (timeout reached)"),
and it isn't necessary to show an error dialog for the latter (since
the user explicitly chose to dismiss the dialog).

## Details

There are some fine points in the async/finish pattern that are worth
drawing attention to:

- The dialog object is kept alive for the duration of the asynchronous
  operation, so it is safe to drop your reference after initiating the
  operation (unless you want to keep using the dialog object for multiple
  operations).
- The finish functions are not _nullable_—they only return `NULL` if an
  error occurred (which is why the 'dismissed by the user' case is handled
  as an error). This is relevant for language bindings.

## Existing dialogs

Here is a list of existing dialogs

| Object/Task  | BOBGUI Dialog               | Main API                               |
|--------------|--------------------------|----------------------------------------|
| Files        | [class@Bobgui.FileDialog]   | [method@Bobgui.FileDialog.open]           |
| Text Files   | [class@Bobgui.FileDialog]   | [method@Bobgui.FileDialog.open_text_file] |
| Folders      | [class@Bobgui.FileDialog]   | [method@Bobgui.FileDialog.select_folder]  |
| Fonts        | [class@Bobgui.FontDialog]   | [method@Bobgui.FontDialog.choose_font]    |
| Colors       | [class@Bobgui.ColorDialog]  | [method@Bobgui.ColorDialog.choose_rgba]   |
| Printing     | [class@Bobgui.PrintDialog]  | [method@Bobgui.PrintDialog.print]         |
| Alerts       | [class@Bobgui.AlertDialog]  | [method@Bobgui.AlertDialog.choose]        |
| URIs         | [class@Bobgui.UriLauncher]  | [method@Bobgui.UriLauncher.launch]        |
| Applications | [class@Bobgui.FileLauncher] | [method@Bobgui.FileLauncher.launch]       |

Note that many of the dialogs have other entry points, for example the
file dialog can open multiple files, or save to a file, and the font dialog
can choose font face or font family objects.

# Language Bindings

A big motivation for strictly following the async/finish pattern for dialogs
is that bindings for languages with support for promises can make this pattern
work seamlessly with their languages native async support.

Here is how the example above might look in JavaScript:

```
async someFunction() {
    const dialog = new Bobgui.FileDialog();

    // Set up dialog here...

    try {
        const file = await dialog.open(window, null);

        // Do something with the file here ...

    } catch (e) {
        logError(e, "Error opening file dialog:");
    }
}
```

## History

In past, dialogs in BOBGUI were widgets that were derived from `BobguiDialog`
(and ultimatively, from `BobguiWindow`), such as `BobguiFileChooserDialog` or
`BobguiColorChooserDialog`. This turned out to be limiting and inconvenient
when wrapping platform APIs that are often out-of-process.

Most of these APIs have been deprecated by now, and will be removed
in the next major version of BOBGUI.
