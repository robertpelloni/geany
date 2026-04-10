# Action Section Model

## Summary
This pass extends the shared action system with an explicit section concept.

## Why this matters
Earlier action grouping relied mostly on category metadata. That worked, but categories are not always enough to describe how actions should be presented in menus, palettes, and toolbars.

A stronger framework model needs an explicit presentation grouping layer.

## What changed
The action registry now supports a section-aware add path:
- `bobgui_action_registry_add_sectioned()`

The action visit model also now carries a `section` field in addition to:
- category
- shortcut
- icon name
- checkable
- checked

## Practical effect
This means action presentation can now distinguish between:
- semantic category
- visual section/grouping

That gives bobgui more room to evolve toward richer menu, toolbar, and command-palette composition without overloading one field with too many responsibilities.

## Direction
This is another step toward making bobgui feel like a coherent application framework:
- one action model
- multiple UI surfaces
- explicit metadata for presentation decisions
