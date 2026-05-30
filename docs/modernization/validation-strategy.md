# Validation Strategy

Use this document to define how modernization work proves behavior was preserved or intentionally changed.

## Current State

The legacy app may not build or run on modern Apple silicon Macs. There is no automated test suite yet.

## Validation Levels

1. Static inspection of relevant legacy files and project metadata.
2. Legacy build attempt when a suitable toolchain is available.
3. Focused harness tests around extracted or ported behavior.
4. Manual verification for gameplay, rendering, input, audio, and save/load behavior.
5. Modern app build and smoke test once a modern target exists.

## Characterization Targets

| Target | Legacy Reference | Harness/Test Approach | Status |
| --- | --- | --- | --- |
|  |  |  |  |

## Skipped Validation

Record skipped checks with the reason, date, and impact.

