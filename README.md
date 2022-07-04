# Pluto
Pluto is a fork of the Lua 5.4.4 programming language. Internally, it's mostly PUC-Rio Lua 5.4, but as I continue to make small modifications, over time it'll be its own little thing. This is why I ascribed the name 'Pluto', because it'll always be something small & neat. The main changes right now are expansions to Lua's syntax, but performance optimizations and QoL features like optional compile-time warnings are under development too.

Pluto will retain a heavy focus on portability, but it will be larger & less light-weight than Lua. I intend on adding features to this that'll make Lua (or in this case, Pluto) more general-purpose and bring more attention to the Lua programming language, which I deeply love. Pluto will not grow to the enormous size of Python, obviously, but the standard library will most definitely be expanded. So far, many operators have been added & overhauled. There are new virtual machine optimizations, new syntax such as lambda expressions & string indexing, and more that you can read below.

### Note
Thanks to everyone who's provided a star towards Pluto. I do notice every single person that does so, and it provides me excellent motivation to continue working on Pluto. On this note, please do send PRs and plenty of bug reports if you stumble upon any. All help is appreciated.

## Breaking changes:
Pluto is largely backwards-compatible with Lua 5.4. This includes compiled bytecode, because Pluto does not implement any new opcodes, or alter the fundamental behavior of any opcodes. However, Pluto does add several new syntaxes which, in rare circumstances, may invalidate previously valid Lua identifiers. This includes the following identifiers:
- `case`
- `switch`
- `default`
- `continue`

If you use these as tables keys, then you can use bracket syntax for compatibility:
```lua
local t = {
  default = 5 -- Error.
  ["default"] = 5 -- Works fine.
}

print(t.default) -- Error.
print(t["default"]) -- Works fine.
```
Otherwise, you have two options:
  1. Refactor your code.
  2. Compile Pluto in compatibility mode.
If you wish to compile Pluto in compatibility mode, all new keywords will be prefixed with 'pluto_' to avoid breaking existing identifiers. Toggle the `PLUTO_COMPATIBLE_MODE` macro in `luaconf.h`. More information can also be found in that file.
## New Features:
### Alternative Operators
- `**` is a valid alternative to `^`, the power operator.
- `!=` is a valid alternative to `~=`, the inequality operator.
### Arbitrary Characters in Numeral Literals
Long numbers can get confusing to read for some people. `1_000_000` is now a valid alternative to `1000000`.
### String Indexing
String indexing makes character-intensive tasks nearly 3x faster. The syntax is as follows:
```lua
local str = "hello world"
print(str[5]) -- "o"
print(str[200]) -- nil
print(str[-1]) -- "d"
```
This is a very nice addition because it avoids a lookup and function call for each character. Things like (i.e, hash algorithms) will significantly benefit from this change. Furthermore, it's friendly with normal instance lookups. So, things like `str["char"]` still work fine. String indexing is invoked whenever a string index is invoked with an integer.

<a href="https://plutolang.github.io/#code=local%20str%20%3D%20%22hello%20world%22%0D%0Aprint(str%5B5%5D)%20--%20%22o%22%0D%0Aprint(str%5B200%5D)%20--%20nil%0D%0Aprint(str%5B-1%5D)%20--%20%22d%22">Try it yourself!</a>
### Lambda Expressions
Without the size constraint of Lua, there's no need to hold weary of shorthand expressions.
Here's example usage of the new lambda expressions:
```lua
local str = "123"
local inc_str = str:gsub(".", |c| -> tonumber(c) + 1)
print(inc_str) -- "234"
```
- The '|' token was chosen because it's not commonly used as an unary operator in programming.
- The '->' arrow syntax looked better and didn't resemble any operators. It also plays along with common lambda tokens.

<a href="https://plutolang.github.io/#code=local%20str%20%3D%20%22123%22%0D%0Alocal%20inc_str%20%3D%20str%3Agsub(%22.%22%2C%20%7Cc%7C%20-%3E%20tonumber(c)%20%2B%201)%0D%0Aprint(inc_str)%20--%20%22234%22">Try it yourself!</a>
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

<a href="https://plutolang.github.io/#code=local%20a%20%3D%206%0Alocal%20b%20%3D%209%0A%0Alocal%20max%20%3D%20if%20a%20%3E%20b%20then%20a%20else%20b%0A%0Aprint(max)">Try it yourself!</a>
### Continue Statement
Although very similar to goto usage, the continue statement has been implemented into Pluto. Usage follows:
```lua
-- Print every number besides five.
for i = 1, 10 do
    if i == 5 then
        continue
    end
    print(i)
    -- continue jumps here.
end
```
The new statement will jump to the end of the loop, so it may proceed to the next iteration. For all intents and purposes, it's the same as this:
```lua
for i = 1, 10 do
    if i == 5 then
        goto continue
    end
    print(i)
    ::continue::
end
```
However, the dedicated statement doesn't complicate pre-defined goto labels, aligns with other language routines, and is slightly more user-friendly. The `continue` statement also isn't limited by the negatives a label would imply, so you don't need to manage local scopes and other pedantry like you would a label. It's important to note, this new statement will jump over any code neccesary to end the loop. Meaning, if you jump over vital code that determines the conditional for your loop, then you will produce a bug.

<a href="https://plutolang.github.io/#code=--%20Print%20every%20number%20besides%20five.%0D%0Afor%20i%20%3D%201%2C%2010%20do%0D%0A%20%20%20%20if%20i%20%3D%3D%205%20then%0D%0A%20%20%20%20%20%20%20%20continue%0D%0A%20%20%20%20end%0D%0A%20%20%20%20print(i)%0D%0A%20%20%20%20--%20continue%20jumps%20here.%0D%0Aend">Try it yourself!</a>
### Table Immutability
Tables can now be frozen at their current state to forbid any future modification. This action is irreversible and permanent for the lifespan of the table.
```lua
-- Disallowing any edits to the global environment table.
table.freeze(_G)
_G.string = {} -- Fails, raises an error.

-- Performing edits, then freezing the resultant table.
local MyTable = {}
MyTable.key1 = "value 1"
MyTable.key2 = "value 2"
table.freeze(MyTable)
MyTable.key3 = "value 3" -- Fails, raises an error.
MyTable.key2 = "new value 2" -- Fails, raises an error.

-- Freezing upvalue tables.
table.freeze(_ENV)

-- Creating a constant local that's associated with a frozen table.
local Frozen = table.freeze({ 1, 2, 3 })
Frozen = {} -- Fails.
Frozen[1] = "new value" -- Fails.
rawset(Frozen, "key", "value") -- Fails.

--- Trying to swap the value with the debug library.
for i = 1, 249 do
  local name, value = debug.getlocal(1, i)
  if name == "Frozen" then
    debug.setlocal(1, i, { ["key"] = "hello world" }) -- Fails.
  end
end
```
This action will dissallow new elements and keys from being assigned. It'll also prevent modification of existing elements and keys. Furthermore, this prevents modification of the local's value via `debug.setlocal`, which can also be used to bypass const-ness.

If you intend on using this for sandboxing, ensure you call `table.freeze` before any users access the Lua environment, as they may be able to hook the function and replace it with something malicious or useless. Furthermore, local variables can still be reassigned since freezing only applies to the value. In this situation, you can take advantage of the `<const>` declaration modifier which will forbid local reassignment.

Alternatively, if you intend on creating a "true constant" with a table, such that the local can neither be reassigned or have its table manipulated, then this is fine too. There's hardly any overhead associated with freezing a table, because the action merely swaps a boolean around.

This change also implements the `table.isfrozen` function which takes a table, and returns a boolean.

<a href="https://plutolang.github.io/#code=--%20Disallowing%20any%20edits%20to%20the%20global%20environment%20table.%0D%0Atable.freeze(_G)%0D%0A_G.string%20%3D%20%7B%7D%20--%20Fails%2C%20raises%20an%20error.%0D%0A%0D%0A--%20Performing%20edits%2C%20then%20freezing%20the%20resultant%20table.%0D%0Alocal%20MyTable%20%3D%20%7B%7D%0D%0AMyTable.key1%20%3D%20%22value%201%22%0D%0AMyTable.key2%20%3D%20%22value%202%22%0D%0Atable.freeze(MyTable)%0D%0AMyTable.key3%20%3D%20%22value%203%22%20--%20Fails%2C%20raises%20an%20error.%0D%0AMyTable.key2%20%3D%20%22new%20value%202%22%20--%20Fails%2C%20raises%20an%20error.%0D%0A%0D%0A--%20Freezing%20upvalue%20tables.%0D%0Atable.freeze(_ENV)%0D%0A%0D%0A--%20Creating%20a%20constant%20local%20that's%20associated%20with%20a%20frozen%20table.%0D%0Alocal%20Frozen%20%3D%20table.freeze(%7B%201%2C%202%2C%203%20%7D)%0D%0AFrozen%20%3D%20%7B%7D%20--%20Fails.%0D%0AFrozen%5B1%5D%20%3D%20%22new%20value%22%20--%20Fails.%0D%0Arawset(Frozen%2C%20%22key%22%2C%20%22value%22)%20--%20Fails.%0D%0A%0D%0A---%20Trying%20to%20swap%20the%20value%20with%20the%20debug%20library.%0D%0Afor%20i%20%3D%201%2C%20249%20do%0D%0A%20%20local%20name%2C%20value%20%3D%20debug.getlocal(1%2C%20i)%0D%0A%20%20if%20name%20%3D%3D%20%22Frozen%22%20then%0D%0A%20%20%20%20debug.setlocal(1%2C%20i%2C%20%7B%20%5B%22key%22%5D%20%3D%20%22hello%20world%22%20%7D)%20--%20Fails.%0D%0A%20%20end%0D%0Aend">Try it yourself!</a>
### Compiler Warnings
Pluto now offers optional compiler warnings for certain misbehaviors. Currently, this is applied only to duplicated local definitions. These internal checks are faster, and more reliable than analytical third-party software. Compiler warnings need to be explicity enabled with the `-D` flag, which is optimal for developers and users alike. For an example, see this code:
```lua
local variable = "hello world"
do
    local variable = "shadowed"
end
```
Given you run the file like this: `pluto -D file.plu`, the parser will emit the following message:
```
tests/quick.lua:3: warning: duplicate local declaration [-D]
         3 | local variable =
           | ^^^^^^^^^^^^^^^^ here: this shadows the value of the initial declaration on line 1.
           |
```
This feature can be removed from Pluto by defining the `PLUTO_NO_PARSER_WARNINGS` macro in `luaconf.h` or your build config.
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
### Switch/Case Statement
Pluto now offers switch/case statement syntax that meets the C standard. Which means, case expressions must be compile-time constants, and proper fall-through is supported. Lua's `<const>` declaration modifier is not included as a constant expression.

The supported constant types are as follows:
- `nil`
- `string`
- `number`
- `boolean`

Here's an example:
```lua
local value = 3
switch value do
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    print "Got 1-5."
    break
  default:
    print "Value is greater than 5."
end
-- Break jumps here.
```
The `default` case runs if no break — or any other escape mechanism, like `return` or `goto` — is encountered throughout the entire structure.

Case blocks do not support an enclosed scope like `function` or `while` declarations do. Instead, they'll include every statement until a `break`, `end`, `default`, or `case` symbol is discovered. Breaking out of a switch/case statement will jump to a pre-defined label defined just outside of the block, like any normal loop. This statement does not support jump tables yet, but it's significantly faster than creating a lookup table. It's also slightly faster than a chain of `if` statements. Either way, it's significantly more readable.

The parenthesis around the control variable — in this case, `value` — are entirely optional. Colons are required to terminate every case expression, and are also required to terminate the `default` symbol. As seen above, switch blocks must enclose themselves with `do` & `end`, respectively.

<a href="https://plutolang.github.io/#code=local%20value%20%3D%203%0D%0Aswitch%20value%20do%0D%0A%20%20case%201%3A%0D%0A%20%20case%202%3A%0D%0A%20%20case%203%3A%0D%0A%20%20case%204%3A%0D%0A%20%20case%205%3A%0D%0A%20%20%20%20print%20%22Got%201-5.%22%0D%0A%20%20%20%20break%0D%0A%20%20default%3A%0D%0A%20%20%20%20print%20%22Value%20is%20greater%20than%205.%22%0D%0Aend%0D%0A--%20Break%20jumps%20here.">Try it yourself!</a>
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
### Table Length
The length of tables (`#mytable`) is now automatically cached by Pluto after you request it for the first time. The cache is updated whenever a change is made to the table. Following this optimization, fetching table length is roughly twice as fast. The `table.getn` function has been provided in case a bug appears with the cache. However, it's unlikely a bug will appear.
## QoL Improvements
These are modifications that don't really add something new, but improve existing behavior.

### Excessive Iteration Prevention (For Game Developers)
This is an optional, but fairly simple heuristic that limits the amount of backward-jumps performed prior to a forward-jump, which applies to loops. Every backward-jump is usually a new iteration, and forward jumps are usually escaping statements. Taking advantage of this circumstance, Pluto can bottleneck the amount of backward-jumps permitted to avoid excessive iterations — which are usually problematic for blocking game threads — and prevent the operation of infinite loops, or loops which may crash the game thread. You can enable this feature by defining the `PLUTO_ILP_ENABLE` macro in `luaconf.h` or your build config.

This supports a statically-defined bottleneck — `PLUTO_ILP_MAX_ITERATIONS` in `luaconf.h` — which defines the amount of permitted iterations. This also supports hooking the `OP_CALL` opcode, and preventing loop termination if a certain function is called. This was implemented because game threads usually force users to call some sort of `yield` mechanism to prevent a crash — `PLUTO_ILP_HOOK_FUNCTION` in `luaconf.h` — but, the function must be available in the Pluto runtime. Pluto will throw a runtime error during exception unless `PLUTO_ILP_SILENT_BREAK` is set, in which case Pluto will simply break out of the loop.

### Syntax Errors
Certain syntax errors now have more descriptive error messages. For example:
```lua
if a < b and t == 5 return "Gottem" end
```
Produces the following result:
```
pluto: tests/quick.lua:1: syntax error: expected 'then' to delimit condition.
         1 | if a < b and t == 5 return
           | ^^^^^^^^^^^^^^^^^^^^^^^^^^ here: expected 'then' symbol.
           |
```
Another example, with a mistyped lambda expression:
```lua
local fn = |a, b, c| => (a == b and a < c)
```
Emits this message:
```
pluto: tests/quick.lua:4: syntax error: impromper lambda definition
         4 | local b = |a, b, c| =>
           | ^^^^^^^^^^^^^^^^^^^^^^ here: expected '->' arrow syntax for lambda expression.
           |
```
This also supports ANSI color codes, however this is disabled by default in order to encourage portability. For example, ANSI color codes do not work on most Windows command prompts. Define the `PLUTO_USE_COLORED_OUTPUT` macro in `luaconf.h` or your build config to enable colored error messages — they look quite nice.
- For most Windows 10 users, you can enable ANSI color code support with this shell command:
  - `REG ADD HKCU\CONSOLE /f /v VirtualTerminalLevel /t REG_DWORD /d 1`

## Standard Library Additions
- `newuserdata` function.
- `table.freeze` function.
- `table.isfrozen` function.
- `table.contains` function.
- `string.endswith` function.
- `string.startswith` function.

## C API Additions
- `lua_setcachelen` function.
- `lua_freezetable` function.
- `lua_erriffrozen` function.
- `lua_istablefrozen` function.
- `L->l_G->user_data` member for custom data per `luaL_newstate`.

## Building Pluto
Pluto was built on C++17, with no backwards-compatibility ensured. Any C++ compiler capable of supporting that feature set should compile fine.
### Windows
#### GCC
- Install `MSYS` & add it to path.
- Install `chocolatey` & add it to path.
- `git clone https://github.com/well-in-that-case/Pluto.git`
- `cd Pluto`
- `make`

#### Visual Studio
Pluto utilizes v143 build tools as the platform toolset, which means you'll probably need Visual Studio 2022.
- `git clone https://github.com/well-in-that-case/Pluto.git`
- `cd Pluto`
- Open the solution file.
- Press build, obviously.

For other platforms, use `make PLAT=xxx` where `xxx` is a valid platform, found inside the makefile configuration. Furthermore, you'll need your compiler on your PATH variable for the makefile to locate it.