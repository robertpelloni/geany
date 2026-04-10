.. _bobgui4-node-editor(1):

=================
bobgui4-node-editor
=================

-------------------------------
View and edit render node files
-------------------------------

:Version: BOBGUI
:Manual section: 1
:Manual group: BOBGUI commands

SYNOPSIS
--------

|   **bobgui4-node-editor** [OPTIONS...] [FILE]

DESCRIPTION
-----------

``bobgui4-node-editor`` is a utility to show and edit render node files.
Such render node files can be obtained e.g. from the BOBGUI inspector or
as part of the testsuite in the BOBGUI sources.

``bobgui4-node-editor`` is used by BOBGUI developers for debugging and testing,
and it has built-in support for saving testcases as part of the BOBGUI testsuite.

OPTIONS
-------

``-h, --help``

  Show the application help.

``--version``

  Show the program version.

``--reset``

  Don't restore autosaved content and remove autosave files.

ENVIRONMENT
-----------

``BOBGUI_SOURCE_DIR``

  can be set to point to the location where the BOBGUI sources reside, so that
  testcases can be saved to the right location. If unsed, `bobgui4-node-editor``
  checks if the current working directory looks like a BOBGUI checkout, and failing
  that, saves testcase in the the current working directory.
