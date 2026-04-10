# Action Metadata Evolution

## Summary
This pass extends the shared action model further so bobgui can move from simple command callbacks toward a richer application action framework.

## New capabilities
### Toggle actions
The action registry now supports explicit toggle-style actions via:
- `bobgui_action_registry_add_toggle()`
- `bobgui_action_registry_set_checked()`
- `bobgui_action_registry_get_checked()`

### Icon-aware metadata
The action model also now carries an icon-name slot so higher-level action surfaces can evolve toward richer rendering in:
- toolbars
- menus
- command launchers

This pass continues that direction by feeding icon metadata into command palette row rendering as well.

This allows the same action model to represent stateful commands like:
- sidebar visibility
- panel toggles
- view modes
- checked menu actions

### Richer metadata shape
The action model now carries additional state for action visitation/projection, including:
- category
- shortcut
- icon name slot
- checkable state
- checked state

This is important because a modern application framework needs one action model to drive many surfaces:
- palette
- menu
- toolbar
- header actions
- future dock/workspace controls

## Demo impact
The workbench demo now updates toggle checked state through the action registry instead of only changing status text, and it uses icon-aware command registration for representative actions.

## Why this matters
This is another step toward making bobgui feel like a coherent framework rather than a bag of widgets and callbacks.
