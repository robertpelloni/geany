
# Build the Go shared library using a custom target
# This ensures it runs 'go build' whenever meson runs the build step.
geanygo_lib = custom_target('libgeanygo',
  output: ['libgeanygo.so', 'libgeanygo.h'],
  command: [find_program('go'), 'build', '-buildmode=c-shared', '-o', '@OUTPUT0@', meson.current_source_dir() + '/main.go'],
  build_by_default: true,
  install: true,
  install_dir: get_option('libdir')
)

# Declare a dependency so the main C++ executable can link against it
geanygo_dep = declare_dependency(
  link_args: ['-L' + meson.current_build_dir(), '-lgeanygo'],
  sources: geanygo_lib
)
