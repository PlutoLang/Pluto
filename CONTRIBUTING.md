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

## Submitting issues/PRs

- Title: Describe the issue/change/request.
- Description: The best description is an empty one because you wrote a good title, but if you need one, keep it concise; you certainly shouldn't need more than 3 paragraphs to describe *anything*. For bug reports, a code snippet of a minimal repro would be good context to add.

In less subtle terms: If you are to use AI, use it as a tool, not as a replacement for your own understanding and words.
