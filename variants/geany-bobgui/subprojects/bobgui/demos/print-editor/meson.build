executable('bobgui4-print-editor',
  sources: ['print-editor.c'],
  c_args: common_cflags,
  dependencies: [ libbobgui_dep, gmodule_headers_dep, profile_conf_h ],
  include_directories: confinc,
  win_subsystem: 'windows',
  link_args: extra_demo_ldflags,
  install: true,
)

# desktop file
install_data('org.bobgui.PrintEditor4.desktop', install_dir: bobgui_applicationsdir)

# appdata
install_data('org.bobgui.PrintEditor4.appdata.xml', install_dir: bobgui_appdatadir)

# icons
icontheme_dir = join_paths(bobgui_datadir, 'icons/hicolor')

foreach size: ['scalable', 'symbolic']
  install_subdir('data/' + size, install_dir: icontheme_dir)
endforeach

if bash.found()
  install_data([ 'completion' ],
    rename: [ 'bobgui4-print-editor' ],
    install_dir: bash_comp_inst_dir,
    install_tag: 'bin',
  )
endif
