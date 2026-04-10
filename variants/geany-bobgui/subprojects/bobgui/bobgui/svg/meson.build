bobgui_svg_public_sources = files([
  'bobguisvg.c',
  'bobguisvgclip.c',
  'bobguisvgcolor.c',
  'bobguisvgcolorstop.c',
  'bobguisvgcontentfit.c',
  'bobguisvgdasharray.c',
  'bobguisvgelement.c',
  'bobguisvgelementtype.c',
  'bobguisvgenum.c',
  'bobguisvgerror.c',
  'bobguisvgfilter.c',
  'bobguisvgfilterfunctions.c',
  'bobguisvgfilterref.c',
  'bobguisvgfiltertype.c',
  'bobguisvghref.c',
  'bobguisvgkeyword.c',
  'bobguisvglanguage.c',
  'bobguisvglocation.c',
  'bobguisvgmask.c',
  'bobguisvgnumber.c',
  'bobguisvgnumbers.c',
  'bobguisvgorient.c',
  'bobguisvgpaint.c',
  'bobguisvgpangoutils.c',
  'bobguisvgpath.c',
  'bobguisvgpathdata.c',
  'bobguisvgpathutils.c',
  'bobguisvgproperty.c',
  'bobguisvgstring.c',
  'bobguisvgstringlist.c',
  'bobguisvgstringutils.c',
  'bobguisvgtextdecoration.c',
  'bobguisvgtransform.c',
  'bobguisvgunit.c',
  'bobguisvgutils.c',
  'bobguisvgvalue.c',
  'bobguisvgviewbox.c',
  'bobguisvganimation.c',
])

bobgui_svg_public_headers = files([
  'bobguisvg.h',
])

install_headers(bobgui_svg_public_headers, 'bobguisvg.h', subdir: 'bobgui-4.0/bobgui/svg')

bobgui_svg_deps = [
  libm,
  glib_dep,
  gobject_dep,
  platform_gio_dep,
  cairo_dep,
  pango_dep,
  pangoft_dep,
  pixbuf_dep,
  libgsk_dep,
]

bobgui_svg_enums = gnome.mkenums('bobguisvgenumtypes',
  sources: bobgui_svg_public_headers,
  c_template: 'bobguisvgenumtypes.c.template',
  h_template: 'bobguisvgenumtypes.h.template',
  install_dir: join_paths(bobgui_includedir, 'bobgui-4.0/bobgui/svg'),
  install_header: true,
)

bobgui_svg_enum_h = bobgui_svg_enums[1]

libbobgui_svg = static_library('bobgui_svg',
  override_options: [ 'unity=on', 'unity_size=1000' ],
  sources: [
    bobgui_svg_public_sources,
    bobguitypebuiltins_h,
    bobgui_svg_enums,
    gdkversionmacros_h,
    gdk_visibility_h,
  ],
  dependencies: bobgui_svg_deps,
  include_directories: [ confinc, gdkinc, gskinc, bobguiinc ],
  c_args: [
    '-DBOBGUI_COMPILATION',
    '-DG_LOG_DOMAIN="Bobgui"',
  ] + common_cflags,
)

# We don't have link_with: to internal static libs here on purpose, just
# list the dependencies and generated headers and such, for use in the
# "public" libbobgui_dep used by internal executables.
libbobgui_svg_dep = declare_dependency(include_directories: [ confinc, ],
  sources: [ bobgui_svg_enum_h ],
  dependencies: bobgui_svg_deps,
)
