node_editor_sources = [
  'bobguirendererpaintable.c',
  'main.c',
  'node-editor-application.c',
  'node-editor-window.c',
]

node_editor_resources = gnome.compile_resources('node_editor_resources',
  'node-editor.gresource.xml',
  source_dir: meson.current_source_dir(),
)

executable('bobgui4-node-editor',
  sources: [node_editor_sources, node_editor_resources],
  dependencies: [ libbobgui_dep, gmodule_headers_dep, profile_conf_h ],
  include_directories: confinc,
  c_args: common_cflags,
  win_subsystem: 'windows',
  link_args: extra_demo_ldflags,
  install: true,
  kwargs: demo_kwargs
)

# icons
icontheme_dir = join_paths(bobgui_datadir, 'icons/hicolor')

foreach size: ['scalable', 'symbolic']
  install_subdir('data/' + size, install_dir: icontheme_dir)
endforeach

# desktop file
install_data('org.bobgui.bobgui4.NodeEditor.desktop', install_dir: bobgui_applicationsdir)

# appdata
configure_file(
  input: 'org.bobgui.bobgui4.NodeEditor.appdata.xml.in',
  output: 'org.bobgui.bobgui4.NodeEditor.appdata.xml',
  configuration: appdata_config,
  install_dir: bobgui_appdatadir
)

if bash.found()
  install_data([ 'completion' ],
    rename: [ 'bobgui4-node-editor' ],
    install_dir: bash_comp_inst_dir,
    install_tag: 'bin',
  )
endif
