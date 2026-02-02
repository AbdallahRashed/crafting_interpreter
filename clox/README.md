# Lox Compiler - Scanner Module

## Project Structure

This is a modular implementation of the Lox language scanner (lexical analyzer) compatible with the expression AST.

### Files

#### Scanner Module
- **TokenType.h** - Enum definition for all token types
- **TokenType.cpp** - Implementation of token type utilities (toString, keywords map)
- **Token.h** - Token struct that holds type, lexeme, literal value, and line number
- **Scanner.h** - Scanner class declaration
- **Scanner.cpp** - Scanner class implementation (tokenization logic)
- **main.cpp** - Test program with various scanner test cases

#### Expression AST Module
- **exp.h** - Complete expression AST with visitor pattern
  - Base `Expr` class with visitor support
  - 12 expression types: Assign, Binary, Call, Get, Grouping, Literal, Logical, Set, Super, This, Unary, Variable
  - Uses raw pointers in visitor pattern (not `shared_ptr`)
  - Expression nodes themselves use `shared_ptr` for child expressions

### Key Design Decisions

1. **Modular Structure**: Separated concerns into focused files
   - TokenType enum and utilities
   - Token data structure
   - Scanner implementation
   
2. **Token Design**: 
   - Uses `std::any` for literal values (compatible with exp.h)
   - Supports string, number, and empty literals
   - Includes line number for error reporting

3. **Changed from Java**:
   - `List<Token>` → `std::vector<Token>`
   - `Object` literal → `std::any` literal
   - Added header guards (`#pragma once`)
   - Separated implementation into .h and .cpp files

4. **Visitor Pattern**:
   - exp.h uses raw pointers in visitor methods
   - Avoids `enable_shared_from_this` complications
   - Expression ownership managed externally via `shared_ptr`

## Building

```bash
make           # Build the scanner test
make run       # Build and run tests
make clean     # Clean build artifacts
```

## Testing

The `main.cpp` includes 10 comprehensive test cases:
1. Single character tokens
2. Two character tokens
3. Comments
4. String literals
5. Number literals
6. Identifiers and keywords
7. Mixed expressions
8. Error: Unterminated string
9. Error: Unexpected characters
10. Multi-line code

## Integration with exp.h

The Token struct is now properly integrated with exp.h:
- exp.h includes Token.h (no forward declaration)
- Token uses `std::any` for literals (same as Expr::Literal)
- Both use modern C++ patterns (smart pointers, header guards)

## Next Steps

To build a complete interpreter, you would need:
- Parser (converts tokens to expression AST)
- Interpreter/Evaluator (visitor implementation for exp.h)
- Error handling and reporting
- REPL or file execution
