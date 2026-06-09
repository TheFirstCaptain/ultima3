# Modern Shell

`ModernShell` is the first AppKit-owned macOS shell scaffold for the Ultima III modernization effort.

AppKit owns lifecycle, menus, command routing, the primary window, and the game host view. SwiftUI is currently limited to a preferences panel boundary. The shell links the plain C `Ultima3Core` Swift package target to prove that modern shell code can call portable core code without exposing AppKit or SwiftUI types through `Core/`.

`GameHostView` renders a `u3_render_frame` through `AppKitRenderAdapter`. The default smoke frame is built from the native new-game current Sosaria map fixture derived from `Resources/English.lproj/MainResources.rsrc`; the shell falls back to the synthetic tile frame if bundled resources are unavailable. The portable core describes clear, rectangle, and tile commands with plain C data, while AppKit owns the concrete drawing.

Keyboard, mouse, and menu actions pass through `ShellInputAdapter`, which translates AppKit-owned events into the portable `u3_input_queue`. Arrow keys and keypad `8/2/4/6` enqueue the first bounded overworld smoke movement commands. The shell tick consumes queued input events, applies movement through `u3_overworld`, redraws the frame, and reports the consumed command. Macro command sequences use the same queue in legacy execution order.

`ShellAudioAdapter` is the first modern audio backend spike. It consumes portable `u3_audio_queue` events on tick and uses AVFoundation inside the shell to play existing sound assets for smoke testing. Missing or unsupported assets are reported in shell status text instead of changing the portable core contract.

`ShellLocationProvider` owns the first concrete resource-root and save-document path candidates for the AppKit shell. `ShellResourceAdapter` uses the shell-provided resource root to read `Resources/English.lproj/MainResources.rsrc`, validate known fixture records through the portable `u3_resource` parser, build the resource-backed render smoke frame, build/load the native new-game smoke document, load the overworld smoke map, and inspect the portable party/roster summary. `ShellSaveAdapter` stages, validates, atomically replaces, and reads back the save smoke document. The game host status overlay reports command, tick, resource/render, and save-path smoke state; Game > Refresh Locations reruns the location/resource/render smoke, Game > Write Save Smoke runs the save write/read smoke, Game > Load New Game Smoke loads the portable save-domain state, Game > Inspect Party/Roster reports active party and leader roster state, and Game > Toggle Tick pauses or resumes the fixed timer.

Build the shell with:

```bash
swift build --product Ultima3ModernShell
```

Run shell adapter tests with:

```bash
swift test
```

Run the shell executable with:

```bash
swift run Ultima3ModernShell
```

This scaffold intentionally does not choose the final renderer, final audio mixing policy, resource conversion path, persistence format, or gameplay loop.
