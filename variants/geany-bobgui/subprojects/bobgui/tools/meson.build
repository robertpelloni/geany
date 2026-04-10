# Installed tools

extra_update_icon_cache_objs = []
if win32_enabled
  gen_uac_manifest = find_program('generate-uac-manifest.py')

  uac_exe_pkg = 'bobgui4'
  uac_exe_name = '@0@-update-icon-cache'.format(uac_exe_pkg)

  # Well, we have to forgo the xxx.exe.manifest in the output listing, since
  # compile_resources doesn't like to consume targets with multiple outputs,
  # and the xxx.exe.manifest and xxx.rc are tied together
  uac_rc = custom_target(
    '@0@-rc'.format(uac_exe_name),
    output: ['@0@.rc'.format(uac_exe_name)],
    command: [gen_uac_manifest,
              '-p=@0@'.format(uac_exe_pkg),
              '-n=@0@'.format(uac_exe_name),
              '--pkg-version=@0@'.format(meson.project_version()),
              '--output-dir=@OUTDIR@'],
  )
  extra_update_icon_cache_objs = import('windows').compile_resources(uac_rc)
endif

bobgui_tools = [
  ['bobgui4-path-tool', ['bobgui-path-tool.c',
                      'bobgui-path-tool-decompose.c',
                      'bobgui-path-tool-info.c',
                      'bobgui-path-tool-render.c',
                      'bobgui-path-tool-restrict.c',
                      'bobgui-path-tool-reverse.c',
                      'bobgui-path-tool-show.c',
                      'bobgui-path-tool-utils.c',
                      'path-view.c'], [libbobgui_dep]],
  ['bobgui4-query-settings', ['bobgui-query-settings.c'], [libbobgui_dep]],
  ['bobgui4-builder-tool', ['bobgui-builder-tool.c',
                         'bobgui-builder-tool-simplify.c',
                         'bobgui-builder-tool-validate.c',
                         'bobgui-builder-tool-enumerate.c',
                         'bobgui-builder-tool-screenshot.c',
                         'bobgui-builder-tool-preview.c',
                         'fake-scope.c'], [libbobgui_dep] ],
  ['bobgui4-rendernode-tool', ['bobgui-rendernode-tool.c',
                        'bobgui-rendernode-filter-copypaste.c',
                        'bobgui-rendernode-filter-strip.c',
                        'bobgui-rendernode-filter-texture.c',
                        'bobgui-rendernode-tool-benchmark.c',
                        'bobgui-rendernode-tool-compare.c',
                        'bobgui-rendernode-tool-convert.c',
                        'bobgui-rendernode-tool-extract.c',
                        'bobgui-rendernode-tool-filter.c',
                        'bobgui-rendernode-tool-info.c',
                        'bobgui-rendernode-tool-match.c',
                        'bobgui-rendernode-tool-render.c',
                        'bobgui-rendernode-tool-show.c',
                       'bobgui-tool-utils.c',
                        '../testsuite/reftests/reftest-compare.c'], [libbobgui_dep] ],
  ['bobgui4-image-tool', ['bobgui-image-tool.c',
                       'bobgui-image-tool-info.c',
                       'bobgui-image-tool-compare.c',
                       'bobgui-image-tool-convert.c',
                       'bobgui-image-tool-play.c',
                       'bobgui-image-tool-relabel.c',
                       'bobgui-image-tool-show.c',
                       'bobgui-tool-utils.c',
                        '../testsuite/reftests/reftest-compare.c'], [libbobgui_dep] ],
  ['bobgui4-update-icon-cache', ['updateiconcache.c', '../bobgui/bobguiiconcachevalidator.c' ] + extra_update_icon_cache_objs, [ libbobgui_dep ] ],
]

if os_unix
  bobgui_tools += [['bobgui4-launch', ['bobgui-launch.c'], [libbobgui_dep]]]
endif

foreach tool: bobgui_tools
  tool_name = tool.get(0)
  tool_srcs = tool.get(1)
  tool_deps = tool.get(2)

  exe = executable(tool_name,
    sources: tool_srcs,
    include_directories: [confinc],
    c_args: common_cflags + [ '-DBUILD_TOOLS' ],
    dependencies: tool_deps,
    install: true,
  )

  set_variable(tool_name.underscorify(), exe) # used in testsuites
  meson.override_find_program(tool_name, exe)
endforeach

# Data to install
install_data('bobgui4builder.rng', install_dir: bobgui_datadir / 'bobgui-4.0')

install_data([
    'bobgui4builder.loc',
    'bobgui4builder.its',
  ],
  install_dir: bobgui_datadir / 'gettext/its',
)

if bash.found()
  install_data([
      'completions/bobgui4-builder-tool',
      'completions/bobgui4-image-tool',
      'completions/bobgui4-path-tool',
      'completions/bobgui4-rendernode-tool',
    ],
    install_dir: bash_comp_inst_dir,
    install_tag: 'bin',
  )
endif
