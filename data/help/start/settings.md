# Changing the Style
Pep allows you to customize the look and feel of the application, including fonts and colors.
These options are available in the **Preferences** dialog.
[evil picture here]()

## Themes
The application includes with both a default **Light** theme and a default **Dark** theme.
You can switch between them with the **Current Palette** combo box, or you can create and use a custom theme.

## Fonts & Colors
The Fonts & Colors section controls the style for the entire application, including the editor, debugger, and circuit views. 
The default themes cannot be modified directly, but you can create an editable copy of any existing theme.

Once copied, you can:
* Modify **colors** for specific interface elements (e.g., buttons, highlights, text).
* Adjust font family and size for individual components.
* Apply font overrides (bold, italic, underline, strikethrough).
Changes apply immediately, but require user input (e.g., **Save**) to be persisted.

## Managing Palettes
* **Copy** — Duplicate the current palette for customization.
* **Save** — Write back changes to your currently selected palette.
* **Rename** — Change the palette name.
* **Import / Export** — Save your palette to a file or load one from disk.
These options make it easy to share color schemes or transfer them between systems.

## Restoring Defaults
If you make changes that make the UI difficult or impossible to read, you can restore the default fonts and colors in two ways:
* Use the **Help → Restore Fonts & Colors** menu item.
* Launch Pep with the reset command-line flag:
```bash
pep --reset-style
```
