env = environment()
env.prepend('GI_TYPELIB_PATH',
  project_build_root / 'bobgui',
)

if host_machine.system() == 'windows'
  env.prepend('PATH', project_build_root / 'bobgui')
endif

test('api',
  find_program('api.py', dirs: meson.current_source_dir()),
  suite: ['introspection'],
  depends: [ bobgui_gir, libbobgui ],
  env: env,
)
