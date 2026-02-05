# std::variant vs Visitor Pattern & Avoiding Cyclic Dependencies

## Table of Contents
1. [Why Not std::variant?](#why-not-stdvariant)
2. [Visitor Pattern vs std::variant Comparison](#visitor-pattern-vs-stdvariant-comparison)
3. [When to Use Each](#when-to-use-each)
4. [Cyclic Dependency Problem](#cyclic-dependency-problem)
5. [How We Avoided Cyclic Dependencies](#how-we-avoided-cyclic-dependencies)
6. [Best Practices](#best-practices)

---

## Why Not std::variant?

### What is std::variant?

`std::variant` is a C++17 feature that represents a type-safe union - it can hold one of several specified types:

```cpp
// Using std::variant instead of inheritance
using Expr = std::variant<
    Literal,
    Binary,
    Unary,
    Grouping,
    Variable,
    Assign,
    Logical,
    Call,
    Get,
    Set,
    This,
    Super
>;
```

### Alternative Implementation with std::variant

```cpp
// Define all expression types as regular structs
struct Literal {
    std::any value;
};

struct Binary {
    std::unique_ptr<Expr> left;  // Recursive!
    Token op;
    std::unique_ptr<Expr> right;
};

struct Unary {
    Token op;
    std::unique_ptr<Expr> right;
};

// The Expr type is a variant of all possible expressions
using Expr = std::variant<
    Literal,
    Binary,
    Unary,
    Grouping,
    Variable,
    Assign,
    Logical,
    Call,
    Get,
    Set,
    This,
    Super
>;

// Using std::visit to process expressions
struct AstPrinter {
    std::string operator()(const Literal& lit) {
        // Handle literal
        return "literal";
    }
    
    std::string operator()(const Binary& bin) {
        // Handle binary
        return "(+ left right)";
    }
    
    // ... one operator() for each type
};

// Usage
Expr expr = Literal{42.0};
std::string result = std::visit(AstPrinter{}, expr);
```

### Why We Chose Visitor Pattern Instead

| Consideration | Visitor Pattern | std::variant |
|---------------|----------------|--------------|
| **Language Version** | C++11+ | C++17+ (not always available) |
| **Recursive Types** | Easy (shared_ptr) | Complex (needs recursive_wrapper) |
| **Open/Closed Principle** | ✅ Easy to add operations | ❌ Hard to add new types |
| **Type Safety** | Compile-time (virtual) | ✅ Compile-time (variant) |
| **Performance** | Virtual call overhead | ✅ No virtual calls |
| **Memory Layout** | Heap-allocated objects | ✅ Stack-friendly |
| **Learning Curve** | Classic OOP pattern | Modern C++17 feature |
| **Code Organization** | Inheritance hierarchy | Flat struct definitions |
| **Matches Java Original** | ✅ Yes | ❌ No |

### The Recursive Type Problem with std::variant

**Problem:** Expressions are recursive (Binary contains Exprs), but std::variant can't directly reference itself:

```cpp
// ❌ DOESN'T COMPILE - incomplete type
struct Binary {
    std::unique_ptr<Expr> left;   // Error: Expr not complete yet!
    Token op;
    std::unique_ptr<Expr> right;
};

using Expr = std::variant<Binary, Unary, ...>;  // Expr defined here
```

**Solution:** Use `std::variant` with forward declarations and wrapper types:

```cpp
// Forward declare
struct Literal;
struct Binary;
struct Unary;

// Define the variant first
using Expr = std::variant<
    std::unique_ptr<Literal>,
    std::unique_ptr<Binary>,
    std::unique_ptr<Unary>
>;

// Now define the structs
struct Binary {
    Expr left;   // Now Expr is complete
    Token op;
    Expr right;
};
```

**But this is complex and verbose!**

### Code Size Comparison

**Visitor Pattern (Our Implementation):**
```cpp
// exp.h - ~160 lines
class Expr {
    virtual std::any accept(ExprVisitor& visitor) = 0;
};

class Binary : public Expr {
    std::any accept(ExprVisitor& visitor) override {
        return visitor.visitBinaryExpr(this);
    }
};

// Clear inheritance hierarchy, easy to understand
```

**std::variant Approach:**
```cpp
// Need recursive wrappers, box types, and complex template magic
template<typename... Ts>
struct Overload : Ts... { using Ts::operator()...; };
template<typename... Ts> Overload(Ts...) -> Overload<Ts...>;

// Need to wrap recursive types
struct ExprBox;
using Expr = std::variant<
    Literal,
    std::unique_ptr<ExprBox>,  // For recursive types
    // ... complexity increases
>;

// More complex, harder to learn
```

---

## Visitor Pattern vs std::variant Comparison

### Complete Example Comparison

#### Visitor Pattern (What We Use)

```cpp
// ===== exp.h =====
class ExprVisitor {
public:
    virtual std::any visitBinaryExpr(Binary* expr) = 0;
    virtual std::any visitLiteralExpr(Literal* expr) = 0;
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::any accept(ExprVisitor& visitor) = 0;
};

class Binary : public Expr {
public:
    std::shared_ptr<Expr> left;
    Token op;
    std::shared_ptr<Expr> right;
    
    std::any accept(ExprVisitor& visitor) override {
        return visitor.visitBinaryExpr(this);
    }
};

class Literal : public Expr {
public:
    std::any value;
    
    std::any accept(ExprVisitor& visitor) override {
        return visitor.visitLiteralExpr(this);
    }
};

// ===== interpreter.cpp =====
class Interpreter : public ExprVisitor {
public:
    std::any visitBinaryExpr(Binary* expr) override {
        auto left = evaluate(expr->left.get());
        auto right = evaluate(expr->right.get());
        // ... compute result
        return result;
    }
    
    std::any visitLiteralExpr(Literal* expr) override {
        return expr->value;
    }
    
private:
    std::any evaluate(Expr* expr) {
        return expr->accept(*this);
    }
};

// Usage
auto expr = std::make_shared<Binary>(...);
Interpreter interpreter;
std::any result = expr->accept(interpreter);
```

#### std::variant Approach

```cpp
// ===== exp.h =====
struct Literal;
struct Binary;

// Forward-declare variant
using ExprPtr = std::unique_ptr<
    std::variant<Literal, Binary>
>;

struct Literal {
    std::any value;
};

struct Binary {
    ExprPtr left;
    Token op;
    ExprPtr right;
};

// Create the actual variant type
using Expr = std::variant<Literal, Binary>;

// ===== interpreter.cpp =====
struct Interpreter {
    std::any operator()(const Literal& lit) {
        return lit.value;
    }
    
    std::any operator()(const Binary& bin) {
        auto left = std::visit(*this, *bin.left);
        auto right = std::visit(*this, *bin.right);
        // ... compute result
        return result;
    }
};

// Usage
ExprPtr expr = std::make_unique<Expr>(Binary{...});
std::any result = std::visit(Interpreter{}, *expr);
```

### Adding New Operations

**Visitor Pattern: EASY**
```cpp
// Just add a new visitor class - no changes to exp.h!
class TypeChecker : public ExprVisitor {
    std::any visitBinaryExpr(Binary* expr) override { /* new logic */ }
    std::any visitLiteralExpr(Literal* expr) override { /* new logic */ }
};
```

**std::variant: EASY (same)**
```cpp
// Just add a new functor - no changes to exp.h!
struct TypeChecker {
    Type operator()(const Binary& bin) { /* new logic */ }
    Type operator()(const Literal& lit) { /* new logic */ }
};
```

### Adding New Expression Types

**Visitor Pattern: MODERATE**
```cpp
// 1. Add to exp.h
class Call : public Expr { ... };

// 2. Update ExprVisitor interface (forces all visitors to update)
class ExprVisitor {
    virtual std::any visitCallExpr(Call* expr) = 0;  // Add this
};

// 3. Update ALL existing visitors (compiler errors force you)
class Interpreter : public ExprVisitor {
    std::any visitCallExpr(Call* expr) override { /* implement */ }
};
```

**std::variant: HARD**
```cpp
// 1. Add to variant type
using Expr = std::variant<
    Literal,
    Binary,
    Call  // Add this
>;

// 2. Update ALL visitors manually (no compiler error if you forget!)
struct Interpreter {
    std::any operator()(const Call& call) { /* implement */ }
    // If you forget this, you get runtime error, not compile error
};
```

---

## When to Use Each

### Use Visitor Pattern When:

✅ **Types are relatively fixed, operations are added frequently**
- Compilers: Expression types are stable, but you add optimizations, code generators, analyzers
- Interpreters: Same AST, different execution modes
- Game engines: Entity types are stable, behaviors change

✅ **Following established patterns (like the original Java Lox)**
- Learning material matches your implementation
- Team familiar with OOP patterns
- Migrating from Java/C# code

✅ **Need runtime polymorphism**
- Working with base class pointers: `Expr*`
- Dynamic dispatch based on actual type
- Plugin systems where types unknown at compile time

✅ **C++11/14 support required**
- Older codebases
- Embedded systems with older compilers
- Maximum compatibility

### Use std::variant When:

✅ **Types are added frequently, operations are fixed**
- Configuration systems: New config types, same processing
- Event systems: New event types, same dispatch
- Data serialization: New data types, same serialization code

✅ **Performance is critical**
- No virtual call overhead
- Better cache locality (stack allocation)
- Compiler can optimize across all branches

✅ **Type safety over flexibility**
- Compile-time exhaustiveness checking
- No runtime type errors
- Pattern matching style

✅ **Modern C++17+ codebase**
- Already using modern C++ features
- Team comfortable with std::visit and variants
- Functional programming style preferred

---

## Cyclic Dependency Problem

### What is Cyclic Dependency?

**Cyclic dependency** (or circular dependency) occurs when two or more modules depend on each other directly or indirectly:

```
File A includes File B
File B includes File A
    ↓
Compilation Error!
```

### Example: The Problem

```cpp
// ===== Parser.h =====
#pragma once
#include "exp.h"  // Parser needs Expr types

class Parser {
    std::shared_ptr<Expr> parse();
    std::shared_ptr<Expr> primary();
};

// ===== exp.h =====
#pragma once
#include "Token.h"  // Expr needs Token
#include "Parser.h"  // ❌ CYCLIC! (if Expr needed Parser)

class Expr {
    // ...
};

// ===== Compilation =====
// Compiler reads Parser.h
//   → includes exp.h
//     → includes Parser.h  (already being read!)
//       → ERROR: Parser incomplete
```

### Common Causes in Our Codebase

```
Parser.h
   ↓ includes
exp.h
   ↓ includes
Token.h
   ↓ includes
TokenType.h
```

**This is OK** - it's a **dependency chain**, not a cycle:
```
Parser → exp → Token → TokenType
(no backward arrows)
```

**Potential cycle:**
```
Parser.h includes exp.h
exp.h includes Parser.h    ← BAD!
```

---

## How We Avoided Cyclic Dependencies

### Strategy 1: One-Way Dependency Flow (Layered Architecture)

Our design follows strict layers:

```
┌────────────────────────────────────┐
│  Layer 4: User Code                │  parser_test.cpp, main.cpp
├────────────────────────────────────┤
│  Layer 3: Parser                   │  Parser.h/cpp
│          (depends on ↓)            │
├────────────────────────────────────┤
│  Layer 2: AST & Visitor            │  exp.h
│          (depends on ↓)            │
├────────────────────────────────────┤
│  Layer 1: Scanner                  │  Scanner.h/cpp, Token.h
│          (depends on ↓)            │
├────────────────────────────────────┤
│  Layer 0: TokenType                │  TokenType.h/cpp
│          (no dependencies)         │
└────────────────────────────────────┘

Rule: Each layer only includes layers below it, never above!
```

**Why this works:**
- Parser includes exp.h ✅ (3 → 2)
- exp.h includes Token.h ✅ (2 → 1)
- Token.h includes TokenType.h ✅ (1 → 0)
- exp.h NEVER includes Parser.h ✅ (no 2 → 3)
- No cycles possible!

### Strategy 2: Forward Declarations

When you only need a pointer or reference, **forward declare** instead of including:

```cpp
// ===== Parser.h =====
#pragma once

// Forward declarations - tell compiler these exist
class Expr;
class Binary;
class Literal;

// Now we can use pointers/references without including exp.h
class Parser {
private:
    std::shared_ptr<Expr> parse();  // OK: only pointer
    std::vector<std::shared_ptr<Expr>> expressions;  // OK: pointer
    
    // Would need full definition:
    // Expr myExpr;  ❌ Error: incomplete type
};
```

**Rule:** Forward declare when you only need:
- Pointers: `Expr*`, `std::shared_ptr<Expr>`
- References: `Expr&`
- Function declarations (not definitions)

**Must include full header when:**
- Creating objects: `Expr obj;`
- Accessing members: `expr.value`
- Using sizeof: `sizeof(Expr)`
- Inheritance: `class Derived : public Base`

### Strategy 3: Separate Interface from Implementation

```cpp
// ===== Parser.h (INTERFACE) =====
#pragma once
#include <vector>
#include <memory>

// Forward declarations only
class Expr;
class Token;

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::shared_ptr<Expr> parse();  // Declaration only
    
private:
    std::vector<Token> tokens;
    // Private methods...
};

// ===== Parser.cpp (IMPLEMENTATION) =====
#include "Parser.h"
#include "exp.h"        // Now we can include full definition
#include "Token.h"      // Now we can include full definition

// Definitions can access full types
std::shared_ptr<Expr> Parser::parse() {
    // Can create Expr objects here
    return std::make_shared<Binary>(...);
}
```

**Benefits:**
- Header only has minimal includes (fast compilation)
- Implementation file has full includes (can use everything)
- Changes to exp.h only recompile Parser.cpp, not everything that includes Parser.h

### Strategy 4: Use Header Guards or #pragma once

**Prevent multiple inclusion:**

```cpp
// ===== Method 1: Header guards =====
#ifndef EXP_H
#define EXP_H

// ... content ...

#endif  // EXP_H

// ===== Method 2: #pragma once (simpler, we use this) =====
#pragma once

// ... content ...
```

**What this prevents:**
```cpp
// File A includes exp.h
// File B includes exp.h
// File C includes both A and B
// Without guards: exp.h included 2+ times → redefinition errors
// With guards: exp.h content only included once ✓
```

### Our Actual Dependency Graph

```
main.cpp ────────┐
parser_test.cpp ─┤
                 ├──→ Parser.h ──→ exp.h ──→ Token.h ──→ TokenType.h
                 └──→ Scanner.h ─────────────┘
```

**No cycles!** All arrows go left-to-right (or top-to-bottom), never backwards.

---

## Best Practices

### DO ✅

1. **Layer your architecture**
   ```cpp
   High Level → Mid Level → Low Level → Foundation
   (never go backwards)
   ```

2. **Forward declare in headers**
   ```cpp
   // In .h file
   class Token;  // Forward declaration
   
   // In .cpp file
   #include "Token.h"  // Full definition
   ```

3. **Use #pragma once**
   ```cpp
   #pragma once  // Simpler than guards
   ```

4. **Include what you use**
   ```cpp
   // If you use std::vector, include it
   #include <vector>
   ```

5. **Minimize header includes**
   ```cpp
   // .h: Only what's needed for declaration
   // .cpp: Everything needed for implementation
   ```

### DON'T ❌

1. **Don't create circular includes**
   ```cpp
   // A.h includes B.h
   // B.h includes A.h  ← BAD!
   ```

2. **Don't include in headers when forward declaration works**
   ```cpp
   // Bad:
   #include "HugeFile.h"  // Just for a pointer
   
   // Good:
   class HugeClass;  // Forward declare
   ```

3. **Don't use `using namespace` in headers**
   ```cpp
   // Bad in .h:
   using namespace std;  // Pollutes all including files
   
   // OK in .cpp:
   using namespace std;  // Only affects this file
   ```

4. **Don't make everything depend on everything**
   ```cpp
   // Bad: God header
   #include "everything.h"  // Includes entire codebase
   
   // Good: Specific includes
   #include "Parser.h"
   #include "Scanner.h"
   ```

### Diagnosing Cyclic Dependencies

**Symptoms:**
```
error: invalid use of incomplete type 'class Foo'
error: forward declaration of 'class Foo'
error: recursive header inclusion
```

**Fix Process:**
1. Draw your dependency graph (what includes what)
2. Find the cycle (A → B → C → A)
3. Break the cycle:
   - Use forward declarations
   - Move code to .cpp files
   - Introduce an intermediate interface
   - Refactor to remove the dependency

**Example Fix:**
```cpp
// Before (cycle):
A.h includes B.h
B.h includes A.h

// After (broken):
A.h forward declares class B
A.cpp includes B.h (full definition)
B.h includes A.h (still OK, A.h doesn't include B.h anymore)
```

---

## Summary

### Why Visitor Pattern Over std::variant?

1. ✅ **Matches original Java Lox** (educational consistency)
2. ✅ **Classic OOP pattern** (widely understood)
3. ✅ **Easy recursive types** (no complex wrappers)
4. ✅ **Easy to add operations** (just add visitor class)
5. ✅ **C++11 compatible** (no C++17 requirement)
6. ⚠️ **Harder to add types** (must update all visitors)

### Why Not std::variant?

1. ❌ **Requires C++17**
2. ❌ **Complex with recursive types**
3. ❌ **Hard to add new types** (no compile-time checking)
4. ✅ **Better performance** (but not critical for interpreter)
5. ✅ **Better for value types** (our AST uses pointers)

### How We Avoided Cycles

1. ✅ **Layered architecture** (strict one-way dependencies)
2. ✅ **Forward declarations** (break include chains)
3. ✅ **Separate interface/implementation** (.h vs .cpp)
4. ✅ **Header guards** (#pragma once)

**Golden Rule:** If you can draw your includes as a DAG (Directed Acyclic Graph), you're good!

---

## Further Reading

- **Modern C++ Design**: Andrei Alexandrescu (advanced template patterns)
- **Large-Scale C++ Software Design**: John Lakos (managing dependencies)
- **std::variant documentation**: https://en.cppreference.com/w/cpp/utility/variant
- **Visitor Pattern**: Gang of Four Design Patterns book
