# Pluto
Pluto is a fork of the Lua 5.4.4 programming language. Internally, it's 99.5% PuC-Rio Lua 5.4, but as I continue to make small modifications, over time it'll be its own little thing. This is why I ascribed the name 'Pluto', because it'll always be something small & neat. The biggest changes right now is augmented assignment and differences in the inequality operator. 

Pluto will not have a heavy focus on light-weight and embeddability, but it'll be kept in mind. I intend on adding features to this that'll make Lua (or in this case, Pluto) more general-purpose and bring more attention to the Lua programming language, which I deeply love. Pluto will not grow to the enormous size of Python, obviously, but the standard library will most definitely be expanded. Standardized object orientation in the form of classes is planned, but there's no implementation abstract yet.

Pluto does, however, have a very strong focus on remaining bytecode-compatible with the latest versions of Lua. As such, anything written with the quirks of this fork can be compiled and ran by the Lua 5.4 virtual machine. I can't quite state that this will remain true forever, because it heavily depends on how many features I can add without needing to implement new opcodes.

## New Features
### Breaking changes:
- `!=` is the new inequality operator.
- The former `~=` inequality operator has been changed to an augmented bitwise XOR assignment.
### Augmented Operators:
- Modulo: `%=`
- Addition: `+=`
- Exponent: `^=`
- Bitwise OR: `|=`
- Subtraction: `-=`
- Bitwise XOR: `~=`
- Bitshift left: `<<=`
- Bitwise AND: `&=`
- Float division: `/=`
- Bitshift right: `>>=`
- Multiplication: `*=`
- Integer division: `//=`

### Lua Compatibility
As things currently stand, compiled Pluto is bytecode-compatible with Lua 5.4.