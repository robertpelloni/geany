.. _bobgui4-demo(1):

=========
bobgui4-demo
=========

-----------------------
Demonstrate BOBGUI widgets
-----------------------

:Version: BOBGUI
:Manual section: 1
:Manual group: BOBGUI commands

SYNOPSIS
--------

|   **bobgui4-demo** [OPTIONS...]

DESCRIPTION
-----------

``bobgui4-demo`` is a collection of examples.

Its purpose is to demonstrate many BOBGUI widgets in a form that is useful to
application developers.

The application shows the source code for each example, as well as other used
resources, such as UI description files and image assets.

OPTIONS
-------

``-h, --help``

  Show help options.

``--version``

  Show program version.

``--list``

  List available examples.

``--run EXAMPLE``

  Run the named example. Use ``--list`` to see the available examples.

``--autoquit``

  Quit after a short timeout. This is intended for use with ``--run``, e.g. when profiling.
