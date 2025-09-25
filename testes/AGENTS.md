# Repository Guidelines

## Directory Overview
`testes/` holds Pluto’s end-to-end regression suite. Top-level `.lua` files mirror the stock Lua test corpus, while `.pluto` cases exercise compiler-specific behavior. The `_driver.pluto` harness discovers sibling scripts, orchestrates compilation, and reports structured diagnostics. Subdirectories such as `pluto/` and `bench/` group specialized scenarios; add new folders only when you introduce a distinct domain of coverage.

## Writing & Organizing Tests
Name new files after the feature under test (`json_roundtrip.pluto`, `ffi_alignment.lua`). Tests should be self-contained, avoid network access, and use deterministic seeds. For multi-file scenarios, place fixtures alongside the test and guard temporary artifacts with `os.remove` in a `finally` block. Prefer assertions through `assert` with descriptive messages, and ensure every negative test explicitly checks the failure path via protected calls.

## Execution Workflow
Run the suite with `src/pluto testes/_driver.pluto` once Pluto is built. To target a single file, pass its relative path: `src/pluto testes/_driver.pluto testes/pluto/basic.pluto`. Benchmark scripts in `bench/` are optional; execute them manually when performance-sensitive code changes (`src/pluto testes/bench/*.pluto`). Capture timing deltas before and after your patch when reporting regressions.

## Coding Style & Conventions
Follow Lua’s idioms: two-space indentation, localize variables, and keep module-level state minimal. Tests may require small helper functions—place them at the top of the file and prefix with `_` if they are not part of the scenario’s assertions. Use lowercase filenames with underscores, mirroring the existing corpus, and keep line endings Unix-style for cross-platform consistency.

## Contribution Checklist
- Update `_driver.pluto` when introducing new discovery patterns or flags.
- Document any flaky behavior in the test header comment and open an issue.
- Run the full suite on at least one non-macOS platform for platform-specific fixes.
- Include failing seed/log excerpts in the PR body when addressing bugs caught here.
