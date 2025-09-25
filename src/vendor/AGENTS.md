# Repository Guidelines

## Directory Overview
The `src/vendor/` folder contains third-party code bundled with Pluto. Currently it hosts the Soup networking and utility library under `Soup/`, including build helpers (`build_lib.php`, `build_common.php`) and the upstream source tree in `Soup/soup/`. Treat this directory as vendored code: prefer upstream updates or patches over heavy local modifications, and keep licensing artefacts (`LICENCE`) intact when syncing.

## Update & Build Workflow
To refresh Soup, run `php src/vendor/Soup/build_lib.php <compiler>` or call `build_soup()` indirectly via `php scripts/compile.php clang`. The script compiles Soup into `libsoup.a` alongside headers that Pluto links against. When upgrading the upstream version, execute `src/vendor/Soup/soup/_update.php` with the desired tag and capture the resulting diff in a dedicated commit. Always rebuild Pluto afterward to verify ABI compatibility.

## Coding Style & Patch Conventions
Respect the upstream formatting: Soup uses tabs, CamelCase types, and extensive header-only templates. When local fixes are unavoidable, isolate them behind `#ifdef PLUTO_*` guards and reference the upstream issue in a nearby comment. Keep custom patches minimal, document them in the PR description, and prepare them as cherry-pickable commits so they can be forwarded upstream. Never rename vendor files; such changes break automated update scripts.

## Testing & Validation
After modifying vendor code or build scripts, run `php scripts/compile.php clang && php scripts/link_pluto.php clang` and execute targeted network tests such as `src/pluto testes/events.lua`. For TLS or HTTP updates, add regression coverage in `testes/pluto/` using mocked responses. Validate Windows builds with a Mingw toolchain plus `php scripts/link_shared.php mingw` to ensure Soup’s optional components still compile.

## Contribution Checklist
- Record the exact upstream commit or release in the changelog or PR body.
- Update vendored licences if upstream changes them.
- Re-run `_update.php` instead of manual edits when possible.
- Notify maintainers before bumping major Soup versions to plan dependent changes.
