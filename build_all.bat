call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
cmake -S subprojects/bobui -B build/bobui-lib -DCMAKE_INSTALL_PREFIX=%CD%\build/bobui-install
cmake --build build/bobui-lib --target install
cmake -S subprojects/btk -B build/btk-lib -DCMAKE_INSTALL_PREFIX=%CD%\build/btk-install
cmake --build build/btk-lib --target install
meson setup build/bobgui-lib subprojects/bobgui --prefix=%CD%\build/bobgui-install
meson install -C build/bobgui-lib
set PKG_CONFIG_PATH=%CD%\build\bobgui-install\lib\pkgconfig;%PKG_CONFIG_PATH%
meson setup build/geany-bobgui variants/geany-bobgui
meson compile -C build/geany-bobgui
cmake -S variants/geany-bobui -B build/geany-bobui -DQt6_DIR=%CD%\build/bobui-install/lib/cmake/Qt6
cmake --build build/geany-bobui
cmake -S variants/geany-btk -B build/geany-btk -DBTK_DIR=%CD%\build/btk-install/lib/cmake/BTK
cmake --build build/geany-btk
