# Bytecode Virtual Machine — Educational Guide

## Introduction

This document is a comprehensive guide to the **clox bytecode virtual machine**, the Part III implementation from *Crafting Interpreters*. While the tree-walking interpreter (in `clox/`) evaluates an AST directly, this implementation **compiles** Lox source code into a compact bytecode format and executes it on a **stack-based virtual machine**.

The result: the same language, ~50x faster.

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│                     Source Code                           │
│              "var x = 1 + 2; print x;"                   │
└───────────────────────┬──────────────────────────────────┘
                        │
                        ▼
┌──────────────────────────────────────────────────────────┐
│                      Scanner                             │
│         Produces tokens: VAR, IDENTIFIER, EQUAL,         │
│         NUMBER, PLUS, NUMBER, SEMICOLON, ...             │
└───────────────────────┬──────────────────────────────────┘
                        │
                        ▼
┌──────────────────────────────────────────────────────────┐
│                     Compiler                             │
│      Single-pass Pratt parser → emits bytecode           │
│                                                          │
│   OP_CONSTANT 1                                          │
│   OP_CONSTANT 2                                          │
│   OP_ADD                                                 │
│   OP_DEFINE_GLOBAL "x"                                   │
│   OP_GET_GLOBAL "x"                                      │
│   OP_PRINT                                               │
└───────────────────────┬──────────────────────────────────┘
                        │
                        ▼
┌──────────────────────────────────────────────────────────┐
│                   Virtual Machine                         │
│              Stack-based execution engine                 │
│                                                          │
│   Stack: [ 1 ]  → [ 1 | 2 ]  → [ 3 ]  → []             │
│   Output: 3                                              │
└──────────────────────────────────────────────────────────┘
```

Key difference from the tree-walker:
- **No AST** — the compiler directly produces flat bytecode
- **No visitor pattern** — the VM is a giant `switch` statement in a loop
- **No heap-allocated nodes** — bytecode is a compact `uint8_t` array
- **Faster dispatch** — one switch vs. virtual method calls + pointer chasing

---

## 1. Value Representation (value.h)

### The Problem

Every variable, expression result, and constant needs a runtime representation. In C, we don't have a universal type like `std::any`. We need something that can hold a `double`, `bool`, `nil`, or a pointer to a heap object — all in a single "slot."

### Tagged Union (Without NaN Boxing)

```c
typedef struct {
    ValueType type;   // VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;
```

This uses 16 bytes per value (8 for the double + padding + tag). Access patterns:
```c
// Creating values
Value v = NUMBER_VAL(3.14);
Value s = OBJ_VAL(myString);

// Type checking
if (IS_NUMBER(v)) { ... }

// Extracting
double d = AS_NUMBER(v);
```

### NaN Boxing (Enabled by Default)

The clever trick: IEEE 754 doubles have many bit patterns that represent NaN. We "steal" those patterns to encode other types in a single 64-bit word:

```
 Sign  Exponent      Mantissa (52 bits)
  [1] [11111111111] [1100000000000000000000000000000000000000000000000000]
   │       │              │
   │       └─ All 1s = NaN│
   │                      └─ "Quiet" NaN signal bits
   └─ We use this bit to mark object pointers
```

Layout:
- **Number**: any valid double (no special encoding needed)
- **nil**: `QNAN | 0x01`
- **false**: `QNAN | 0x02`
- **true**: `QNAN | 0x03`
- **Object pointer**: `SIGN_BIT | QNAN | pointer` (pointers fit in 48 bits on x86-64)

Result: every `Value` is exactly 8 bytes — half the size of the tagged union, and fits in a register.

---

## 2. Chunks — Bytecode Storage (chunk.h)

A "chunk" is the compiled output — an array of bytes representing instructions:

```c
typedef struct {
    int count;           // how many bytes used
    int capacity;        // how many bytes allocated
    uint8_t* code;       // the bytecode array
    int* lines;          // source line for each byte (for error reporting)
    ValueArray constants; // constant pool
} Chunk;
```

### Instruction Format

Most instructions are 1-3 bytes:

```
[opcode]                     — simple: OP_ADD, OP_RETURN, OP_NIL
[opcode][operand]            — 1-byte operand: OP_CONSTANT, OP_GET_LOCAL
[opcode][hi][lo]             — 2-byte operand: OP_JUMP, OP_LOOP
[opcode][constant][argcount] — invoke: OP_INVOKE
```

### The Constant Pool

Literal values (numbers, strings) are stored in a separate `ValueArray`. Bytecode references them by index:

```
OP_CONSTANT 3   →  push constants[3] onto the stack
```

This means even large numbers or long strings cost only 2 bytes in the bytecode stream.

---

## 3. The OpCodes (chunk.h)

| Category | OpCodes | Purpose |
|----------|---------|---------|
| Constants | `OP_CONSTANT`, `OP_NIL`, `OP_TRUE`, `OP_FALSE` | Push values onto stack |
| Arithmetic | `OP_ADD`, `OP_SUBTRACT`, `OP_MULTIPLY`, `OP_DIVIDE`, `OP_NEGATE` | Pop operands, push result |
| Comparison | `OP_EQUAL`, `OP_GREATER`, `OP_LESS` | Pop two, push bool |
| Logic | `OP_NOT` | Pop one, push negation |
| Variables | `OP_GET/SET/DEFINE_GLOBAL`, `OP_GET/SET_LOCAL`, `OP_GET/SET_UPVALUE` | Variable access at different scopes |
| Control | `OP_JUMP`, `OP_JUMP_IF_FALSE`, `OP_LOOP` | Branching and looping |
| Functions | `OP_CALL`, `OP_CLOSURE`, `OP_RETURN`, `OP_CLOSE_UPVALUE` | Function machinery |
| Classes | `OP_CLASS`, `OP_METHOD`, `OP_INHERIT`, `OP_GET/SET_PROPERTY`, `OP_INVOKE`, `OP_SUPER_INVOKE`, `OP_GET_SUPER` | OOP |
| I/O | `OP_PRINT` | Print top of stack |
| Stack | `OP_POP` | Discard top of stack |

---

## 4. The Scanner (scanner.h/c)

The scanner converts source text into tokens. Unlike the tree-walker's scanner (which produces a `vector<Token>`), this scanner is **on-demand** — the compiler calls `scanToken()` one token at a time.

Key design:
- **No token list** — tokens are produced lazily
- **Keyword detection** uses a trie-like switch (not a hash map)
- **Zero allocation** — tokens point directly into the source string

```c
Token scanToken(void) {
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();
    switch (c) { /* single/double char tokens */ }
    return errorToken("Unexpected character.");
}
```

The `Token` struct is just a pointer + length into the source string:
```c
typedef struct {
    TokenType type;
    const char* start;   // pointer into source
    int length;
    int line;
} Token;
```

---

## 5. The Compiler (compiler.h/c)

This is the heart of the system — a **single-pass Pratt parser** that compiles directly to bytecode without constructing an AST.

### Pratt Parsing

Each token type has a parsing rule:
```c
typedef struct {
    ParseFn prefix;       // how to parse when token appears at start
    ParseFn infix;        // how to parse when token appears after an operand
    Precedence precedence; // binding power
} ParseRule;

ParseRule rules[] = {
    [TOKEN_MINUS] = {unary,  binary, PREC_TERM},
    [TOKEN_PLUS]  = {NULL,   binary, PREC_TERM},
    [TOKEN_STAR]  = {NULL,   binary, PREC_FACTOR},
    [TOKEN_DOT]   = {NULL,   dot,    PREC_CALL},
    // ...
};
```

The central dispatcher:
```c
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
}
```

This elegant algorithm handles operator precedence without recursive descent nesting.

### Compiling Expressions

When the compiler encounters `1 + 2 * 3`:

1. `number()` → emits `OP_CONSTANT 1`
2. Sees `+` at PREC_TERM. Parses RHS with PREC_FACTOR:
   - `number()` → emits `OP_CONSTANT 2`
   - Sees `*` at PREC_FACTOR, parses RHS:
     - `number()` → emits `OP_CONSTANT 3`
   - Returns from inner, emits `OP_MULTIPLY`
3. Returns from outer, emits `OP_ADD`

Result: `OP_CONSTANT 1, OP_CONSTANT 2, OP_CONSTANT 3, OP_MULTIPLY, OP_ADD`

### Compiling Variables

**Global variables** are stored by name in a hash table:
```c
OP_DEFINE_GLOBAL "x"   // define
OP_GET_GLOBAL "x"      // read
OP_SET_GLOBAL "x"      // write
```

**Local variables** live on the stack — no name lookup needed:
```c
OP_GET_LOCAL 3         // read stack slot 3
OP_SET_LOCAL 3         // write stack slot 3
```

The compiler tracks locals in a compile-time array and resolves them to stack indices. This is why locals are faster than globals.

### Compiling Control Flow

**If statements** use jump instructions:
```
      [condition code]
      OP_JUMP_IF_FALSE ─────┐
      OP_POP                │
      [then body]           │
      OP_JUMP ──────────────┼──┐
      OP_POP   ◄────────────┘  │
      [else body]               │
      ...      ◄────────────────┘
```

**While loops** jump backward:
```
 ┌──► [condition code]
 │    OP_JUMP_IF_FALSE ─────┐
 │    OP_POP                │
 │    [body]                │
 │    OP_LOOP ──────────────┘
 └─── (target)
      OP_POP   ◄────────────┘
```

The compiler uses **backpatching** — it emits placeholder jump offsets, then patches them once it knows the target address.

### Compiling Functions and Closures

Each function gets its own `Compiler` struct and `ObjFunction`:
```c
static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);  // push new compiler
    beginScope();
    // ... compile parameters and body ...
    ObjFunction* fn = endCompiler(); // pop compiler, get function
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(fn)));
    // emit upvalue metadata
}
```

For closures, the compiler tracks which variables are "captured":
```
OP_CLOSURE <fn_constant>
  1 3    ← upvalue 0: local, slot 3 (captures enclosing local)
  0 1    ← upvalue 1: upvalue, index 1 (captures enclosing upvalue)
```

### Compiling Classes

```c
static void classDeclaration(void) {
    // 1. Create the class object
    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    // 2. Handle inheritance
    if (match(TOKEN_LESS)) {
        // load superclass, emit OP_INHERIT
        // create scope for "super"
    }

    // 3. Compile methods
    namedVariable(className, false);  // push class
    while (!check(TOKEN_RIGHT_BRACE)) {
        method();  // each method: compile function, emit OP_METHOD
    }
    emitByte(OP_POP);  // pop class
}
```

---

## 6. The Virtual Machine (vm.h/c)

### VM Structure

```c
typedef struct {
    CallFrame frames[FRAMES_MAX];  // call stack
    int frameCount;

    Value stack[STACK_MAX];        // operand stack
    Value* stackTop;               // top of stack pointer

    Table globals;                 // global variable hash table
    Table strings;                 // string interning table
    ObjString* initString;         // cached "init" string
    ObjUpvalue* openUpvalues;      // linked list of open upvalues

    // GC fields
    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects;                  // linked list of ALL objects
    Obj** grayStack;               // GC worklist
} VM;
```

### The Execution Loop

The VM is a simple decode-dispatch loop:

```c
static InterpretResult run(void) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    for (;;) {
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                }
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                callValue(peek(argCount), argCount);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            // ... ~40 cases total
        }
    }
}
```

### The Stack

All computation happens on the stack:

```
Expression: (1 + 2) * 3

Bytecode:           Stack after:
OP_CONSTANT 1       [ 1 ]
OP_CONSTANT 2       [ 1 | 2 ]
OP_ADD              [ 3 ]
OP_CONSTANT 3       [ 3 | 3 ]
OP_MULTIPLY         [ 9 ]
```

### Call Frames

Each function call creates a `CallFrame`:
```c
typedef struct {
    ObjClosure* closure;  // the function being executed
    uint8_t* ip;          // instruction pointer (where in the bytecode)
    Value* slots;         // pointer into vm.stack (this frame's window)
} CallFrame;
```

The `slots` pointer creates a "window" into the shared stack:
```
Stack:  [ script locals | fn1 args | fn1 locals | fn2 args | fn2 locals ]
         ^                ^                       ^
         frame[0].slots   frame[1].slots          frame[2].slots
```

Local variables are accessed by offset from `frame->slots`:
```c
case OP_GET_LOCAL: {
    uint8_t slot = READ_BYTE();
    push(frame->slots[slot]);
    break;
}
```

---

## 7. Closures and Upvalues (object.h)

### The Problem

When a function captures a variable from an enclosing scope, that variable might live on the stack (which gets popped when the enclosing function returns). We need a way to keep the captured value alive.

### Solution: Upvalues

An `ObjUpvalue` is an indirection to a captured variable:

```c
typedef struct ObjUpvalue {
    Obj obj;
    Value* location;     // points to stack slot (while open)
    Value closed;        // stores value after stack slot is gone
    struct ObjUpvalue* next;
} ObjUpvalue;
```

**Open upvalue**: `location` points to a live stack slot
**Closed upvalue**: when the variable goes out of scope, its value is copied into `closed`, and `location` is redirected to `&closed`

```
BEFORE closing (variable still on stack):
    Upvalue.location ──────► [ stack slot: 42 ]

AFTER closing (function returned, stack popped):
    Upvalue.location ──┐
                       ▼
    Upvalue.closed = 42
```

### Capturing

```c
static ObjUpvalue* captureUpvalue(Value* local) {
    // Walk the sorted open upvalue list
    // If one already points to this slot, reuse it
    // Otherwise, create a new one and insert it
    ObjUpvalue* createdUpvalue = newUpvalue(local);
    // ... insert into linked list ...
    return createdUpvalue;
}
```

### Closing

When a scope ends:
```c
static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL &&
           vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;  // copy value
        upvalue->location = &upvalue->closed;  // redirect
        vm.openUpvalues = upvalue->next;
    }
}
```

---

## 8. Object System (object.h/c)

All heap-allocated values share a common header:

```c
struct Obj {
    ObjType type;      // discriminant for downcasting
    bool isMarked;     // GC mark bit
    struct Obj* next;  // intrusive linked list of all objects
};
```

Object types: `OBJ_STRING`, `OBJ_FUNCTION`, `OBJ_CLOSURE`, `OBJ_UPVALUE`, `OBJ_CLASS`, `OBJ_INSTANCE`, `OBJ_BOUND_METHOD`, `OBJ_NATIVE`

### String Interning

All strings are stored in a global hash table (`vm.strings`). When creating a string, we first check if an identical one exists:

```c
ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;  // reuse existing
    // ... allocate new string ...
}
```

Benefits:
- String equality is pointer comparison (`O(1)` instead of `O(n)`)
- Less memory usage (no duplicate strings)

### Classes and Instances

```c
typedef struct {
    Obj obj;
    ObjString* name;
    Table methods;       // method name → ObjClosure
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;        // field name → Value
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;      // the "this" instance
    ObjClosure* method;  // the bound method
} ObjBoundMethod;
```

### Method Invocation Optimization

Normal property access: `obj.method()` requires:
1. `OP_GET_PROPERTY` → creates `ObjBoundMethod`
2. `OP_CALL` → calls the bound method

Optimized path: `OP_INVOKE` combines both in one instruction:
```c
case OP_INVOKE: {
    ObjString* method = READ_STRING();
    int argCount = READ_BYTE();
    invoke(method, argCount);  // direct dispatch, no BoundMethod allocation
    break;
}
```

---

## 9. Hash Table (table.h/c)

The hash table is used for globals, string interning, instance fields, and class methods.

### Design

- **Open addressing** with linear probing (no linked lists)
- **Tombstones** for deletion (entry with NULL key but non-nil value)
- **Load factor** < 75% (auto-grows)
- **FNV-1a** hash function

```c
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);  // fast modulo (power of 2)
    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {  // pointer comparison! (interned strings)
            return entry;
        }
        index = (index + 1) & (capacity - 1);
    }
}
```

Note: because strings are interned, key comparison is just `entry->key == key` (pointer equality).

---

## 10. Garbage Collector (memory.h/c)

### Mark-Sweep with Tri-Color Marking

The GC uses three "colors":
- **White**: not yet visited (candidates for collection)
- **Gray**: visited but children not yet traced (in the worklist)
- **Black**: visited and all children traced (definitely alive)

### Algorithm

```
1. MARK ROOTS
   - Everything on the stack
   - All call frame closures
   - Open upvalues
   - Global variables
   - Compiler's in-progress functions
   - The "init" string

2. TRACE REFERENCES (process gray worklist)
   while (grayStack not empty):
       object = pop gray
       for each reference in object:
           mark that reference gray

3. SWEEP
   Walk the full object list:
       if marked → unmark (for next cycle)
       if not marked → free it

4. REMOVE WEAK REFERENCES
   Remove white strings from the interning table
```

### When Does GC Run?

The GC is triggered when `bytesAllocated > nextGC`:
```c
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize && vm.bytesAllocated > vm.nextGC) {
        collectGarbage();
    }
    // ...
}
```

After collection, `nextGC` doubles: the heap must grow significantly before the next GC (amortized cost).

### GC Safety

Any allocation can trigger GC. Values that are live but not yet reachable from roots must be protected:
```c
// UNSAFE: if allocateString triggers GC, the key is unreachable
tableSet(&vm.strings, string, NIL_VAL);

// SAFE: push onto stack first (stack is a root)
push(OBJ_VAL(string));
tableSet(&vm.strings, string, NIL_VAL);
pop();
```

---

## 11. Inheritance at the VM Level

### OP_INHERIT

When `class Dog < Animal` is compiled:
```
OP_GET_GLOBAL "Animal"    // push superclass
OP_GET_GLOBAL "Dog"       // push subclass
OP_INHERIT                // copy methods
OP_POP                    // pop subclass
```

`OP_INHERIT` copies all methods from the superclass into the subclass's method table. This is "copy-down inheritance" — after inheriting, the subclass has its own copy of all methods. Method override is just writing a new entry with the same key.

### super

The compiler creates a local scope with a "super" variable pointing to the superclass:
```
beginScope();
addLocal(syntheticToken("super"));
defineVariable(0);                // store superclass in a local slot
namedVariable(className, false);  // push the class
OP_INHERIT
```

When `super.method()` is encountered:
```
namedVariable("this", false);   // push instance
namedVariable("super", false);  // push superclass (from upvalue/local)
OP_SUPER_INVOKE "method" 0      // call method on superclass with 0 args
```

---

## 12. Disassembler (debug.h/c)

The disassembler is invaluable for debugging. It prints human-readable bytecode:

```
== test ==
0000    1 OP_CONSTANT         0 '1'
0002    | OP_CONSTANT         1 '2'
0004    | OP_ADD
0005    | OP_DEFINE_GLOBAL    2 'x'
0007    2 OP_GET_GLOBAL       2 'x'
0009    | OP_PRINT
0010    | OP_NIL
0011    | OP_RETURN
```

Enable with `-DDEBUG_PRINT_CODE` (prints after compilation) or `-DDEBUG_TRACE_EXECUTION` (prints each instruction as it executes, with stack state).

---

## 13. Performance: Why Bytecode is Faster

| Factor | Tree-Walker | Bytecode VM |
|--------|-------------|-------------|
| Dispatch | Virtual method call (vtable lookup + pointer chase) | Single `switch` + computed `goto` |
| Data layout | AST nodes scattered across heap | Flat `uint8_t[]` (cache-friendly) |
| Value size | `std::any` = 32+ bytes | NaN-boxed = 8 bytes |
| Variable access | Hash table lookup every time | Stack slot index (single array access) |
| Allocation per expression | AST node on heap | Zero (values live on stack) |
| Method calls | Create new Environment + hash lookup | Push frame + index into stack |

Benchmark: `fib(30)`
- Tree-walker: ~3-5 seconds
- Bytecode VM: ~0.06 seconds

---

## 14. File Map

| File | Lines | Purpose |
|------|-------|---------|
| `common.h` | ~15 | Shared includes, feature flags, `UINT8_COUNT` |
| `value.h/c` | ~100 | Value representation (NaN boxing or tagged union) |
| `chunk.h/c` | ~60 | Bytecode chunk: code array + constant pool |
| `debug.h/c` | ~140 | Disassembler for debugging bytecode |
| `scanner.h/c` | ~200 | On-demand tokenizer |
| `compiler.h/c` | ~600 | Single-pass Pratt parser → bytecode emitter |
| `object.h/c` | ~180 | Heap objects: strings, functions, closures, classes |
| `table.h/c` | ~140 | Hash table (open addressing, linear probing) |
| `memory.h/c` | ~200 | Allocator + mark-sweep GC |
| `vm.h/c` | ~400 | Stack-based VM execution loop |
| `main.c` | ~65 | REPL + file reader |

Total: ~2100 lines of logic (excluding headers/boilerplate)

---

## 15. Building and Running

```bash
cd bytecode/

# Optimized build
make

# Debug build (prints bytecode + execution trace)
make debug

# Run a file
./build/clox test.lox

# Interactive REPL
./build/clox
```

---

## 16. Key Takeaways

1. **Single-pass compilation is possible** — Pratt parsing elegantly handles precedence without an AST
2. **The stack is the universal data structure** — locals, temporaries, arguments all live there
3. **Flat bytecode is cache-friendly** — a `uint8_t[]` beats pointer-chasing through AST nodes
4. **NaN boxing is free performance** — halves value size with no runtime cost
5. **String interning makes equality O(1)** — pointer comparison instead of character-by-character
6. **Upvalues solve the closure problem** — elegant transition from stack to heap when needed
7. **Copy-down inheritance is simple** — copying method tables avoids runtime superclass chain walks
8. **GC must be allocation-aware** — any point that allocates can trigger collection; roots must be visible
9. **invoke optimization matters** — most method calls can skip BoundMethod allocation entirely
10. **The compiler and VM are independent** — you could replace the frontend (e.g., compile Python syntax) and keep the same VM
