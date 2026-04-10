# App Framework Ergonomics Progress

## Summary
This pass continued the shift from "bobgui as a large widget toolkit" toward "bobgui as a coherent application framework".

The workbench and command palette are especially important because they target one of the biggest practical reasons developers prefer Qt in many desktop scenarios:
- easier application shell composition
- command-driven workflows
- integrated discoverability
- less manual widget plumbing

## What improved in this pass
### Command palette interaction quality
The command palette now has:
- dynamic query-driven rebuilds
- ranked results
- pinned-command prioritization
- recent-command prioritization
- automatic selection of the best match
- keyboard navigation from the search entry
- Enter-based activation of the selected command
- public APIs to mark usage and clear history

### Why this matters
This is a concrete ergonomics improvement, not just another abstract subsystem:
- application developers can now define commands once and expose them through a modern launcher
- users get a stronger keyboard-first interaction model
- workbench-based apps feel more like professional desktop software and less like improvised widget layouts

## Strategic interpretation
If bobgui is going to become "more like Qt but not Qt", then these are exactly the kinds of layers it needs:
1. workbench shell
2. command palette
3. action registry
4. dock/workspace integration
5. polished demo application patterns

## Recommended next step
The next most valuable step would be a shared action registry so that:
- workbench header actions
- command palette entries
- future menus/toolbars
- future dock commands

all derive from a single application action model.
