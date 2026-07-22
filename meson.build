project('lanscape', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17', 'b_vscrt=static_from_buildtype'])

add_project_arguments('-DZMQ_BUILD_DRAFT_API', language: 'cpp')

if build_machine.system() == 'windows'
  libzmq_dep = dependency('ZeroMQ', modules: ['libzmq-static'], method: 'cmake')
  cppzmq_dep = dependency('cppzmq', method: 'cmake')
  json_dep = dependency('nlohmann_json', method: 'cmake')
else
  libzmq_dep = dependency('libzmq')
  cppzmq_dep = dependency('cppzmq')
  json_dep = dependency('nlohmann_json')
endif

subdir('src')
subdir('registry')
