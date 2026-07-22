raw_dll_path = meson.project_build_root() / 'src' / 'lanscape_shellext.dll'

escaped_dll_path = raw_dll_path.replace('/', '\\\\').replace('\\', '\\\\')

conf_data = configuration_data()
conf_data.set('DLL_PATH', escaped_dll_path)

configure_file(
  input: 'install.reg.in',
  output: 'install.reg',
  configuration: conf_data
)

configure_file(
  input: 'uninstall.reg.in',
  output: 'uninstall.reg',
  configuration: conf_data
)

configure_file(
  input: 'sidebar_install.reg.in',
  output: 'sidebar_install.reg',
  configuration: conf_data
)

configure_file(
  input: 'sidebar_uninstall.reg.in',
  output: 'sidebar_uninstall.reg',
  configuration: conf_data
)
