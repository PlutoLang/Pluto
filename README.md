# Pluto
Pluto is a fork of the Lua 5.4 programming language. Lua is designed to be a small, fast, and embeddable language. It's used very often in games and resource-restricted regions. Lua bites a bullet by needing to stay small though, it lacks general-purpose features. This strain on the language makes it difficult to produce complex scripts at an accelerated rate. It takes more time to implement alternatives for missing syntax, or writing your own manual functions to split strings.

Pluto is a dialect of Lua which intends on removing these cons, which makes Pluto a best-of-both-worlds Lua dialect.

### Why use Pluto over other dialects?
- **Diffable.**
  - Pluto aims to retain some diffability with Lua, so it won't just be a Lua *5.4* dialect.
- **Runs Lua.**
  - Pluto's VM can run Lua 5.4 bytecode. It aims to be able to parse/compile Lua 5.4 code, and to that end, it removes language limitations and offers compatibility options to ensure Pluto keywords don't break existing code.
- **Compiles to Lua.**
  - Many features in Pluto are achieved entirely in the lexer, parser, or code generator, but don't require any VM patches, therefore you can leverage faster & more type-safe code while still running it with a stock Lua VM. Not all features are compatible though, and our documentation should tell you which.
- **Drag & Drop Compatibility.**
  - Pluto is the only Lua dialect on the planet that's proven it can be dropped into massive [communities](https://stand.gg/) (50K> users), and not a break a single existing script. 

### Note
Thanks to everyone who's provided a star towards Pluto. I do notice every single person that does so, and it provides me excellent motivation to continue working on Pluto. On this note, please do send PRs and plenty of bug reports if you stumble upon any. All help is appreciated.

## Documentation

Instructions on how to install Pluto, as well as a detailed documentation for all of its additions and improvements can be found [here](https://plutolang.github.io/docs/Introduction).

## Related links

- [Repository for Website](https://github.com/PlutoLang/plutolang.github.io)
- [Write Pluto Online](https://plutolang.github.io/web/)