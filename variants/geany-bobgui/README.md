# Geany BobGUI Variant

This directory hosts an **experimental BobGUI-based alternate frontend** for Geany.

## Purpose

- explore a BobGUI / GTK-fork frontend path without destabilizing the production tree
- keep this variant toolkit-exclusive to BobGUI / GObject-style APIs
- provide a separately buildable BobGUI shell while deeper toolkit migration work continues elsewhere

## Current scope

This variant currently provides a BobGUI-only Search Studio shell with pages for:

- Find
- Replace
- Find in Files
- Mark
- a lower activity-style text surface

The current focus is toolkit isolation and build wiring. This variant is intentionally minimal compared with the more evolved alternate frontend prototypes.

## Toolchain direction

The variant is expected to build against the BobGUI submodule at:
- `../../subprojects/bobgui`

It currently uses a standalone Meson file with:
- `dependency('bobgui4')`
- `dependency('gio-2.0')`

So a usable BobGUI development environment still needs:
- Meson
- Ninja
- BobGUI development packages / pkg-config metadata

## Current validation status

- variant scaffolding and BobGUI-exclusive sources were added in this repo
- full build validation in this environment remains blocked by the same missing Meson / pkg-config plumbing that also blocks the main GTK-oriented tree

## Next steps

1. validate the BobGUI shell in a Meson-capable BobGUI dev environment
2. evolve the shell toward a richer Search Studio workbench
3. decide how much of the GTK-facing Geany core can be shared versus re-wrapped for a BobGUI-exclusive frontend
