# Repository Guidelines

## Project Structure & Module Organization
Pluto's runtime and compiler sources live in `src/`, following the upstream Lua layout with paired `.cpp`/`.h` files (e.g., `lapi.cpp` with `lapi.h`). Third-party extensions and generated switch tables sit under `src/vendor/` and `src/*.sun`. Build helpers reside in `scripts/`, while cross-platform project files (`Makefile`, `Pluto.sln`) sit at the repo root. Functional tests and fixtures live in `testes/`; add new scenarios beside `_driver.pluto` so they are discovered by the harness.

## Build, Test, and Development Commands
Use the PHP scripts for the fastest clang workflow:
```
php scripts/compile.php clang && \
php scripts/link_pluto.php clang && \
php scripts/link_plutoc.php clang
```
Invoke `make -j PLAT=linux` for a full GCC build on Linux. Both paths emit `pluto` and `plutoc` into `src/`. During incremental work, rebuild a single translation unit by touching its `.cpp` and rerunning the relevant script; Visual Studio users can open `Pluto.sln` to build the same targets.

## Coding Style & Naming Conventions
Match the Lua-derived C++ style: two-space indents, no tabs, and braces on the same line as control statements. Keep function and file names lowercase with underscores or digits (`index2value`, `ljson.hpp`), and reserve PascalCase for classes introduced in vendor code. Macros stay uppercase, constants use the `PLUTO_*` family, and headers should include the Lua prefix guard pattern already present.

## Testing Guidelines
Run the full suite with `src/pluto testes/_driver.pluto`; it exercises interpreter and compiler paths together. When adding tests, keep filenames descriptive (`testes/json/encode_numbers.pluto`) and assert both success and failure modes. New features require companion tests that fail before your change and pass afterward; document any non-determinism in the test body.

## Commit & Pull Request Guidelines
Follow the existing history by writing short, imperative commit subjects (`Fix GC debt accounting`, `[Soup] Update request fallback`). Each commit should compile and pass tests. Pull requests need a clear summary, reproduction steps for bug fixes, and a checklist of the commands you ran. Reference related issues in the description and attach logs or screenshots when behavior changes.
