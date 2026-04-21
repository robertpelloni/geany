# Project Submodules

This document tracks the external repositories utilized by this project, specifically focusing on the native UI frontends.

## 1. Bobui (Qt6)
- **URL**: [https://github.com/robertpelloni/bobui](https://github.com/robertpelloni/bobui)
- **Location**: `submodules/bobui`
- **Purpose**: Provides the Qt6 (bobq) native UI frontend components.
- **Integration**: To be heavily integrated into the C++ refactor and Go port as the premier modern Qt interface.

## 2. Btk (CopperSpice/Qt4)
- **URL**: [https://github.com/robertpelloni/btk](https://github.com/robertpelloni/btk)
- **Location**: `submodules/btk`
- **Purpose**: Provides the CopperSpice/Qt4 native UI frontend components.
- **Integration**: Ensures legacy Qt compatibility and alternative rendering paths.

## 3. Bobgui (GTK)
- **URL**: [https://github.com/robertpelloni/bobgui](https://github.com/robertpelloni/bobgui)
- **Location**: `submodules/bobgui`
- **Purpose**: Provides the GTK (bobtk) native UI frontend components.
- **Integration**: Serves as the evolution of Geany's current GTK interface, modernizing it through the bobgui framework.
