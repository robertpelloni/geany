call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
meson setup build --reconfigure --force-fallback-for glib-2.0,gmodule-2.0,gtk+-3.0
meson compile -C build
