# Statement System Design Documentation

## Overview

The Lox interpreter now supports a complete statement-based parsing system, moving beyond simple expression evaluation to full program execution with declarations, control flow, and functions.

## Architecture

### Two-Tier AST System

The interpreter uses two parallel AST hierarchies:

1. **Expressions (exp.h)** - Produce values
   - Binary, Unary, Literal, Variable, etc.
   - Used within statements (e.g., if condition, print value)
   - Follow precedence-climbing parsing

2. **Statements (Stmt.h)** - Perform actions
   - Block, If, While, Print, Var, Function, etc.
   - Top-level program structure
   - Can contain expressions and other statements

### Design Pattern: Visitor

Both hierarchies use the **Visitor Pattern** for traversal:

```cpp
// Define visitor interface
class StmtVisitor {
    virtual std::any visitBlockStmt(Block* stmt) = 0;
    virtual std::any visitIfStmt(If* stmt) = 0;
    // ... other visit methods
};

// Each statement accepts a visitor
class If : public Stmt {
    std::any accept(StmtVisitor& visitor) override {
        return visitor.visitIfStmt(this);
    }
};

// Implement visitor for specific purpose
class Interpreter : public StmtVisitor, public ExprVisitor {
    std::any visitIfStmt(If* stmt) override {
        // Execute if statement logic
    }
};
```

**Benefits:**
- Separation of concerns: AST structure vs. operations
- Easy to add new operations without modifying AST classes
- Type-safe dispatch to correct handler method
- Common pattern in compiler design

## Statement Types

### 1. Block Statement
```lox
{
    var x = 10;
    print x;
}
```
- Creates new lexical scope
- Contains list of statements
- Variables declared inside are local

### 2. Expression Statement
```lox
foo();
x = 5;
1 + 2;  // Valid but useless
```
- Wraps an expression as a statement
- Expression evaluated for side effects
- Return value discarded

### 3. Var Declaration
```lox
var x;          // Initialized to nil
var y = 10;     // Initialized to 10
```
- Declares variable in current scope
- Optional initializer expression
- Uninitialized variables default to `nil`

### 4. Function Declaration
```lox
fun add(a, b) {
    return a + b;
}
```
- Defines named function
- Parameters are tokens (identifiers)
- Body is list of statements
- Functions are first-class values

### 5. Class Declaration
```lox
class Animal {
    speak() {
        print "Some sound";
    }
}

class Dog < Animal {
    speak() {
        print "Woof!";
    }
}
```
- Defines class with methods
- Optional superclass for inheritance
- Methods are Function statements

### 6. If Statement
```lox
if (condition) {
    print "true";
} else {
    print "false";
}
```
- Conditional execution
- Else branch is optional
- Condition is evaluated for truthiness

### 7. While Statement
```lox
while (x < 10) {
    print x;
    x = x + 1;
}
```
- Loop with condition check before each iteration
- Body executes while condition is truthy

### 8. For Statement (Desugared)
```lox
for (var i = 0; i < 10; i = i + 1) {
    print i;
}
```

**Desugared to:**
```lox
{
    var i = 0;
    while (i < 10) {
        print i;
        i = i + 1;
    }
}
```

- Parser transforms for-loops into while-loops
- Simplifies interpreter (only needs while-loop logic)
- Common compiler technique

### 9. Print Statement
```lox
print "Hello, World!";
print 42;
```
- Language-level output statement (not a function)
- Evaluates expression and prints result

### 10. Return Statement
```lox
fun double(x) {
    return x * 2;
}

fun noReturn() {
    return;  // Returns nil
}
```
- Exits function with optional value
- Empty return yields `nil`
- Implemented via exception-based control flow

## Parser Design

### Recursive Descent Parsing

The parser uses recursive descent with these key methods:

```
parse()
├── declaration() (loop)
    ├── classDeclaration()
    ├── functionDeclaration()
    ├── varDeclaration()
    └── statement()
        ├── printStatement()
        ├── returnStatement()
        ├── ifStatement()
        ├── whileStatement()
        ├── forStatement() → desugars to while
        ├── block() → list of declarations
        └── expressionStatement()
            └── expression()
                └── [precedence climbing for expressions]
```

### Grammar (Simplified)

```
program        → declaration* EOF ;

declaration    → classDecl | funDecl | varDecl | statement ;
classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
funDecl        → "fun" function ;
varDecl        → "var" IDENTIFIER ( "=" expression )? ";" ;

statement      → exprStmt | forStmt | ifStmt | printStmt 
               | returnStmt | whileStmt | block ;
exprStmt       → expression ";" ;
forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
                 expression? ";" expression? ")" statement ;
ifStmt         → "if" "(" expression ")" statement ( "else" statement )? ;
printStmt      → "print" expression ";" ;
returnStmt     → "return" expression? ";" ;
whileStmt      → "while" "(" expression ")" statement ;
block          → "{" declaration* "}" ;

function       → IDENTIFIER "(" parameters? ")" block ;
parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
```

### Expression Precedence (for reference)

```
expression     → assignment ;
assignment     → IDENTIFIER "=" assignment | logic_or ;
logic_or       → logic_and ( "or" logic_and )* ;
logic_and      → equality ( "and" equality )* ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary | call ;
call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" | IDENTIFIER | "this" 
               | "super" "." IDENTIFIER ;
```

## Error Handling

### Panic Mode Recovery

When a syntax error occurs:

1. **Report**: Error message with line number
2. **Throw**: ParseError exception
3. **Catch**: In declaration() method
4. **Synchronize**: Skip tokens until statement boundary
5. **Continue**: Resume parsing next statement

### Synchronization Points

Parser synchronizes at:
- Semicolons (`;`) - statement terminators
- Keywords: `class`, `fun`, `var`, `for`, `if`, `while`, `print`, `return`

This allows multiple errors to be reported in a single parse pass.

## Memory Management

### Smart Pointers

All AST nodes use `std::shared_ptr` for automatic memory management:

```cpp
std::shared_ptr<Stmt> stmt = std::make_shared<Print>(expr);
std::vector<std::shared_ptr<Stmt>> statements;
```

**Benefits:**
- Automatic cleanup (no memory leaks)
- Reference counting handles shared ownership
- Safe for AST nodes that can be referenced multiple times

### Ownership Model

```
Parser creates statements → returns vector<shared_ptr<Stmt>>
                          ↓
Interpreter receives statements → executes via visitor pattern
                          ↓
When done, shared_ptrs go out of scope → automatic deletion
```

## Pipeline Flow

### Complete Execution Flow

```
Source Code (string)
    ↓
Scanner (Scanner.cpp)
    ↓
Tokens (vector<Token>)
    ↓
Parser (Parser.cpp)
    ↓
Statements (vector<shared_ptr<Stmt>>)
    ↓
Interpreter (Interpreter.cpp)
    ↓
Execution / Output
```

### Example

**Input:**
```lox
var x = 10;
if (x > 5) {
    print "big";
}
```

**Tokens:**
```
VAR, IDENTIFIER(x), EQUAL, NUMBER(10), SEMICOLON,
IF, LEFT_PAREN, IDENTIFIER(x), GREATER, NUMBER(5), RIGHT_PAREN,
LEFT_BRACE, PRINT, STRING("big"), SEMICOLON, RIGHT_BRACE
```

**AST:**
```
[
  Var { name: "x", initializer: Literal(10) },
  If {
    condition: Binary(Variable("x"), GREATER, Literal(5)),
    thenBranch: Block([
      Print(Literal("big"))
    ]),
    elseBranch: nullptr
  }
]
```

**Execution:**
1. Execute Var statement → bind x to 10
2. Execute If statement:
   - Evaluate condition: x > 5 → true
   - Execute then branch → Print statement → output "big"

## Key Design Decisions

### 1. Statements vs Expressions

**Expressions** produce values, **statements** perform actions:
- `2 + 3` is an expression (produces 5)
- `print 2 + 3;` is a statement (outputs 5)
- `x = 10;` is an expression statement (assignment produces value, semicolon makes it statement)

### 2. Desugaring

Complex constructs are transformed into simpler ones:
- For-loops → while-loops (in parser)
- Simplifies interpreter implementation
- Common compiler optimization technique

### 3. Visitor Pattern

Separates AST structure from operations:
- AST classes (Stmt.h, exp.h) define structure
- Visitors (Interpreter, future Resolver) define operations
- Easy to add new operations without modifying AST

### 4. Two-Phase Structure

**Phase 1: Parse** (this implementation)
- Scanner → tokens
- Parser → AST statements

**Phase 2: Execute** (Interpreter.cpp - partially implemented)
- Interpreter visits statements
- Executes according to semantics

Future phases could include:
- Resolver: Variable binding and semantic analysis
- Optimizer: AST transformations
- Code generator: Bytecode or native code emission

## Testing

### Test Coverage

The system is tested at multiple levels:

1. **Scanner tests** (scanner_test) - Token generation
2. **Parser tests** (parser_test) - AST construction for expressions
3. **Interpreter tests** (interpreter_test) - End-to-end expression evaluation

### Future Testing Needs

Statement execution testing requires:
- Environment/scope management
- Variable storage and lookup
- Control flow execution
- Function call mechanism
- Class instantiation and method dispatch

These are partially implemented in Interpreter.cpp but need completion.

## Extending the System

### Adding a New Statement Type

1. **Define in Stmt.h:**
```cpp
class NewStmt : public Stmt {
public:
    // Add fields
    const Token keyword;
    
    NewStmt(Token keyword) : keyword(keyword) {}
    
    std::any accept(StmtVisitor& visitor) override {
        return visitor.visitNewStmt(this);
    }
};
```

2. **Add visitor method to StmtVisitor:**
```cpp
virtual std::any visitNewStmt(NewStmt* stmt) = 0;
```

3. **Implement in Parser.cpp:**
```cpp
std::shared_ptr<Stmt> Parser::newStatement() {
    // Parse the new statement
    Token keyword = consume(NEW_KEYWORD, "Expect keyword");
    // ... parse components
    return std::make_shared<NewStmt>(keyword);
}
```

4. **Add to statement() dispatcher:**
```cpp
std::shared_ptr<Stmt> Parser::statement() {
    if (match(NEW_KEYWORD)) return newStatement();
    // ... existing cases
}
```

5. **Implement in Interpreter:**
```cpp
std::any Interpreter::visitNewStmt(NewStmt* stmt) {
    // Execute the new statement
}
```

## Conclusion

The statement system provides a solid foundation for complete Lox program execution. The design is:

- **Extensible**: Easy to add new statement types
- **Maintainable**: Clean separation of concerns via visitor pattern
- **Efficient**: Smart pointers handle memory automatically
- **Robust**: Error recovery allows multiple errors per parse

The current implementation focuses on parsing (frontend). Full execution (backend) requires completing the interpreter with environment management, function calls, and class support.
