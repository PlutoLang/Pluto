# Pluto
Pluto is a fork of the Lua 5.4.4 programming language. Internally, it's 99.5% PuC-Rio Lua 5.4, but as I continue to make small modifications, over time it'll be its own little thing. This is why I ascribed the name 'Pluto', because it'll always be something small & neat. The main changes right now are overhauls to Lua's operators, which will be adjusted to meet the C standard.

Pluto will not have a heavy focus on light-weight and embeddability, but it'll be kept in mind. I intend on adding features to this that'll make Lua (or in this case, Pluto) more general-purpose and bring more attention to the Lua programming language, which I deeply love. Pluto will not grow to the enormous size of Python, obviously, but the standard library will most definitely be expanded. Standardized object orientation in the form of classes is planned, but there's no implementation abstract yet.

### Note
This is a learner's project concerning the internals of Lua. I will often add and change things simply to learn more about my environment. As a result, there will be breaking changes very often. There may be bugs, and there will be design choices that people probably don't agree with. However, this will become a good base to write more Lua patches from. I do welcome other people in using Pluto as a reference for their own Lua patches — because Pluto offers some neat improvements over Lua already.

## Breaking changes:
- `!=` is the new inequality operator.
- The `^` operator now performs bitwise XOR instead of exponentiation.
- The former `~=` inequality operator has been changed to an augmented bitwise NOT assignment.

## Optimizations:
- Cases of `x = x / 2` are now optimized into `x = x * 0.5` by the parser. This is 15% faster.
  - This only applies when both operands are numbers.
- Cases of `x = x ** 2` are now optimized into `x = x * x` by the parser. This is 35% faster.
  - This only applies when both operands are numbers.

## New Features:
### Dedicated Exponent Operator
The `**` operator has been implemented into the operator set. It has replaced the previous use of '^'.
### Arbitrary Characters in Numeral Literals
Long numbers can get confusing to read for some people. `1_000_000` is now a valid alternative to `1000000`.
### Lambda Expressions
Without the size constraint of Lua, there's no need to hold weary of shorthand expressions.
Here's example usage of the new lambda expressions:
```lua
local t = {
    9, 8,
    7, 6,
    5, 4,
    3, 2,
    1, 0
}
table.sort(t, |a, b| -> a < b)

for key, value in ipairs(t) do
    print(value)
end
```
This will sort the table as expected. The syntax is as follows:
```
|explist| -> expr
|a, b, c| -> expression
```
Another example of the new lambda expressions:
```lua
local str = "123"
local inc_str = str:gsub(".", |c| -> tonumber(c) + 1)
assert(inc_str == "234")
```
- The '|' token was chosen because it's not commonly used as an unary operator in programming.
- The '->' arrow syntax looked better and didn't resemble any operators. It also plays along with common lambda tokens.
### Augmented Operators
The following augmented operators have been added:
- Modulo: `%=`
- Addition: `+=`
- Bitwise OR: `|=`
- Subtraction: `-=`
- Bitwise XOR: `^=`
- Bitwise NOT: `~=`
- Bitshift left: `<<=`
- Bitwise AND: `&=`
- Float division: `/=`
- Bitshift right: `>>=`
- Multiplication: `*=`
- Integer division: `//=`

## Standard Library Additions
### `_G`
- `newuserdata` function.
### `string`
- `endswith` function.
- `startswith` function.