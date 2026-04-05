# Geany Experimental Toolkit Variants

This directory contains isolated experimental frontends for multiple alternate UI toolsets.

## Variants

- `geany-btk/`
  - BTK / CopperSpice-style variant
  - currently the most validated path in this environment
- `geany-bobui/`
  - BobUI / Qt6-fork variant
  - uses Qt6-compatible package discovery
- `geany-bobgui/`
  - BobGUI / GTK-fork variant
  - uses Meson + `dependency('bobgui4')`

## Intent

Each variant is meant to stay toolkit-exclusive inside its own folder so Geany can evolve parallel frontend experiments without forcing a single migration path across the whole production tree.

## Current environment status

- BTK variant has active Windows/MSVC build, runtime staging, and packaging validation in this repo
- BobUI variant scaffolding is present, but local build validation still depends on a usable BobUI / Qt6 package tree
- BobGUI variant scaffolding is present, but local build validation still depends on Meson / pkg-config / BobGUI dev dependencies that are not available in this environment
