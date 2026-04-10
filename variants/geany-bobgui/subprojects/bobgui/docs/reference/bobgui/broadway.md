Title: The Broadway windowing system
Slug: broadway

The GDK Broadway backend provides support for displaying BOBGUI applications in
a web browser, using HTML5 and web sockets.

Broadway was written as an experiment and is not the most actively developed
backend. It supports the features that were required of GDK backends in BOBGUI 4.0,
but may not be up-to-date with the latest developments.

## Using BOBGUI with Broadway

To run your application under Broadway, first run the broadway server,
`bobgui-broadwayd`, that ships with BOBGUI:

```
bobgui4-broadwayd :5
```

The server expects the colon-prefixed display number as a commandline argument.

Then point your web browser at `http://127.0.0.1:8085`.

Once the Broadway server is running, you can start your applications like
this:

```
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 bobgui4-demo
```

Multiple applications can be presented in the same web browser window.

## Broadway-specific environment variables

### `BROADWAY_DISPLAY`

Specifies the Broadway display number. The default display is 0.

The display number determines the port to use when connecting to a Broadway
application via the following formula:

```
port = 8080 + display
```
