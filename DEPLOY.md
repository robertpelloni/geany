# Deployment Instructions

*This file will contain the latest detailed deployment and build instructions.*

## Current Geany (C/GTK)
Standard Autotools or Meson build.
```bash
./autogen.sh
./configure
make
sudo make install
```
Or via meson:
```bash
meson setup build
ninja -C build
```

## Upcoming Architectures
- **C++ Refactor**: Instructions will be updated as the build system is modified to support modern C++.
- **Go Port (`geany-go`)**: `go build` / `go run`.
- **Submodule UIs**: Specific build instructions for Qt6, Qt4, and custom GTK builds will be added here once integrated.

## Native UI Submodule Build Instructions

### bobgui (GTK)
```bash
cd subprojects/bobgui
./autogen.sh
make -j$(nproc)
```

### bobui (Qt6)
```bash
cd subprojects/bobui
cmake -S . -B build -G "Ninja"
cmake --build build
```

### btk (Qt4/CopperSpice)
```bash
cd subprojects/btk
cmake -S . -B build -G "Ninja"
cmake --build build
```
