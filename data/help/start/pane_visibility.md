# Hiding and Rearranging Panes
In both the [Editor]() and [Debugger]() modes, you can control which parts of the interface are visible and how they are arranged.

## Showing and Hiding Panes
A row of checkboxes at the bottom of the window lists all available panes for the current mode. Selecting a checkbox shows that pane; clearing it hides the pane.
When you switch modes, the list updates to match the UI components available for that mode.
For example, the memory dump is visible by default available in Debugger mode but not in Editor mode.
[That evil img]()

## Rearranging the Layout
You can rearrange panes by clicking and dragging their title bars.
This allows you to dock panes to different positions within the project workspace.
Grey areas between panes are splitters, which can be dragged to resize panes.

Layout changes apply per project and are saved until you adjust them again.
[Layout fun pic]()

## Known Issues
* **Rearrangement limited to leftmost project**: Currently, UI rearrangement is only available for the leftmost project tab. Closing the leftmost project enables rearrangement in other tabs.
* **Pane resizing on mode switch**: Switching between modes may cause pane sizes to shift gradually over time.
These issues are known and being addressed in a future update.
