vcpkg install --triplet x64-windows-static
meson setup build --cmake-prefix-path="$PWD/vcpkg_installed/x64-windows-static"
