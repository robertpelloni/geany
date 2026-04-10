print_backend_cflags += [
  '-DBOBGUI_PRINT_BACKEND_ENABLE_UNSUPPORTED',
]

# Use cpdb backend if present and enabled.
# If not, use cups if present.

if get_option('print-cpdb').enabled() and cpdb_dep.found()
  print_backends += 'cpdb'
  print_backend_sources += files([
    'bobguiprintbackendcpdb.c',
    'bobguiprintercpdb.c',
    'bobguiprintbackendutils.c',
  ])
  print_backend_deps += [cpdb_dep]
  cdata.set('HAVE_PRINTBACKEND_CPDB', 1)
elif get_option('print-cups').enabled() and cups_dep.found()
  print_backends += 'cups'
  print_backend_sources += files([
    'bobguiprintbackendcups.c',
    'bobguiprintercups.c',
    'bobguicupsutils.c',
    'bobguicupssecretsutils.c',
    'bobguiprintbackendutils.c',
  ])
  print_backend_deps += [cups_dep, colord_dep]
  cdata.set('HAVE_PRINTBACKEND_CUPS', 1)
endif

# The 'file' print backend cannot be disabled
print_backends += 'file'
print_backend_sources += files(['bobguiprintbackendfile.c'])

cdata.set_quoted('BOBGUI_PRINT_BACKENDS', ','.join(print_backends))
