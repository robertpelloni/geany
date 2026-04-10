bobgui_print_headers = files([
  'bobguipagesetup.h',
  'bobguipapersize.h',
  'bobguiprintcontext.h',
  'bobguiprintoperation.h',
  'bobguiprintoperationpreview.h',
  'bobguiprintsettings.h',
])

bobgui_print_sources = files([
  'bobguipagesetup.c',
  'bobguipapersize.c',
  'bobguiprintcontext.c',
  'bobguiprintoperation.c',
  'bobguiprintoperationpreview.c',
  'bobguiprintsettings.c',
  'bobguiprintutils.c',
])

print_backends = []
print_backend_sources = []
print_backend_deps = []
print_backend_cflags = []

bobgui_unix_print_headers = []
bobgui_unix_print_sources = []

if os_unix
  subdir('backends')

  bobgui_unix_print_headers += files([
    'bobguipagesetupunixdialog.h',
    'bobguiprintunixdialog.h',
    'bobguiprinter.h',
    'bobguiprintjob.h',
  ])

  bobgui_unix_print_sources += files([
    'bobguicustompaperunixdialog.c',
    'bobguipagesetupunixdialog.c',
    'bobguipagethumbnail.c',
    'bobguiprintbackend.c',
    'bobguiprinter.c',
    'bobguiprinteroption.c',
    'bobguiprinteroptionset.c',
    'bobguiprinteroptionwidget.c',
    'bobguiprintjob.c',
    'bobguiprintoperation-unix.c',
    'bobguiprintoperation-portal.c',
    'bobguiprintunixdialog.c',
  ])
endif

bobgui_win32_print_headers = []
bobgui_win32_print_sources = []

if os_win32
  bobgui_win32_print_sources += files([
    'bobguiprint-win32.c',
    'bobguiprintoperation-win32.c',
  ])

  if cc.has_header_symbol('windows.h', 'IPrintDialogCallback')
    cdata.set('HAVE_IPRINTDIALOGCALLBACK', 1)
  endif
endif
