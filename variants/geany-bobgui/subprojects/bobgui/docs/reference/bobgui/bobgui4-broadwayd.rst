.. _bobgui4-broadwayd(1):

==============
bobgui4-broadwayd
==============

---------------------------
The Broadway display server
---------------------------

:Version: BOBGUI
:Manual section: 1
:Manual group: BOBGUI commands

SYNOPSIS
--------
|   **bobgui4-broadwayd** [OPTIONS...] <DISPLAY>
|   **bobgui4-broadwayd** --port=PORT --address=ADDRESS <DISPLAY>
|   **bobgui4-broadwayd** --unixaddress=ADDRESS <DISPLAY>


DESCRIPTION
-----------

``bobgui4-broadwayd`` is a display server for the Broadway GDK backend. It allows
multiple BOBGUI applications to display their windows in the same web browser, by
connecting to bobgui4-broadwayd.

When using bobgui4-broadwayd, specify the display number to use, prefixed with a
colon, similar to X. The default display number is 0.

::

   bobgui4-broadwayd :5


Then point your web browser at ``http://127.0.0.1:8085``.

Start your applications like this:

::

   GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 bobgui4-demo


OPTIONS
-------

``--port PORT``

  Use the given ``PORT`` for the HTTP connection, instead of the default ``8080 + (DISPLAY - 1)``.

``--address ADDRESS``

  Use the given ``address`` for the HTTP connection, instead of the default ``http://127.0.0.1``.

``--unixsocket ADDRESS``

  Use the given ``address`` as the unix domain socket address. This option
  overrides ``--address`` and ``--port``, and it is available only on Unix-like
  systems.
