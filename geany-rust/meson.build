# Build the Rust static/shared library using cargo
# This ensures it runs 'cargo build' whenever meson runs the build step.
geanyrust_lib = custom_target('libgeanyrust',
  output: ['libgeanyrust.a', 'libgeanyrust.so'],
  command: [find_program('cargo'), 'build', '--manifest-path', meson.current_source_dir() + '/Cargo.toml', '--release', '--target-dir', meson.current_build_dir() + '/target'],
  build_by_default: true,
  install: true,
  install_dir: get_option('libdir')
)

# Declare a dependency so the main C++ executable can link against it
geanyrust_dep = declare_dependency(
  link_args: ['-L' + meson.current_build_dir() + '/target/release', '-lgeanyrust'],
  sources: geanyrust_lib
)
