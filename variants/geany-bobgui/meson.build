project('geany-bobgui-variant', 'c',
  version: '0.1.0',
  default_options: ['c_std=c11'])

bobgui_proj = subproject('bobgui')
bobgui_dep = bobgui_proj.get_variable('libbobgui_dep')
gio_dep = dependency('gio-2.0')

gui_variant = executable(
  'geany-bobgui-search-studio',
  'src/main.c',
  dependencies: [bobgui_dep, gio_dep],
  install: false,
)
