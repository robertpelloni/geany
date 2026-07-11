# Memory & Observations

- **User Preferences**: The user is extremely enthusiastic ("Insanely Great!!!") and demands comprehensive, exhaustive detail in documentation, analysis, and execution.
- **Submodules**:
  - `https://github.com/robertpelloni/bobui` (qt6)
  - `https://github.com/robertpelloni/btk` (qt4)
  - `https://github.com/robertpelloni/bobgui` (gtk)
- **Target Architecture**: We are maintaining a C++ core alongside a massive Go port. Multiple UI frontends will be supported.
- **Parity**: Notepad++ is the benchmark for feature parity. We must exceed it.
- **Documentation**: Always comment code in depth. Always document findings in extreme detail. Maintain changelog, roadmap, and vision. Update version numbers consistently.
- **Go Port Architecture (`geany-go`)**: The backend must isolate text manipulation from UI code. File I/O, Plugin Management, Configurations, and Project states are now tracked by idiomatic Go registries holding mutexes for thread safety. Submodules (bobui, btk, bobgui) act merely as rendering surfaces implementing the `ui.Application` interfaces.
- **Submodule State**: As of Phase 1 conclusion, the pointers for `bobui`, `btk`, and `bobgui` have been successfully pinned, recursively updated, and validated via `git submodule update`. Their initialization into the build graphs (e.g. Qt6 inside `meson.build`) confirms they are in a ready state.

## LLM Session Architecture Observations (Rust & Go Extension)
- The legacy \`origin/master\` C files contain compilation errors out of the box due to partially completed C-to-C++ refactoring steps (e.g. \`src/build.c\` referencing undeclared \`cmdindex\`, \`src/document.c\` missing \`enc_idx\`, and missing function arguments to \`configuration_save()\`).
- Attempted to revert C/C++ files to master to safely inject the Rust/Go submodules without dealing with the broken C build chain.
- Successfully built \`libgeanyrust.so\` via Cargo and hooked it up to Meson.
- Successfully built Go \`textfx\` logic and wired it up via CGO. Both languages export clean C-compatible symbol tables to \`Application.cpp\`.
