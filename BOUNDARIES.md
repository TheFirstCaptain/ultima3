# Implementation Boundary Checklist

Use this checklist before implementing or reviewing modern shell, adapter, or core changes. It summarizes the accepted F-011 shell decision and the target architecture rules; `ARCHITECTURE.md` and `DECISION_LOG.md` remain the source of record.

## Shell Ownership

- AppKit must own the modern shell lifecycle, primary windows, menus, command routing, keyboard and mouse event intake, fullscreen/window behavior, game host view, and concrete save-file location provider.
- SwiftUI must be limited to preferences, inspectors, setup flows, and future non-game panels unless a later decision explicitly changes ownership.
- SwiftUI must not own shell lifecycle, command model, primary game surface, deterministic game loop, or first-hop game input routing in the first shell milestone.
- SDL3 may be considered later as a renderer/input portability fallback inside an AppKit-owned shell, but it is not the first app shell.

## Core Boundary

- `Core/` public headers must not expose AppKit, SwiftUI, SDL, Metal, AVFoundation, Foundation filesystem, sandbox, or other platform framework types.
- The portable game core must not call macOS UI, audio, filesystem, resource, rendering, preference, or application lifecycle APIs directly.
- Core-facing APIs should use portable C data, result structs, events, command buffers, or explicit state transitions.
- State mutations must be explicit and testable before they move into stable core-facing APIs.

## Adapter Boundary

- Renderer, input, audio, resource, and persistence implementations must remain behind adapters.
- Platform adapters must translate modern app events and services into core-facing interfaces.
- The app shell may provide concrete bundle and save-file URLs or paths, but resource parsing and persistence serialization remain adapter-owned.
- The F-011 shell decision does not choose the final renderer, final audio backend, final asset conversion path, or persistence format beyond the accepted persistence direction.

## Pre-Review Checks

Run these checks before final review when a change touches `Core/`, the modern shell, or adapters:

```bash
rg -n "AppKit|SwiftUI|Metal|AVFoundation|Foundation|SDL|NS[A-Z][A-Za-z]+\\b|NSURL|Sandbox" Core
make -C harness test
```

If a match in `Core/` is intentional, document why it does not violate the public-header or portable-core boundary before handoff.
