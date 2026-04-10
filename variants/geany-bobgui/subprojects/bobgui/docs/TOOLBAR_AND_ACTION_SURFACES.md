# Toolbar and Action Surfaces

## Summary
This pass continues the transition from a widget toolkit toward a more coherent application framework by expanding how shared actions are projected into the UI.

## What exists now
The action model can now drive multiple surfaces:
- command palette
- workbench header actions
- generated menubar
- generated toolbar buttons

This pass also pushes two improvements:
- command palette grouping via section rows inferred from action metadata
- toolbar grouping via category labels inferred from action metadata

## Why this matters
A framework feels "Qt-like" not because it has the same class names, but because a single action can appear in multiple places consistently.

That consistency is what reduces boilerplate and makes application structure feel intentional.

## Current design direction
`BobguiActionRegistry` acts as the source of truth.
From there, actions can be exposed through:
- palette search
- header buttons
- menubar sections
- toolbar buttons

## Remaining gap
Toolbars are currently generated in a simple way and should evolve toward:
- full icon-aware rendering beyond basic icon-name forwarding
- stronger toggle/stateful visual treatment
- overflow behavior
- stronger visual separation than plain labels

## Recommended next step
1. add icons and shortcuts as richer toolbar metadata
2. support toggle actions and checked state
3. wire dock/workspace commands into the same action model
4. keep refining the demo shell into a canonical bobgui app structure
