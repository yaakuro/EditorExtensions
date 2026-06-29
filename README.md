# EditorExtensions Gem

An O3DE editor gem that adds quality-of-life tools to the Editor. It is an **editor-only** gem. The features below are host-tools code and do not ship in game runtime builds.

## Features

### Viewport Screenshot / Snapshot

Capture the current 3D viewport to a PNG on disk, at a resolution independent of the on-screen viewport size.

- **Toolbar button** in the viewport's top toolbar, with a dropdown submenu (also available under the viewport **Options** menu).
- **Hotkey:** `Ctrl+Shift+S`.
- **Resolution presets** in the submenu — 1280×720, 1920×1080, 2560×1440, 3840×2160, 1920×1200, 2560×1600, 3440×1440 plus **Viewport Resolution** (match the current viewport) and a **Custom…** dialog (64–8192 px per side). The selection is remembered for subsequent captures.
- **Antialiasing (SMAA)** toggle.
- Rendering goes through a temporary offscreen render-to-texture pipeline, so captures are not limited to the on-screen viewport's aspect or size. The projection is aspect-corrected and the source view's post-process settings are mirrored. Editor overlays (axis gizmo, debug text) are excluded, TAA is disabled for the one-shot pass, and the PNG's alpha channel is stripped so the image is visible in ordinary viewers.
- Output: `<project>/user/Screenshots/Viewport_<timestamp>_<W>x<H>_<n>.png`, with a sequence number so captures taken within the same second don't overwrite each other. The result is reported in the Editor status bar and the console.

Implemented in `Source/Tools/ViewportScreenshot*` and `ScreenshotCustomResolutionDialog.*`.

### Component Config Presets

Save and load a single component's configuration as a reusable preset file.

- With **exactly one component selected** in the **Entity Inspector**, use **Save Config Preset…** / **Load Config Preset…** from the property editor's action list.
- Presets serialize to `<project>/Presets/*.componentpreset` via the SerializeContext.
- Loading validates that the preset was saved from the **same component type**, so a preset can't be applied to an incompatible component. After loading, the component is cycled off and on so its internal state re-initializes from the restored values.

Implemented in `Source/Tools/ComponentPresetHelper.*`, wired in `EditorExtensionsEditorSystemComponent`.

### C++ Class Generator

A wizard for generating C++ class scaffolding from a template.

- Available under **Tools → Examples** as **C++ Generator**, and also placed on the Tools toolbar.
- Choose a destination, with toggles for paths relative to the engine or relative to the project, then generate the class.

Implemented in `Source/Tools/CppGeneratorWidget.*`.

### Editor Extensions panel

A placeholder dockable tool pane under **Tools → Examples → Editor Extensions**. It currently shows intro/help text and is intended as a starting point for adding further tools.

Implemented in `Source/Tools/EditorExtensionsWidget.*`.

## Adding it to a project

Add the gem to your project's `project.json` dependencies:

```json
"EditorExtensions==1.0.0"
```

If the gem lives outside your project, register its path first (`/o3de register -gem-path <path> -enabled-gems <project>/enabled_gems.cmake` or via the Project Settings dialog), then re-run CMake.

## Building

Standard O3DE gem targets. The editor features build under `PAL_TRAIT_BUILD_HOST_TOOLS`:

```bash
cmake --build <build_dir> --target EditorExtensions.Editor --config profile
```

Editor unit tests (Linux/Windows/macOS, when tests are enabled for the gem):

```bash
cmake --build <build_dir> --target EditorExtensions.Editor.Tests --config profile
ctest --test-dir <build_dir> -C profile -R "Gem::EditorExtensions.Editor.Tests" --output-on-failure
```

The screenshot resolution/path policy and capture lifecycle are covered by `Tests/Tools/test_ViewportScreenshot.cpp`.

## Layout

```
Code/
  Include/EditorExtensions/     Public gem API (bus + type IDs)
  Source/Clients/               Runtime module + system component (no editor features)
  Source/Tools/                 All editor-only features, plus the .Editor module
  Tests/                        Unit tests
  Platform/<os>/                PAL traits and per-platform file lists
Registry/                       Asset Processor scan-folder settings
```

The runtime `EditorExtensions` module is effectively a shell; everything above lives in `EditorExtensions.Editor`.

## Notes and limitations

- **Unbuilt sources:** `Source/Tools/AttachmentPreviewManager.*` and `AttachmentPreviewPanel.*` implement an actor/attachment preview tool (spawn prefab attachments onto an actor bone, drive animation playback) but are **not listed in any CMake target**, so they do not compile. They are work in progress.
- The screenshot feature depends on Atom (`Atom_RPI.Public`, `Atom_Feature_Common.Public`, `Atom_Utils.Static`) and AzToolsFramework's Action/Menu/ToolBar Manager. The Action, Menu, and ToolBar Manager interfaces must be available before the gem's editor system component activates, or the registrations are skipped.
- Screenshot action/menu identifiers are namespaced `o3de.action.editorExtensions.*` and `o3de.menu.editorExtensions.*`, so they don't collide with same-named engine registrations.

## License

MIT. See `gem.json`.
