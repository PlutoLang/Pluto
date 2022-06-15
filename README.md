# Pluto
Pluto is a fork of the Lua 5.4.4 programming language. Internally, it's mostly PUC-Rio Lua 5.4, but as I continue to make small modifications, over time it'll be its own little thing. This is why I ascribed the name 'Pluto', because it'll always be something small & neat. The main changes right now are expansions to Lua's syntax, but performance optimizations and QoL features like optional compile-time warnings are under development too.

Pluto will retain a heavy focus on portability, but it will be larger & less light-weight than Lua. I intend on adding features to this that'll make Lua (or in this case, Pluto) more general-purpose and bring more attention to the Lua programming language, which I deeply love. Pluto will not grow to the enormous size of Python, obviously, but the standard library will most definitely be expanded. So far, many operators have been added & overhauled. There are new virtual machine optimizations, new syntax such as lambda expressions & string indexing, and more that you can read below.

### Note
This is a learner's project concerning the internals of Lua. I will often add and change things simply to learn more about my environment. As a result, there will be breaking changes very often. There may be bugs, and there will be design choices that people probably don't agree with. However, this will become a good base to write more Lua patches from. I do welcome other people in using Pluto as a reference for their own Lua patches — because Pluto offers some neat improvements over Lua already.

## Breaking changes:
- None.

Pluto is fully backwards-compatible (inc. C-ABI) with Lua 5.4.
## New Features:
### Dedicated Exponent Operator
The `**` operator has been implemented into the operator set. It's roughly an alternative to `^` now.
### Arbitrary Characters in Numeral Literals
Long numbers can get confusing to read for some people. `1_000_000` is now a valid alternative to `1000000`.
### String Indexing
String indexing makes character-intensive tasks nearly 3x faster. The syntax is as follows:
```lua
local str = "hello world"
assert(str[5] == "o")
assert(str[200] == nil)
```
This is a very nice addition because it avoids a lookup and function call for each character. Things like (i.e, hash algorithms) will significantly benefit from this change.
### Lambda Expressions
Without the size constraint of Lua, there's no need to hold weary of shorthand expressions.
Here's example usage of the new lambda expressions:
```lua
local str = "123"
local inc_str = str:gsub(".", |c| -> tonumber(c) + 1)
assert(inc_str == "234")
```
- The '|' token was chosen because it's not commonly used as an unary operator in programming.
- The '->' arrow syntax looked better and didn't resemble any operators. It also plays along with common lambda tokens.
### Ternary Expressions
Ternary expressions allow seamless implementation of short statements which may assign falsy values. It also allows shorter, and more concise logic. Here is an example.
```lua
local max
if a > b then
  max = a
else
  max = b
end
```
Can be translated into this:
```lua
local max = if a > b then a else b
```
Essentially, `if` symbols inside expressions are ternary. Otherwise, they are statements. Ternary expressions don't accept `end` as a termination, the control structure will jump to evaluate the sub-expression. Sub-expressions are evaluated on a need-to-know basis, such that the `else` block will never run if the preceding logic is true. Unlike the `if` statement, ternary expressions do not support of `else if`, this is outside the purpose of ternary expressions in C.

Traditionally, this would be possible with the `and` symbol, however I personally dislike this syntax because it's inconsistent with other languages. Until informed otherwise, people will assume `and` exclusively returns booleans. It's not immediately obvious — in fact, I meet many good Lua programmers who are unaware — that `and` can be used to emulate ternary expressions. The `if/then/else` syntax is imperfect, but I believe it's better. Not only visually, but also in functionality; such that `and` depends on the truthyness of its operands.
### Continue Statement
Although very similar to goto usage, the continue statement has been implemented into Pluto. Usage follows:
```lua
local i = 0
repeat
    i = i + 1
    if i == 6 then
        continue
    end
    print(i - 1)
until i > 10
```
This will print every number besides 5, because `continue` will skip towards the next iteration of the loop. It's nearly identical to this:
```lua
repeat
    i = i + 1
    if i == 6 then
        goto continue
    end
    print(i - 1)
    ::continue::
until i > 10
```
However, the dedicated statement doesn't complicate pre-defined goto labels, aligns with other language routines, and is slightly more user-friendly. The `continue` statement also isn't limited by the negatives a label would imply, such that:
```lua
local i = 0
repeat
    i = i + 1
    if i == 6 then
        goto ::cont::
    end
    local a
    print(i - 1)
    local b
    ::cont::
until i > 10
```
Would error, since `::cont::` jumps into a new local scope. However,
```lua
local i = 0
repeat
    i = i + 1
    if i == 6 then
        continue
    end
    local a
    print(i - 1)
    local b
until i > 10
```
Works fine, because `continue` doesn't need to prepare for an edge case where a jump is performed before logic.
### Compiler Warnings
Pluto now offers optional compiler warnings for certain misbehaviors. Currently, this is applied only to duplicated local definitions. These internal checks are faster, and more reliable than analytical third-party software. Compiler warnings need to be explicity enabled with the `-D` flag, which is optimal for developers and users alike. For an example, see this code:
```lua
local variable = "hello world"
do
    local variable = "shadowed"
end
```
Given you run the file like this: `pluto -D file.plu`, the parser will emit the following message to standard error output:

![This image failed to load.](https://i.imgur.com/cyQpvjB.png)

This feature can be removed from Pluto via the `PLUTO_PARSER_WARNING_LOCALDEF` macro in `luaconf.h`.
### Augmented Operators
The following augmented operators have been added:
- Modulo: `%=`
- Addition: `+=`
- Exponent: `^=`
- Bitwise OR: `|=`
- Subtraction: `-=`
- Bitshift left: `<<=`
- Bitwise AND: `&=`
- Float division: `/=`
- Bitshift right: `>>=`
- Multiplication: `*=`
- Integer division: `//=`

These are all syntactic sugar for the usual binary operation & manual assignment, as such they produce nearly the same bytecode. In some instances, Pluto's augmented operators are faster than Lua. This happens because augmented operators temporarily store the left-hand operand inside of a register & share the value to the expression, whereas Lua would request the value twice.
## Optimizations:
### For Loops
Xmilia Hermit discovered an interesting for loop optimization on June 7th, 2022. It has been implemented in Pluto.
For example, take this code:
```lua
local t = {}
for i = 1, 10000000 do
    t[i] = i
end

local hidetrace
local start = os.clock()
for key, value in ipairs(t) do
    hidetrace = key
    hidetrace = value
end
print(os.clock() - start)
```
It takes roughly 650ms to run on my machine. Following the VM optimization, it takes roughly 160ms.
This optimization takes place in any loop that doesn't access the TBC variable returned by the `pairs` and `ipairs` function. See these returns:
```
pairs: next, table, nil, nil
ipairs: ipairsaux, table, integer, nil
```
When the latter `nil` TBC variable is never accessed, this optimization will occur.
## QoL Improvements
These are modifications that don't really add something new, but improve existing behavior. Certain syntax errors now have more descriptive error messages, and some include a guide on how to correctly use Pluto's syntax. For example:
```lua
function ()
    return 10
end
```
This code would normally return an error like, "expected `<name>` on line 98". Of course, this is plenty of information to solve the problem. However, here is the new error message:
```
file.plu:1: syntax error: expected <name> to perform as an identifier.
        1 | function ()
          |         ^ here: missing function name.
          |
note: You may've forgot to name your function during declaration. 
      Functions must be associated with names when they're declared.
      Here's an example inside the PIL: https://www.lua.org/pil/5.html
```
Another example: 
```lua
if true return 5 end
```
Produces the following result:
```
file.plu:1: syntax error: expected 'then' to delimit 'if' condition.
           1 | if ... return
             |       ^ here: missing 'then' symbol.
             |
note: You forgot to finish your condition with 'then'.
      Pluto requires this symbol to append each condition.
      If needed, here's an example of how to use the 'if' statement: https://www.lua.org/pil/4.3.1.html
```
This also supports ANSI color codes, however this is disabled by default in order to encourage portability. For example, ANSI color codes do not work on most Windows command prompts. Define the `PLUTO_USE_COLORED_OUTPUT` macro in `luaconf.h` to enable colored error messages — they look quite nice.
- For most Windows 10 users, you can enable ANSI color code support with this shell command:
  - `REG ADD HKCU\CONSOLE /f /v VirtualTerminalLevel /t REG_DWORD /d 1`

## Standard Library Additions
### `_G`
- `newuserdata` function.
### `string`
- `endswith` function.
- `startswith` function.