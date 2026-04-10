.. _bobgui4-builder-tool(1):

=================
bobgui4-builder-tool
=================

-----------------------
BobguiBuilder File Utility
-----------------------

:Version: BOBGUI
:Manual section: 1
:Manual group: BOBGUI commands

SYNOPSIS
--------
|   **bobgui4-builder-tool** <COMMAND> [OPTIONS...] <FILE>
|
|   **bobgui4-builder-tool** validate [OPTIONS...] <FILE>
|   **bobgui4-builder-tool** enumerate [OPTIONS...] <FILE>
|   **bobgui4-builder-tool** simplify [OPTIONS...] <FILE>
|   **bobgui4-builder-tool** preview [OPTIONS...] <FILE>
|   **bobgui4-builder-tool** render [OPTIONS...] <FILE>
|   **bobgui4-builder-tool** screenshot [OPTIONS...] <FILE>

DESCRIPTION
-----------

``bobgui4-builder-tool`` can perform various operations on BobguiBuilder UI definition
files.

COMMANDS
--------

Validation
^^^^^^^^^^

The ``validate`` command validates the given UI definition file and reports
errors to ``stderr``.

Note that there are limitations to the validation that can be done for templates,
since they are closely tied to the class_init function they are used in.
If your UI file uses types from third-party libraries, it may help to add those
libraries to the `LD_PRELOAD` environment variable.

``--deprecations``

  Warn about uses of deprecated types in the UI definition file.

Enumeration
^^^^^^^^^^^

The ``enumerate`` command prints all the named objects that are present in the UI
definition file.

``--callbacks``

  Print the names of callbacks as well.

Preview
^^^^^^^

The ``preview`` command displays the UI definition file.

This command accepts options to specify the ID of the toplevel object and a CSS
file to use.

``--id=ID``

  The ID of the object to preview. If not specified, bobgui4-builder-tool will
  choose a suitable object on its own.

``--css=FILE``

  Load style information from the given CSS file.

Render
^^^^^^

The ``render`` command saves a rendering of the UI definition file as a png image
or node file. The name of the file to write can be specified as a second FILE argument.

This command accepts options to specify the ID of the toplevel object and a CSS
file to use.

``--id=ID``

  The ID of the object to preview. If not specified, bobgui4-builder-tool will
  choose a suitable object on its own.

``--css=FILE``

  Load style information from the given CSS file.

``--node``

  Write a serialized node file instead of a png image.

``--force``

  Overwrite an existing file.

Screenshot
^^^^^^^^^^

The ``screenshot`` command is an alias for ``render``.

Simplification
^^^^^^^^^^^^^^

The ``simplify`` command simplifies the UI definition file by removing
properties that are set to their default values and writes the resulting XML to
the standard output, or back to the input file.

When the ``--3to4`` option is specified, the ``simplify`` command interprets the
input as a BOBGUI 3 UI definuition file and attempts to convert it to BOBGUI 4
equivalents. It performs various conversions, such as renaming properties,
translating child properties to layout properties, rewriting the setup for
BobguiNotebook, BobguiStack, BobguiAssistant  or changing toolbars into boxes.

You should always test the modified UI definition files produced by
bobgui4-builder-tool before using them in production.

Note in particular that the conversion done with ``--3to4`` is meant as a
starting point for a port from BOBGUI 3 to BOBGUI 4. It is expected that you will have
to do manual fixups  after the initial conversion.

``--replace``

  Write the content back to the UI definition file instead of using the standard
  output.

``--3to4``

  Transform a BOBGUI 3 UI definition file to the equivalent BOBGUI 4 definitions.
