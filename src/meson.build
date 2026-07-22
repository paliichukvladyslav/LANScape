common_sources = files(
  'configuration.cpp'
)

proj_inc = include_directories('include')
deps = [libzmq_dep, cppzmq_dep, json_dep]

executable('lanscape-server',
  ['server.cpp', common_sources],
  dependencies : deps,
  include_directories : proj_inc
)

executable('lanscape-client',
  ['client.cpp', common_sources],
  dependencies : deps,
  include_directories : proj_inc
)

if host_machine.system() == 'windows'
    shared_library('lanscape_shellext',
        'shellext.cpp',
        name_prefix: '',
        vs_module_defs: 'shellext.def',
        dependencies: deps,
        include_directories: proj_inc,
        link_args: ['-lole32', '-luuid', '-lshlwapi']
    )
endif
