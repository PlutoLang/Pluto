## Building

The fastest way to compile Pluto is using the PHP build scripts, e.g. with Clang:
```
php scripts/compile.php clang
php scripts/link_pluto.php clang
php scripts/link_plutoc.php clang
```

But it can also be compiled via Make with GCC, e.g. multithreaded compile on Linux:
```
make -j PLAT=linux
```

In both cases, the binaries `pluto` and `plutoc` will be put in the `src/` folder.

## Testing

After building Pluto, you can run the test suite like so:
```
src/pluto testes/_driver.pluto
```
