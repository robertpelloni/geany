compose_parse = executable('compose-parse',
  sources: 'compose-parse.c',
  c_args: bobgui_cargs + common_cflags,
  include_directories: [confinc, bobguiinc],
  dependencies: libbobgui_static_dep,
  install: false,
)
