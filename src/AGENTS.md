# Repository Guidelines

## Directory Overview
The `src/` tree mirrors Lua’s core layout, with each subsystem split into paired `.cpp` and `.h` files (`lapi.cpp`/`lapi.h`, `lstate.cpp`/`lstate.h`, etc.). Top-level `.sun` tables are generated jump tables; modify the generator rather than editing them by hand. Vendor dependencies such as Soup live inside `src/vendor/`. Keep new runtime modules in the root alongside their peers, and surface public headers through `lua.h`/`luaconf.h` only when the API must be exposed.

## Build & Integration Workflow
Recompile this directory with the PHP helpers (`php scripts/compile.php clang`) or `make -j PLAT=linux`; both traverse every `.cpp` here. Adding a new translation unit requires updating the aggregators in `scripts/common.php::for_each_obj()` or the `Makefile` object list if the filename deviates from the `l*.cpp` convention. Generated binaries `pluto` and `plutoc` are written back into this directory—do not move them without updating the toolchain scripts.

## Coding Style & Conventions
Follow the Lua house style: two-space indentation, trailing spaces trimmed, and braces on the same line as control keywords. Function names use the `lua_` or `pluto_` prefixes for public API, while internals prefer the `l` + subsystem prefix (`luaD_throw`, `gco2ts`). Keep headers self-contained with include guards mirroring the filename (`LAPI_H`). Prefer macros already defined in `llimits.h` and reuse helper inline functions rather than crafting new ad-hoc utilities. When touching GC or VM files, update the related comments at the top of each translation unit.

## Testing & Validation
After significant changes, run `php scripts/compile.php clang && php scripts/link_pluto.php clang` and then execute `src/pluto testes/_driver.pluto`. For GC or coroutine work, also run focused tests such as `src/pluto testes/gc.lua`. Capture perf regressions by comparing `src/pluto testes/bench/*.pluto` before and after your patch if you modify hot loops.

## Contribution Checklist
- Update `lua.hpp` and `lua.h` when you expose new API surface.
- Keep platform-specific branches guarded by `#if defined` blocks grouped in `luaconf.h`.
- Document non-obvious invariants in the file header comment block.
- Coordinate ABI-impacting changes with `plutoc` maintainers before merging.
