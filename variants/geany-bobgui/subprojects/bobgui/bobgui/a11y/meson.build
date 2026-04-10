bobgui_a11y_src = []
bobgui_a11y_backends = []
bobgui_a11y_backend_info = {}

if os_unix
  bobgui_a11y_backends += 'atspi'
endif

if bobgui_a11y_backends.contains('atspi')
  bobgui_a11y_backend_info += { 'atspi': { 'description': 'BOBGUI AT-SPI accessibility support' } }

  subdir('atspi')

  bobgui_a11y_src += files([
    'bobguiatspiaction.c',
    'bobguiatspicache.c',
    'bobguiatspicomponent.c',
    'bobguiatspicontext.c',
    'bobguiatspieditabletext.c',
    'bobguiatspipango.c',
    'bobguiatspiroot.c',
    'bobguiatspiselection.c',
    'bobguiatspisocket.c',
    'bobguiatspitextbuffer.c',
    'bobguiatspitext.c',
    'bobguiatspiutils.c',
    'bobguiatspivalue.c',
    'bobguiatspihypertext.c',
    'bobguiatspihyperlink.c',
  ])

  bobgui_atspi_public_headers = files([
    'bobguiatspisocket.h',
  ])

  install_headers(bobgui_atspi_public_headers, 'bobguiatspi.h', subdir: 'bobgui-4.0/bobgui/a11y/')
endif

if accesskit_dep.found()
  bobgui_a11y_backends += 'accesskit'
endif

if bobgui_a11y_backends.contains('accesskit')
  bobgui_a11y_backend_info += { 'accesskit': { 'description': 'BOBGUI AccessKit accessibility support' } }

  bobgui_a11y_src += files([
    'bobguiaccesskitcontext.c',
    'bobguiaccesskitroot.c',
  ])
endif
