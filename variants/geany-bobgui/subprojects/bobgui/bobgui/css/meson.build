bobgui_css_public_sources = files([
  'bobguicsserror.c',
  'bobguicsssection.c',
])

bobgui_css_private_sources = files([
  'bobguicssdataurl.c',
  'bobguicssparser.c',
  'bobguicssserializer.c',
  'bobguicsstokenizer.c',
  'bobguicssvariablevalue.c',
])

bobgui_css_public_headers = files([
  'bobguicssenums.h',
  'bobguicsserror.h',
  'bobguicsslocation.h',
  'bobguicsssection.h',
])

install_headers(bobgui_css_public_headers, 'bobguicss.h', subdir: 'bobgui-4.0/bobgui/css')

bobgui_css_deps = [
  libm,
  glib_dep,
  gobject_dep,
  platform_gio_dep,
]

bobgui_css_enums = gnome.mkenums('bobguicssenumtypes',
  sources: bobgui_css_public_headers,
  c_template: 'bobguicssenumtypes.c.template',
  h_template: 'bobguicssenumtypes.h.template',
  install_dir: join_paths(bobgui_includedir, 'bobgui-4.0/bobgui/css'),
  install_header: true,
)

bobgui_css_enum_h = bobgui_css_enums[1]

libbobgui_css = static_library('bobgui_css',
  sources: [
    bobgui_css_public_sources,
    bobgui_css_private_sources,
    bobgui_css_enums,
    gdkversionmacros_h,
    gdk_visibility_h,
  ],
  dependencies: bobgui_css_deps,
  include_directories: [ confinc, ],
  c_args: [
    '-DBOBGUI_COMPILATION',
    '-DG_LOG_DOMAIN="Bobgui"',
  ] + common_cflags,
)

# We don't have link_with: to internal static libs here on purpose, just
# list the dependencies and generated headers and such, for use in the
# "public" libbobgui_dep used by internal executables.
libbobgui_css_dep = declare_dependency(include_directories: [ confinc, ],
  sources: [ bobgui_css_enum_h ],
  dependencies: bobgui_css_deps,
)
