# Modern Shell

`ModernShell` is the first AppKit-owned macOS shell scaffold for the Ultima III modernization effort.

AppKit owns lifecycle, menus, command routing, the primary window, and the game host view. SwiftUI is currently limited to a preferences panel boundary. The shell links the plain C `Ultima3Core` Swift package target to prove that modern shell code can call portable core code without exposing AppKit or SwiftUI types through `Core/`.

`GameHostView` renders a synthetic `u3_render_frame` through `AppKitRenderAdapter`. This is an early renderer adapter smoke path: the portable core describes clear, rectangle, and tile commands with plain C data, while AppKit owns the concrete drawing.

Keyboard, mouse, and menu actions pass through `ShellInputAdapter`, which translates AppKit-owned events into the portable `u3_input_queue` before the shell reports the consumed command. Macro command sequences use the same queue in legacy execution order.

`ShellAudioAdapter` is the first modern audio backend spike. It consumes portable `u3_audio_queue` events and uses AVFoundation inside the shell to play existing sound assets for smoke testing. Missing or unsupported assets are reported in shell status text instead of changing the portable core contract.

Build the shell with:

```bash
swift build --product Ultima3ModernShell
```

Run the shell executable with:

```bash
swift run Ultima3ModernShell
```

This scaffold intentionally does not choose the final renderer, final audio mixing policy, resource conversion path, persistence format, or game loop.
