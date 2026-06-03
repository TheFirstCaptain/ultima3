# Repository Guidelines

## Project Structure & Module Organization

This is a legacy macOS/Xcode project for LairWare's Ultima III. Core game code lives in `Sources/`, split between C modules (`UltimaMain.c`, `UltimaGraphics.c`, `UltimaDngn.c`) and Objective-C bridge/UI files (`CocoaBridge.m`, `PrefsDialog.m`, `main.m`). Declarations sit beside implementations as `.h` files. Bundled assets are under `Resources/`: strings in `Resources/English.lproj/`, sounds in `Resources/Sounds/`, music in `Resources/Music/`, graphics in `Resources/Graphics/`, and `Resources/Application.icns`. UI nibs and localized app metadata are in top-level `English.lproj/`. Reference PDFs are in `Images/`, cursor notes in `Cursors/`, and project metadata in `Ultima3.xcodeproj/`.

## Build, Test, and Development Commands

- `open Ultima3.xcodeproj`: open the project in Xcode for inspection or manual builds.
- `xcodebuild -project Ultima3.xcodeproj -target Ultima3 -configuration Debug build`: attempt a Debug build from the command line.
- `xcodebuild -project Ultima3.xcodeproj -target Ultima3 -configuration Release build`: attempt a Release build.

The project references legacy SDKs and frameworks including Carbon, QuickTime, `macosx10.6`, and a PowerPC SDK path. Modern Xcode installations may not build it without a legacy toolchain or project updates.

## Coding Style & Naming Conventions

Match the existing style. Use C/Objective-C files with `.c`, `.m`, and `.h` extensions. Keep module names descriptive and consistent with `Ultima*` for game systems and `LW*` for LairWare helpers. Existing code uses 4-space indentation, K&R-style C functions, Objective-C methods with opening braces on the method line, and globals prefixed with `g`. Prefer `#import` as used throughout the project. Avoid broad formatting-only changes in legacy files.

## Testing Guidelines

There is no automated test suite or dedicated test directory. Validate changes by building the `Ultima3` target where possible and manually exercising affected gameplay, UI dialogs, resources, or sound paths. For future tests, keep them isolated from bundled historical assets and name test files after the module under test, for example `UltimaGraphicsTests`.

After completing any coding task, run an independent subagent code review before final handoff. Treat review findings as part of the task: resolve them, or explicitly document why they are accepted residual risk. If subagent tooling is unavailable, state that clearly in the handoff.

## Commit & Pull Request Guidelines

Git history is minimal and uses short, sentence-case commit subjects such as `Initial commit` and `Final state of code for public release`. Keep new commit messages concise and imperative, for example `Fix sound volume clamping`. Pull requests should describe the changed behavior, note whether a legacy or modern Xcode build was attempted, list manual verification steps, and include screenshots for UI or rendering changes.

## Asset & Licensing Notes

The MIT license applies to code, but the README calls out historical non-code assets with uncertain rights. Do not add third-party franchise assets without documenting their source and license.
