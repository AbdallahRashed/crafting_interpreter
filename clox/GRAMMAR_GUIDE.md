# Lox Grammar and Expression Precedence Guide

## Why `expression()` Returns `assignment()`

The short answer: **Expression parsing uses precedence climbing**, where each parsing method calls the next higher precedence level.

```cpp
std::shared_ptr<Expr> Parser::expression() {
    return assignment();  // Start at LOWEST precedence
}
```

This creates a **chain of method calls** that enforces operator precedence from lowest to highest.

## How Precedence Climbing Works

### The Method Call Chain

```
expression()
    ↓
assignment()        // = (lowest precedence, right-associative)
    ↓
logicalOr()         // or
    ↓
logicalAnd()        // and
    ↓
equality()          // == !=
    ↓
comparison()        // > >= < <=
    ↓
term()              // + -
    ↓
factor()            // * /
    ↓
unary()             // ! - (prefix)
    ↓
call()              // () . (postfix)
    ↓
primary()           // literals, identifiers, (expr) (highest precedence)
```

### Example: Parsing `2 + 3 * 4`

Let's trace how this works:

```
expression()
  → assignment()
      → logicalOr()
          → logicalAnd()
              → equality()
                  → comparison()
                      → term()                // Sees "+"
                          → factor()           // Gets left: "2"
                              → unary()
                                  → call()
                                      → primary() → Literal(2)
                          // Matches PLUS
                          → factor()           // Gets right
                              → unary()
                                  → call()
                                      → primary() → Literal(3)
                              // Inside factor(), matches STAR
                              // Builds: 3 * 4 BEFORE returning
                              → unary()
                                  → call()
                                      → primary() → Literal(4)
                              // Returns: Binary(3, *, 4)
                          // Builds: Binary(2, +, Binary(3, *, 4))
```

**Result:** `(+ 2 (* 3 4))` - multiplication happens first!

### Why This Works

**Key insight:** Higher precedence operations are parsed DEEPER in the recursion, so they bind TIGHTER to their operands.

When `term()` sees `2 + 3 * 4`:
1. It parses `2` as left operand
2. Sees `+`, so enters while loop
3. Calls `factor()` to get right operand
4. **`factor()` sees `3 * 4` and builds it as ONE expression**
5. `term()` receives the complete `(3 * 4)` expression as right operand
6. Builds `2 + (3 * 4)`

## Complete Grammar Reference

### Statement Grammar

```
program        → declaration* EOF ;

declaration    → classDecl 
               | funDecl 
               | varDecl 
               | statement ;

classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )? 
                 "{" function* "}" ;

funDecl        → "fun" function ;

varDecl        → "var" IDENTIFIER ( "=" expression )? ";" ;

function       → IDENTIFIER "(" parameters? ")" block ;

parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
```

#### Statement Grammar (continued)

```
statement      → exprStmt
               | forStmt
               | ifStmt
               | printStmt
               | returnStmt
               | whileStmt
               | block ;

exprStmt       → expression ";" ;

forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
                 expression? ";"
                 expression? ")" statement ;

ifStmt         → "if" "(" expression ")" statement
                 ( "else" statement )? ;

printStmt      → "print" expression ";" ;

returnStmt     → "return" expression? ";" ;

whileStmt      → "while" "(" expression ")" statement ;

block          → "{" declaration* "}" ;
```

### Expression Grammar (Precedence Order)

```
expression     → assignment ;

assignment     → ( call "." )? IDENTIFIER "=" assignment
               | logic_or ;

logic_or       → logic_and ( "or" logic_and )* ;

logic_and      → equality ( "and" equality )* ;

equality       → comparison ( ( "!=" | "==" ) comparison )* ;

comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;

term           → factor ( ( "-" | "+" ) factor )* ;

factor         → unary ( ( "/" | "*" ) unary )* ;

unary          → ( "!" | "-" ) unary
               | call ;

call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;

primary        → "true" | "false" | "nil" | "this"
               | NUMBER | STRING | IDENTIFIER | "(" expression ")"
               | "super" "." IDENTIFIER ;
```

## Operator Precedence Table

| Level | Operators | Associativity | Example | Result |
|-------|-----------|---------------|---------|--------|
| 1 (lowest) | `=` | Right | `a = b = 5` | `a = (b = 5)` |
| 2 | `or` | Left | `a or b or c` | `(a or b) or c` |
| 3 | `and` | Left | `a and b and c` | `(a and b) and c` |
| 4 | `==` `!=` | Left | `a == b == c` | `(a == b) == c` |
| 5 | `<` `<=` `>` `>=` | Left | `a < b < c` | `(a < b) < c` |
| 6 | `+` `-` | Left | `a - b + c` | `(a - b) + c` |
| 7 | `*` `/` | Left | `a / b * c` | `(a / b) * c` |
| 8 | `!` `-` (unary) | Right | `!!x` | `!(!x)` |
| 9 (highest) | `()` `.` | Left | `a.b().c` | `((a.b)()).c` |

## Detailed Examples

### Example 1: Arithmetic Precedence

**Source:** `1 + 2 * 3 + 4`

**Parse tree:**
```
       +
      / \
     +   4
    / \
   1   *
      / \
     2   3
```

**Evaluation order:**
1. `2 * 3` = 6
2. `1 + 6` = 7
3. `7 + 4` = 11

**Code trace:**
```cpp
expression()
  → assignment()
    → logicalOr()
      → ... (skipping to term)
      → term()
          left = factor()  // Returns 1
          // See PLUS
          right = factor()
              left = unary() // Returns 2
              // See STAR - while loop iterates!
              right = unary() // Returns 3
              // Returns Binary(2, *, 3)
          // Build Binary(1, +, Binary(2, *, 3))
          // See PLUS again - while loop continues!
          right = factor() // Returns 4
          // Returns Binary(Binary(1, +, Binary(2, *, 3)), +, 4)
```

### Example 2: Assignment Chains (Right Associative)

**Source:** `a = b = c = 5`

**Parse tree:**
```
    =
   / \
  a   =
     / \
    b   =
       / \
      c   5
```

**Why right-associative?**

In `assignment()`:
```cpp
std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();  // Parse left side
    
    if (match(EQUAL)) {
        Token equals = previous();
        std::shared_ptr<Expr> value = assignment();  // RECURSIVELY parse right!
        // ...
    }
    return expr;
}
```

The recursive call to `assignment()` on the RIGHT side makes it right-associative.

**Evaluation:** (right to left)
1. `c = 5` → 5
2. `b = 5` → 5
3. `a = 5` → 5

### Example 3: Logical Operators with Short-Circuit

**Source:** `a or b and c`

**Parse tree:**
```
    or
   /  \
  a   and
     /   \
    b     c
```

**Why?** `and` has higher precedence than `or`

**Code trace:**
```cpp
logicalOr() {
    expr = logicalAnd()  // This will consume "b and c"
        expr = equality() // Gets 'b'
        // Sees AND
        right = equality() // Gets 'c'
        // Returns Binary(b, and, c)
    // Now back in logicalOr, expr = Binary(b, and, c)
    // Sees OR at start
    right = logicalAnd()
        // Gets 'a'
    // Returns Binary(a, or, Binary(b, and, c))
}
```

Wait, that's wrong! Let me retrace:

```cpp
logicalOr() {
    expr = logicalAnd() // Gets 'a' (no 'and' after it)
    // Sees OR
    while (match(OR)) {
        right = logicalAnd()
            expr = equality() // Gets 'b'
            // Sees AND
            while (match(AND)) {
                right = equality() // Gets 'c'
                // Returns Binary(b, and, c)
            }
        // Returns Binary(b, and, c)
    }
    // Returns Binary(a, or, Binary(b, and, c))
}
```

**Evaluation with short-circuit:**
1. Evaluate `a`
2. If `a` is truthy, return `a` (don't evaluate `b and c`)
3. If `a` is falsy, evaluate `b and c`

### Example 4: Complex Expression

**Source:** `a.b(c + d, e).f = g * h`

**Parse tree:**
```
        =
       / \
      .   *
     / \ / \
    () f g h
   / \
  .  args
 / \   |
a  b   +
      / \
     c   d
     e
```

**Step-by-step parsing:**

1. **`assignment()` parses left side:**
   - Calls `logicalOr()` → ... → `call()`
   - `call()` starts with `primary()` → `a` (identifier)
   - Sees `.` → matches, gets `b`
   - Expression is now `a.b` (Get node)
   - Sees `(` → calls `finishCall()`
     - Parses arguments: `c + d` and `e`
     - Expression is now `a.b(c+d, e)` (Call node)
   - Sees `.` → matches, gets `f`
   - Expression is now `a.b(c+d, e).f` (Get node)
   - Returns this as `expr`

2. **`assignment()` sees `=`:**
   - Recursively parses right side: `g * h`
   - Creates Set node (property assignment)

### Example 5: Unary Operators

**Source:** `!-5`

**Parse tree:**
```
  !
  |
  -
  |
  5
```

**Code trace:**
```cpp
unary() {
    // Sees BANG
    if (match(BANG)) {
        op = previous()
        right = unary()  // RECURSIVELY call unary!
            // Sees MINUS
            if (match(MINUS)) {
                op = previous()
                right = unary()
                    // No unary operator
                    return call()
                        → primary() → 5
                // Returns Unary(-, 5)
            }
        // Returns Unary(!, Unary(-, 5))
    }
}
```

**Evaluation:**
1. `-5` = -5
2. `!(-5)` = false (non-zero is truthy, negation makes it false)

### Example 6: Method Chaining

**Source:** `obj.method1().method2(arg).prop`

**Parse tree:**
```
       .
      / \
    ()  prop
   / \
  .  args
 / \   |
() method2 arg
|
.
/ \
obj method1
   (empty args)
```

**Parsing in `call()`:**
```cpp
call() {
    expr = primary()  // Gets 'obj'
    
    while (true) {
        if (match(DOT)) {           // First dot
            name = "method1"
            expr = Get(obj, method1)
        }
        if (match(LEFT_PAREN)) {    // First call
            expr = Call(Get(obj, method1), [])
        }
        if (match(DOT)) {           // Second dot
            name = "method2"
            expr = Get(Call(...), method2)
        }
        if (match(LEFT_PAREN)) {    // Second call
            expr = Call(Get(Call(...), method2), [arg])
        }
        if (match(DOT)) {           // Third dot
            name = "prop"
            expr = Get(Call(...), prop)
        }
        // No more . or (, exit loop
    }
    return expr
}
```

## Why This Design?

### 1. **Precedence is Encoded in Structure**

The deeper in the call stack, the tighter the binding:
- `primary()` is deepest → binds tightest (literals, parentheses)
- `assignment()` is shallowest → binds loosest (lowest precedence)

### 2. **Associativity is Encoded in Logic**

**Left-associative (most operators):**
```cpp
std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();  // Get left operand
    
    while (match({MINUS, PLUS})) {          // Loop for multiple operators
        Token op = previous();
        std::shared_ptr<Expr> right = factor();  // Get right operand
        expr = std::make_shared<Binary>(expr, op, right);  // Rebuild expr
    }
    return expr;
}
```

For `a - b - c`:
1. First iteration: `expr = (a - b)`
2. Second iteration: `expr = ((a - b) - c)`

**Right-associative (assignment):**
```cpp
std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();
    
    if (match(EQUAL)) {  // Single if, not while!
        Token equals = previous();
        std::shared_ptr<Expr> value = assignment();  // RECURSIVE call
        // ...
    }
    return expr;
}
```

For `a = b = c`:
1. Parse `a` as left
2. Recursively parse `b = c` as right
3. Build `a = (b = c)`

### 3. **Each Method Has One Job**

- `term()`: Handle `+` and `-` (same precedence level)
- `factor()`: Handle `*` and `/` (same precedence level)
- `unary()`: Handle prefix `!` and `-`
- etc.

### 4. **Easy to Extend**

To add a new operator:
1. Decide its precedence
2. Add a method at that precedence level (or modify existing)
3. No need to change other methods!

## Common Patterns

### Pattern 1: Binary Operators (Left-Associative)

```cpp
std::shared_ptr<Expr> Parser::binaryOp() {
    std::shared_ptr<Expr> expr = nextHigherPrecedence();
    
    while (match({OP1, OP2, ...})) {
        Token op = previous();
        std::shared_ptr<Expr> right = nextHigherPrecedence();
        expr = std::make_shared<Binary>(expr, op, right);
    }
    
    return expr;
}
```

### Pattern 2: Prefix Operators (Right-Associative)

```cpp
std::shared_ptr<Expr> Parser::prefixOp() {
    if (match({OP1, OP2, ...})) {
        Token op = previous();
        std::shared_ptr<Expr> right = prefixOp();  // Recursive!
        return std::make_shared<Unary>(op, right);
    }
    
    return nextHigherPrecedence();
}
```

### Pattern 3: Postfix Operators

```cpp
std::shared_ptr<Expr> Parser::postfixOp() {
    std::shared_ptr<Expr> expr = nextHigherPrecedence();
    
    while (match({OP1, OP2, ...})) {
        // Build postfix expression
        expr = buildPostfixExpression(expr);
    }
    
    return expr;
}
```

## Summary

**Why `expression()` returns `assignment()`:**

It's the entry point to the precedence hierarchy. By calling `assignment()`, which calls `logicalOr()`, which calls `logicalAnd()`, etc., we ensure that:

1. **Higher precedence operators are parsed deeper** (bind tighter)
2. **Operators are evaluated in the correct order**
3. **Associativity is naturally enforced** by the parsing structure
4. **The code is clean and maintainable** (each method has one responsibility)

This is called **precedence climbing** or **Pratt parsing**, and it's a standard technique for parsing expressions in recursive descent parsers.

## Visual Summary

```
                    PRECEDENCE LEVELS
                          
    Lowest    ← expression() → assignment()
              ↓
              ← logicalOr()
              ↓
              ← logicalAnd()
              ↓
              ← equality()
              ↓
              ← comparison()
              ↓
              ← term()
              ↓
              ← factor()
              ↓
              ← unary()
              ↓
              ← call()
              ↓
    Highest   ← primary()
```

Each arrow down represents "calls" and "higher precedence than".
