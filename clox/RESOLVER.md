# The Resolver — Fixing the Dynamic Scoping Leak

## The Problem

After implementing closures, a subtle bug exists in the interpreter.
Variable lookup walks up the environment chain **at runtime** — meaning a
function can see variables that didn't exist when it was **defined**, only
when it was **called**.

### The Bug

```lox
var a = "global";
{
    fun showA() {
        print a;       // which 'a' does this refer to?
    }

    showA();           // prints "global"  ✓
    var a = "block";
    showA();           // prints "block"   ✗  (should still print "global")
}
```

**Expected:** `showA` always prints `"global"` — at the textual point where
`fun showA()` is declared, the only `a` in scope is the global one.

**Actual:** The second call prints `"block"` because:

1. `showA`'s `closure` is a `shared_ptr` to the **block's environment object**
2. After `var a = "block"` executes, that same object now contains `a = "block"`
3. When `showA()` looks up `a`, it finds `"block"` in the closure before
   reaching globals

This is **dynamic scoping** — the variable's meaning depends on what's in the
environment *at call time*, not at *definition time*.

---

## The Solution: Static Resolution

Before the interpreter runs, a **Resolver** walks the entire AST and computes,
for each variable use, exactly how many scopes up the variable lives. This
depth is fixed at *parse time* and never changes.

```
Source code → Scanner → Parser → AST → [Resolver] → Interpreter
                                              ↓
                                    locals map: {expr* → depth}
```

At runtime, instead of walking the chain until a name is found, the
interpreter jumps to the **exact pre-computed depth**:

```cpp
// Before resolver — dynamic walk (buggy)
environment->get(name);          // walks up until found

// After resolver — fixed depth lookup (correct)
environment->getAt(depth, name); // jumps exactly 'depth' levels up
```

---

## How the Resolver Works

### The Scope Stack

```cpp
std::vector<std::unordered_map<std::string, bool>> scopes;
//          ↑ one map per active scope
//                                        ↑ bool: false=declared, true=defined
```

A `vector` (not `stack`) so we can index into it for depth calculation.
Each entry is a map of variable name → whether it has been fully initialized.

The `bool` value catches this error:
```lox
var a = a;   // Error: Can't read 'a' in its own initializer
```
When we see `a` on the right-hand side, `a` is in the scope map with
`false` (declared but not yet defined) — we report the error.

---

### declare vs define

Variable resolution is split into two phases to catch the self-init error:

```
declare(name)  →  scope[name] = false   // "I know this name exists, but not ready yet"
  ... resolve initializer ...
define(name)   →  scope[name] = true    // "Now it's safe to use"
```

Example for `var x = 1 + 2`:
```
declare("x")   scopes = [{x: false}]
resolve(1+2)   — visits BinaryExpr, nothing to resolve
define("x")    scopes = [{x: true}]
```

Example for `var x = x` (error):
```
declare("x")   scopes = [{x: false}]
resolve(x)     — visitVariableExpr: finds x with false → ERROR
define("x")    never reached
```

---

### resolveLocal — Computing the Depth

```cpp
void Resolver::resolveLocal(Expr* expr, const Token& name) {
    for (int i = scopes.size() - 1; i >= 0; --i) {
        if (scopes[i].find(name.lexeme) != scopes[i].end()) {
            interpreter.resolve(expr, scopes.size() - 1 - i);
            return;
        }
    }
    // Not found → global variable, no entry needed
}
```

Walk from innermost scope (`scopes.back()`, index `size-1`) outward toward
globals (index `0`). When found at index `i`:

```
depth = (size - 1) - i
```

This is the number of hops from the **current** (innermost) scope to where
the variable lives.

**Example:** Three nested scopes, variable found in the middle one:

```
scopes = [ {a: global}, {x: outer}, {y: inner} ]
           index 0        index 1     index 2 (innermost, size-1)

Variable 'x' found at index 1:
depth = (3-1) - 1 = 1     → 1 hop up from innermost
```

At runtime `getAt(1)` walks exactly 1 level up — directly to the right
environment, bypassing anything added to inner scopes later.

---

### resolveFunction — Separate Scope for Parameters

```cpp
void Resolver::resolveFunction(Function* stmt) {
    beginScope();                     // new scope for function body
    for (const Token& param : stmt->params) {
        declare(param);               // parameters are pre-declared
        define(param);                // and immediately defined
    }
    resolve(stmt->body);              // resolve the body statements
    endScope();
}
```

Functions get their own scope so that parameters are treated as local
variables with depth 0 relative to the function body.

---

### visitFunctionStmt — declare before resolving body

```cpp
std::any Resolver::visitFunctionStmt(Function* stmt) {
    declare(stmt->name);   // add name to current scope immediately...
    define(stmt->name);    // ...and define it right away
    resolveFunction(stmt);
    return std::any();
}
```

Both `declare` and `define` happen *before* resolving the body. This allows
**recursive functions** — the function name is visible inside its own body:

```lox
fun fib(n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);   // 'fib' is already in scope here
}
```

---

## The Fix in Action

Going back to the original bug:

```lox
var a = "global";
{
    fun showA() { print a; }
    showA();
    var a = "block";
    showA();
}
```

**Resolver analysis of `print a` inside `showA`:**
```
scopes during resolution of showA's body:
  [ {} ]                       ← the block scope (showA not yet defined)
  [ {showA: true} ]            ← outer block scope  
  (no 'a' found in any scope)

→ 'a' not found in locals → it's global
→ interpreter.resolve() NOT called for this expr
```

**At runtime:**
```cpp
// lookUpVariable:
auto it = locals.find(expr);    // not found — it's global
if (it == locals.end()) return globals->get(name);   // always returns "global"
```

`globals` is fixed — `var a = "block"` goes into the block environment, not
globals. So both calls to `showA()` now correctly print `"global"`.

---

## Summary

| | Before Resolver | After Resolver |
|---|---|---|
| Variable lookup | Walk chain at runtime | Jump to pre-computed depth |
| `var a = "block"` after `fun` | Visible to function | Invisible (depth fixed at definition) |
| Scoping model | Dynamic | Lexical (correct) |
| Self-init `var x = x` | Silent wrong value | Compile-time error |
| Duplicate `var` in same scope | Silent shadowing | Compile-time error |
