call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
meson setup build/geany-bobgui variants/geany-bobgui
meson compile -C build/geany-bobgui
