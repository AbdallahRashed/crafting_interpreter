# Lox Expression Parser - Architecture & Design Documentation

## Table of Contents
1. [Overall Architecture](#overall-architecture)
2. [Why This Structure?](#why-this-structure)
3. [Fixing Ambiguity with Precedence Climbing](#fixing-ambiguity-with-precedence-climbing)
4. [The Visitor Design Pattern](#the-visitor-design-pattern)
5. [Call Flow Architecture](#call-flow-architecture)
6. [Error Handling Strategy](#error-handling-strategy)
7. [Java vs C++ Implementation Differences](#java-vs-c-implementation-differences)

---

## Overall Architecture

### Component Diagram

```
┌─────────────┐
│   Source    │
│   Code      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Scanner   │ ──→ Tokens (vector)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Parser    │ ──→ Expression AST (shared_ptr<Expr>)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Visitor    │ ──→ Result (evaluation, printing, etc.)
│(Interpreter)│
└─────────────┘
```

### Module Breakdown

1. **TokenType Module** (`TokenType.h/cpp`)
   - Defines all token types as enum
   - Maps keywords to token types
   - Provides string conversion utilities

2. **Token Module** (`Token.h`)
   - Represents a lexical token
   - Contains: type, lexeme, literal value (std::any), line number

3. **Scanner Module** (`Scanner.h/cpp`)
   - Lexical analysis (tokenization)
   - Converts source code → tokens
   - Handles comments, strings, numbers, identifiers

4. **Expression AST** (`exp.h`)
   - 12 expression node types
   - Base Expr class with visitor support
   - Uses raw pointers for visitor, shared_ptr for ownership

5. **Parser Module** (`Parser.h/cpp`)
   - Syntax analysis (parsing)
   - Converts tokens → expression AST
   - Implements precedence climbing
   - Handles parse errors

6. **Visitor Pattern** (User-implemented)
   - ExprVisitor interface in `exp.h`
   - Example: AstPrinter in `parser_test.cpp`
   - Future: Interpreter, Code Generator, etc.

---

## Why This Structure?

### 1. Separation of Concerns

**Each module has ONE responsibility:**

```cpp
Scanner:    "How do I break text into meaningful tokens?"
Parser:     "How do I organize tokens into a tree structure?"
AST:        "How do I represent the program structure?"
Visitor:    "How do I process/transform the AST?"
```

**Benefits:**
- Easy to understand each component in isolation
- Can test each module independently
- Can replace/modify one without affecting others
- Follows Single Responsibility Principle (SOLID)

### 2. Layered Architecture

Each layer only knows about the layer directly below it:

```
Visitor/Interpreter  ─depends on→  AST (exp.h)
        ↑
    Parser           ─depends on→  Token, AST
        ↑
    Scanner          ─depends on→  Token, TokenType
        ↑
    TokenType        ─depends on→  (nothing, base layer)
```

**Why this matters:**
- Scanner doesn't need to know about expressions
- Parser doesn't need to know what we'll DO with the AST
- Visitor doesn't need to know how tokens were scanned

### 3. Forward-Looking Design

This structure supports future features:
- **Statements**: Add Stmt classes alongside Expr
- **Type Checking**: Add TypeChecker visitor
- **Optimization**: Add Optimizer visitor
- **Code Generation**: Add CodeGen visitor
- **Multiple Backends**: Different visitors for different targets

---

## Fixing Ambiguity with Precedence Climbing

### The Problem: Ambiguous Grammars

Consider this expression:
```
1 + 2 * 3
```

Without precedence rules, this grammar is ambiguous:

```
     +                  *
   /   \     OR      /     \
  1     *           +       3
       / \         / \
      2   3       1   2
```

Result: `7` or `9`? **We need to enforce precedence!**

### The Solution: Recursive Descent with Precedence Levels

Our parser uses **precedence climbing** (also called recursive descent parsing):

```cpp
// Lowest precedence (evaluated last, appears high in tree)
expression()     → assignment()
assignment()     → logicalOr()
logicalOr()      → logicalAnd()      // or
logicalAnd()     → equality()        // and
equality()       → comparison()      // == !=
comparison()     → term()            // < <= > >=
term()           → factor()          // + -
factor()         → unary()           // * /
unary()          → call()            // ! - (unary)
call()           → primary()         // () . (highest precedence)
primary()        → literals, variables, grouping
// Highest precedence (evaluated first, appears low in tree)
```

### How It Works

**Example: Parsing `1 + 2 * 3`**

```
parse()
  └─ expression()
      └─ assignment()
          └─ logicalOr()
              └─ logicalAnd()
                  └─ equality()
                      └─ comparison()
                          └─ term()              ← Handles +
                              ├─ factor()         ← Handles *
                              │   ├─ unary()
                              │   │   └─ call()
                              │   │       └─ primary() → 1
                              │   └─ [sees *, loops]
                              │       └─ unary()
                              │           └─ call()
                              │               └─ primary() → 2
                              │       └─ unary()
                              │           └─ call()
                              │               └─ primary() → 3
                              └─ [returns Binary(*, 2, 3)]
                          └─ [sees +, creates Binary(+, 1, (2*3))]
```

**Result AST:**
```
      +
     / \
    1   *
       / \
      2   3
```

**The Key Insight:**

1. **Lower precedence** operators are parsed at **higher levels** (term handles +)
2. **Higher precedence** operators are parsed at **lower levels** (factor handles *)
3. When factor() is called, it **fully consumes** all * and / operations before returning
4. Then term() only sees + or -, which it handles
5. This naturally creates the correct tree structure

### Associativity

Our operators are **left-associative**:

```cpp
while (match({PLUS, MINUS})) {  // Loop = left-associative
    Token op = previous();
    auto right = factor();
    expr = new Binary(expr, op, right);  // Build left to right
}
```

For `1 - 2 - 3`:
```
      -
     / \
    -   3
   / \
  1   2
```
Result: `(1 - 2) - 3 = -4` ✓ (not `1 - (2 - 3) = 2`)

---

## The Visitor Design Pattern

### What Problem Does It Solve?

**Problem**: We have many expression types, and we want to perform different operations on them (print, evaluate, optimize, type-check, etc.) **without modifying the expression classes themselves**.

### Bad Approach (Without Visitor)

```cpp
// Bad: Operations mixed into AST classes
class Binary {
    double evaluate();
    std::string print();
    Type typeCheck();
    Bytecode compile();
    // Adding new operation = modify EVERY class
};

class Unary {
    double evaluate();
    std::string print();
    Type typeCheck();
    Bytecode compile();
    // Same methods in every class!
};
// ... 12 classes with duplicate code
```

**Problems:**
- ❌ Violates Single Responsibility (AST should just represent structure)
- ❌ Hard to add new operations (modify 12 classes)
- ❌ Can't add operations without modifying source code
- ❌ Operations are scattered across classes

### Good Approach (With Visitor)

```cpp
// Good: Operations are separate visitors
class ExprVisitor {
    virtual std::any visitBinaryExpr(Binary* expr) = 0;
    virtual std::any visitUnaryExpr(Unary* expr) = 0;
    // ... one method per expression type
};

class Interpreter : public ExprVisitor {
    std::any visitBinaryExpr(Binary* expr) override {
        // Evaluation logic here
    }
};

class AstPrinter : public ExprVisitor {
    std::any visitBinaryExpr(Binary* expr) override {
        // Printing logic here
    }
};
```

**Benefits:**
- ✅ AST classes are simple and focused
- ✅ Easy to add new operations (just add new visitor class)
- ✅ Related logic is grouped together
- ✅ Follows Open/Closed Principle (open for extension, closed for modification)

### The Expression Problem

The Visitor pattern is a solution to the **Expression Problem**:

```
               Easy to add new...
               ┌─────────────┬──────────────┐
               │  Operations │  Data Types  │
┌──────────────┼─────────────┼──────────────┤
│ OOP Style    │     ❌      │      ✅      │
│ (methods)    │             │              │
├──────────────┼─────────────┼──────────────┤
│ Functional   │     ✅      │      ❌      │
│ (functions)  │             │              │
├──────────────┼─────────────┼──────────────┤
│ **VISITOR**  │     ✅      │      ⚠️      │
│ (pattern)    │  (easy!)    │  (harder)    │
└──────────────┴─────────────┴──────────────┘
```

For compilers/interpreters:
- Expression types are relatively **fixed** (grammar is stable)
- Operations are **frequently added** (optimize, interpret, compile, type-check, etc.)
- **Visitor is perfect for this use case!**

---

## How We Applied the Visitor Pattern (C++ vs Java)

### Java Implementation (Original)

```java
abstract class Expr {
    interface Visitor<R> {
        R visitBinaryExpr(Binary expr);
        R visitUnaryExpr(Unary expr);
    }
    
    abstract <R> R accept(Visitor<R> visitor);
}

static class Binary extends Expr {
    @Override
    <R> R accept(Visitor<R> visitor) {
        return visitor.visitBinaryExpr(this);  // Pass 'this'
    }
}
```

**Java Features Used:**
- Nested interfaces
- Generics with type parameters `<R>`
- Automatic garbage collection (no memory management)
- `this` always works

### C++ Implementation (Our Version)

```cpp
// 1. Visitor interface (NOT nested, C++ limitation)
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual std::any visitBinaryExpr(Binary* expr) = 0;  // Raw pointer!
    virtual std::any visitUnaryExpr(Unary* expr) = 0;
};

// 2. Base Expr class
class Expr {
public:
    virtual ~Expr() = default;
    virtual std::any accept(ExprVisitor& visitor) = 0;
};

// 3. Concrete expression
class Binary : public Expr {
public:
    const std::shared_ptr<Expr> left;   // Children owned
    const Token op;
    const std::shared_ptr<Expr> right;  // Children owned
    
    std::any accept(ExprVisitor& visitor) override {
        return visitor.visitBinaryExpr(this);  // Pass raw 'this'
    }
};
```

### Key Differences

| Aspect | Java | C++ (Our Version) |
|--------|------|-------------------|
| **Visitor Interface** | Nested in Expr class | Separate top-level class |
| **Type Parameter** | Generic `<R>` | `std::any` (type-erased) |
| **Passing to Visitor** | `this` (managed reference) | `this` (raw pointer) |
| **Child Storage** | References (GC) | `shared_ptr<Expr>` (RAII) |
| **Memory Model** | Garbage collected | Manual (smart pointers) |

### Why Raw Pointers in Visitor?

**Original attempt (problematic):**
```cpp
class Expr : public std::enable_shared_from_this<Expr> {
    std::any accept(ExprVisitor& visitor) {
        return visitor.visitBinaryExpr(
            std::static_pointer_cast<Binary>(shared_from_this())
        );
    }
};
```

**Problem**: 
- `shared_from_this()` throws if object isn't already in a `shared_ptr`
- Requires object to be heap-allocated via `make_shared` before use
- Fragile and error-prone

**Our solution (clean):**
```cpp
class Expr {  // No enable_shared_from_this
    std::any accept(ExprVisitor& visitor) {
        return visitor.visitBinaryExpr(this);  // Just raw pointer
    }
};
```

**Why this works:**
- Visitor doesn't own the expression (just observes/processes)
- Lifetime managed externally by Parser's `shared_ptr<Expr>`
- Simple, safe, idiomatic C++
- Visitor can't accidentally delete or extend lifetime

---

## Call Flow Architecture

### Complete Flow: Source Code → Result

```
┌──────────────────────────────────────────────────────────┐
│ 1. User Program / REPL                                   │
└───────────────────┬──────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 2. SCANNER                                              │
│    Scanner scanner(source);                             │
│    vector<Token> tokens = scanner.scanTokens();         │
│                                                          │
│    source: "1 + 2 * 3"                                  │
│    ↓                                                     │
│    tokens: [NUMBER(1), PLUS, NUMBER(2), STAR,          │
│             NUMBER(3), EOF]                             │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 3. PARSER                                               │
│    Parser parser(tokens);                               │
│    shared_ptr<Expr> ast = parser.parse();               │
│                                                          │
│    Calls chain (for "1 + 2 * 3"):                      │
│    parse()                                              │
│      → expression()                                      │
│         → assignment()                                   │
│            → logicalOr()                                 │
│               → logicalAnd()                             │
│                  → equality()                            │
│                     → comparison()                       │
│                        → term()          ← PLUS         │
│                           → factor()     ← STAR         │
│                              → unary()                   │
│                                 → call()                 │
│                                    → primary()           │
│                                                          │
│    Result AST:                                          │
│         Binary(+)                                       │
│         /        \                                      │
│    Literal(1)   Binary(*)                              │
│                 /        \                              │
│            Literal(2)  Literal(3)                      │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 4. VISITOR (Example: AstPrinter)                        │
│    AstPrinter printer;                                  │
│    string result = printer.print(ast.get());           │
│                                                          │
│    Call flow (depth-first traversal):                   │
│    printer.print(Binary+)                               │
│      → Binary.accept(printer)                           │
│         → printer.visitBinaryExpr(Binary+)              │
│            → printer.print(left)    [Literal 1]        │
│            → printer.print(right)   [Binary *]         │
│               → Binary.accept(printer)                  │
│                  → printer.visitBinaryExpr(Binary*)     │
│                     → print(left)   [Literal 2]        │
│                     → print(right)  [Literal 3]        │
│                  ← "(* 2 3)"                            │
│            ← "(+ 1 (* 2 3))"                            │
│                                                          │
│    Result: "(+ 1.000000 (* 2.000000 3.000000))"       │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 5. OUTPUT                                               │
│    Console, File, Next Stage (Interpreter), etc.        │
└─────────────────────────────────────────────────────────┘
```

### Detailed Call Sequence for `term()`

When parsing `1 + 2 * 3`, here's what happens in `term()`:

```cpp
Expr* Parser::term() {
    Expr* expr = factor();  // Step 1: Get left operand
    // At this point: expr = factor() consumed "1", current is at "+"
    
    while (match({PLUS, MINUS})) {  // Step 2: Check for + or -
        // Matched PLUS, advance to "2"
        Token op = previous();  // op = PLUS token
        
        Expr* right = factor();  // Step 3: Get right operand
        // factor() will fully consume "2 * 3":
        //   factor() sees "2", then sees "*"
        //   factor() loops: 2 * 3, returns Binary(*, 2, 3)
        // right = Binary(*, 2, 3)
        
        expr = new Binary(expr, op, right);  // Step 4: Build tree
        // expr = Binary(+, Literal(1), Binary(*, 2, 3))
    }
    
    return expr;  // Final tree with correct precedence
}
```

**Why this enforces precedence:**
- `factor()` is called from inside `term()`
- `factor()` handles `*` and `/` **completely** before returning
- `term()` only sees `+` and `-` after all higher-precedence ops are done
- Result: `*` is deeper in the tree (evaluated first)

---

## Error Handling Strategy

### Multi-Level Error Handling

Our parser uses a **three-phase** error handling strategy:

```
┌────────────────────────────────────────────────────────┐
│ Phase 1: DETECTION                                     │
│ ────────────────────────────────────────────────────── │
│ • consume() method fails to find expected token        │
│ • primary() encounters unexpected token                │
│ • Semantic checks fail (e.g., too many args)          │
└──────────────────┬─────────────────────────────────────┘
                   │ throw ParseError
                   ▼
┌────────────────────────────────────────────────────────┐
│ Phase 2: REPORTING                                     │
│ ────────────────────────────────────────────────────── │
│ • error() method formats and prints message            │
│ • Includes line number and context                     │
│ • Returns ParseError exception                         │
│                                                         │
│ Format: "[Line X] Error at 'token': message"          │
└──────────────────┬─────────────────────────────────────┘
                   │ exception propagates
                   ▼
┌────────────────────────────────────────────────────────┐
│ Phase 3: RECOVERY                                      │
│ ────────────────────────────────────────────────────── │
│ • parse() catches ParseError                           │
│ • Returns nullptr (for single expression)              │
│ • synchronize() advances to safe point (for stmts)    │
└────────────────────────────────────────────────────────┘
```

### 1. Error Detection

**Method 1: consume() - Expected Token Missing**

```cpp
Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();  // Happy path
    
    throw error(peek(), message);  // Error path
}
```

Usage:
```cpp
consume(RIGHT_PAREN, "Expect ')' after expression.");
//      ^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//      expected      error message if not found
```

**Method 2: primary() - Unexpected Token**

```cpp
Expr* Parser::primary() {
    if (match(FALSE)) return new Literal(false);
    if (match(TRUE)) return new Literal(true);
    // ... other cases ...
    
    throw error(peek(), "Expect expression.");
    //    ^^^^^^^^^^^^^  No valid expression found
}
```

**Method 3: Semantic Checks**

```cpp
if (arguments.size() >= 255) {
    error(peek(), "Can't have more than 255 arguments.");
    // Note: doesn't throw, just reports warning
}
```

### 2. Error Reporting

The `error()` method formats and displays errors:

```cpp
ParseError Parser::error(const Token& token, const std::string& message) {
    if (token.type == TOKEN_EOF) {
        cerr << "[Line " << token.line << "] Error at end: " 
             << message << endl;
    } else {
        cerr << "[Line " << token.line << "] Error at '" 
             << token.lexeme << "': " << message << endl;
    }
    return ParseError(message);  // Create and return exception
}
```

**Examples:**
```
Input: "(1 + 2"
Output: [Line 1] Error at end: Expect ')' after expression.

Input: "1 +"
Output: [Line 1] Error at end: Expect expression.

Input: "foo @ bar"
Output: [Line 1] Error at '@': Expect expression.
```

### 3. Error Recovery

**For Single Expressions (Current Implementation):**

```cpp
shared_ptr<Expr> Parser::parse() {
    try {
        return expression();  // Try to parse
    } catch (const ParseError& error) {
        // Error already reported by error() method
        return nullptr;  // Signal failure
    }
}
```

**For Multiple Statements (Future):**

```cpp
void Parser::synchronize() {
    advance();  // Skip bad token
    
    while (!isAtEnd()) {
        // Found statement boundary
        if (previous().type == SEMICOLON) return;
        
        // Found statement keyword
        switch (peek().type) {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
                return;  // Safe to continue
        }
        
        advance();  // Keep scanning
    }
}
```

**Why synchronize()?**
- When parsing multiple statements, one error shouldn't stop everything
- Advance to a "safe" boundary (semicolon or statement keyword)
- Continue parsing next statement
- Collect multiple errors in one pass

### Error Handling Comparison

| Language | Error Type | Detection | Reporting | Recovery |
|----------|------------|-----------|-----------|----------|
| **Java** | ParseError (RuntimeException) | consume() throws | Lox.error() | synchronize() |
| **C++**  | ParseError (runtime_error) | consume() throws | Parser::error() | synchronize() |

**Both implementations:**
- ✅ Use exceptions for control flow
- ✅ Report errors with line numbers
- ✅ Continue after errors (in statement mode)
- ✅ Don't crash on syntax errors

### Error Handling Philosophy

**Fail Fast, Report Well, Recover Gracefully**

1. **Fail Fast**: Detect errors immediately when encountered
   ```cpp
   throw error(peek(), "Expect expression.");
   ```

2. **Report Well**: Give users helpful, specific error messages
   ```cpp
   "[Line 5] Error at '}': Expect expression."
   ```

3. **Recover Gracefully**: Continue parsing to find more errors
   ```cpp
   synchronize();  // Find next statement
   continue;       // Keep going
   ```

---

## Java vs C++ Implementation Differences

### Summary Table

| Feature | Java Implementation | C++ Implementation |
|---------|---------------------|-------------------|
| **Memory Management** | Garbage Collection | Smart Pointers (shared_ptr) |
| **Token Storage** | List\<Token\> | vector\<Token\> |
| **AST Storage** | Expr references | shared_ptr\<Expr\> |
| **Visitor Passing** | Expr (object) | Expr* (raw pointer) |
| **Generics** | \<R\> type parameter | std::any (type erasure) |
| **Literal Values** | Object | std::any |
| **Nested Classes** | static class inside Expr | Separate top-level classes |
| **Interface** | interface Visitor\<R\> | abstract class ExprVisitor |
| **Inheritance** | extends | : public |
| **Exceptions** | RuntimeException | std::runtime_error |
| **String Concat** | "str" + var | std::to_string() |

### Detailed Differences

#### 1. Memory Management

**Java:**
```java
Expr left = new Binary(left, op, right);
// No need to worry about deletion
// GC handles it automatically
```

**C++:**
```cpp
shared_ptr<Expr> left = make_shared<Binary>(left, op, right);
// Reference counted
// Automatically deleted when last shared_ptr goes away
// Must use shared_ptr to prevent memory leaks
```

#### 2. Type System

**Java:**
```java
interface Visitor<R> {
    R visitBinaryExpr(Binary expr);  // Generic type R
}

class AstPrinter implements Visitor<String> {
    String visitBinaryExpr(Binary expr) { ... }  // R = String
}
```

**C++:**
```cpp
class ExprVisitor {
    virtual std::any visitBinaryExpr(Binary* expr) = 0;  // Always std::any
};

class AstPrinter : public ExprVisitor {
    std::any visitBinaryExpr(Binary* expr) override {
        return std::string("...");  // Wrap in std::any
    }
};
```

**Why std::any?**
- C++ doesn't support virtual template methods
- `std::any` can hold any type (like Object in Java)
- Caller must `any_cast` to extract value

#### 3. Ownership Model

**Java (Implicit):**
```java
class Binary extends Expr {
    final Expr left;    // Just a reference
    final Expr right;   // GC tracks ownership
}
```

**C++ (Explicit):**
```cpp
class Binary : public Expr {
    const std::shared_ptr<Expr> left;   // Explicit shared ownership
    const std::shared_ptr<Expr> right;  // Won't delete while in use
    
    // Visitor sees raw pointer (doesn't own)
    std::any accept(ExprVisitor& visitor) {
        return visitor.visitBinaryExpr(this);  // 'this' is raw pointer
    }
};
```

**Key insight:**
- Children (left/right) use `shared_ptr` (owned)
- Visitor uses raw pointer (not owned, just visiting)
- This separates ownership from visitation

#### 4. Code Organization

**Java (Nested):**
```java
abstract class Expr {
    interface Visitor<R> { ... }
    
    static class Binary extends Expr { ... }
    static class Unary extends Expr { ... }
    // All in one file
}
```

**C++ (Separate):**
```cpp
// exp.h
class ExprVisitor { ... };    // Can't nest in C++
class Expr { ... };
class Binary : public Expr { ... };
class Unary : public Expr { ... };
```

**Why separate in C++?**
- C++ doesn't have Java's nested class features
- Separate classes are more common in C++
- Header files organize public interfaces

---

## Conclusion

### Why This Architecture Works

1. **Modular**: Each component has clear boundaries
2. **Extensible**: Add new visitors without changing AST
3. **Correct**: Precedence climbing ensures proper parse trees
4. **Robust**: Error handling catches and reports problems
5. **Portable**: Core concepts work in Java, C++, Python, etc.

### Key Takeaways

- **Scanner → Parser → AST → Visitor** is the standard compiler pipeline
- **Precedence climbing** naturally creates correct expression trees
- **Visitor pattern** separates data structure from operations
- **Error handling** uses exceptions for flow control and recovery
- **C++ and Java** implement same patterns with different mechanics

This architecture is used by:
- **Clang**: C/C++ compiler (uses visitor for code gen)
- **Roslyn**: C# compiler (.NET, visitor-based)
- **V8**: JavaScript engine (visitor for optimization)
- **LLVM**: Compiler infrastructure (visitor passes)
- **Most modern compilers**: Follow this basic structure

The patterns you see here scale from toy interpreters to production compilers!
