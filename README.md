<p align='center'>
  <img alt="Pluto Logo" width="20%" src="https://avatars.githubusercontent.com/u/108627128" /><br>
</p>
Pluto is a unique dialect of Lua with a focus on general-purpose programming.

### Why should you choose Pluto?
- **Accelerated Development.**
  - Greatly enhanced standard library.
  - Several new syntaxes, such as switch statements, compound operators, ternary expressions, etc.

- **Focused On Lua Compatibility.**
  - Pluto is largely compatible with Lua 5.4 source code, but there is an imperfection:
    - Pluto implements new keywords, which can cause conflicts with otherwise normal identifiers such as 'switch', or 'class'. We offer features — such as Compatibility Mode — to relieve this issue, see our [documentation](https://pluto-lang.org/docs/Compatibility).
  - Pluto is also compatible with Lua 5.4 bytecode. Pluto can execute Lua bytecode, and most Pluto features generate bytecode compatible with Lua.
    - There's a small subset of Pluto features which do not generate Lua 5.4 bytecode. This is documented alongside those features, so scripters can vouch to avoid using them when bytecode compatibility is desired.
  - Pluto has been dropped into large [communities](https://stand.gg/) (100K> users), and did not break any existing scripts.
  - Pluto actively rebases with Lua's main repository. We are not a time-frozen dialect. When Lua 5.5 releases, we intend on updating to that.

## Documentation

A detailed documentation of getting started with, tooling for, and the additions and improvements of Pluto can be found [on our website](https://plutolang.github.io/docs/Introduction), which is [open-source as well](https://github.com/PlutoLang/plutolang.github.io).

### Getting Started

You can use Pluto right in your browser [in the interactive playground](https://plutolang.github.io/web/), or find [pre-built binaries](https://github.com/PlutoLang/Pluto/releases) on our releases page. [Read more...](https://pluto-lang.org/docs/Getting%20Started)

### Tooling

- [Pluto Syntax Highlighting](https://github.com/PlutoLang/Syntax-Highlighting)
- [Pluto Language Server](https://github.com/PlutoLang/pluto-language-server)
