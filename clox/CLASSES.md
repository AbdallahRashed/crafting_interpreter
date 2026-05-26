# Classes & Inheritance in the Lox Interpreter

## Overview

This document explains how **classes**, **instances**, **methods**, **constructors**, **inheritance**, and **super** are implemented in our tree-walking Lox interpreter. These are the final features that complete the language.

---

## 1. What Classes Add to Lox

```lox
class Circle {
    init(radius) {
        this.radius = radius;
    }
    area() {
        return 3.14159 * this.radius * this.radius;
    }
}

var c = Circle(5);
print c.area(); // 78.53975
```

Classes provide:
- **Encapsulation**: bundling data (fields) with behavior (methods)
- **Constructors** (`init`): automatic initialization when creating instances
- **`this`**: a reference to the current instance inside methods
- **Inheritance** (`<`): reusing behavior from a parent class
- **`super`**: calling overridden methods from the parent

---

## 2. Runtime Representation

### LoxClass

A class at runtime is a **callable** object. When you "call" a class (e.g., `Circle(5)`), it creates a new instance:

```cpp
class LoxClass : public LoxCallable, public enable_shared_from_this<LoxClass> {
public:
    const std::string name;
    shared_ptr<LoxClass> superclass;
    unordered_map<string, shared_ptr<LoxFunction>> methods;

    shared_ptr<LoxFunction> findMethod(const string& name) const;
    int arity() const override;
    any call(Interpreter& interpreter, const vector<any>& arguments) override;
};
```

Key design decisions:
- `LoxClass` extends `LoxCallable` — classes are callable (calling them creates instances)
- `enable_shared_from_this` — needed so `call()` can pass a `shared_ptr<LoxClass>` to the new instance
- `findMethod()` walks up the inheritance chain (class → superclass → ...)
- `arity()` delegates to the `init` method's arity (or 0 if no `init`)

### LoxInstance

An instance holds a reference to its class and a map of fields:

```cpp
class LoxInstance : public enable_shared_from_this<LoxInstance> {
    shared_ptr<LoxClass> klass;
    unordered_map<string, any> fields;
public:
    any get(const Token& name);    // field lookup → method lookup
    void set(const Token& name, any value);  // always sets a field
};
```

Property access (`instance.foo`) checks **fields first**, then **methods**. This means a field can shadow a method.

---

## 3. How Class Declaration Works

When the interpreter encounters a class statement:

```cpp
std::any Interpreter::visitClassStmt(Class* stmt) {
    // 1. Evaluate superclass (if any)
    // 2. Define class name in current environment (allows self-reference)
    // 3. If inheriting, create a scope with "super" bound
    // 4. Wrap each method as a LoxFunction (capturing current environment)
    // 5. Create LoxClass with methods map
    // 6. Assign the class to its name in the environment
}
```

### Step by step:

1. **Superclass evaluation**: If `class Dog < Animal`, evaluate `Animal` and verify it's actually a `LoxClass`
2. **Forward declaration**: Define the class name before processing methods (so methods can reference the class)
3. **Super environment**: Create a new scope containing `"super"` → the superclass. Methods close over this scope.
4. **Method creation**: Each method becomes a `LoxFunction` whose closure captures the environment (including `super`)
5. **Assembly**: Create the `LoxClass` object with the methods map and optional superclass
6. **Assignment**: Store the finished class in the environment

---

## 4. Instance Creation (Calling a Class)

```cpp
any LoxClass::call(Interpreter& interpreter, const vector<any>& arguments) {
    auto instance = make_shared<LoxInstance>(shared_from_this());
    
    auto initializer = findMethod("init");
    if (initializer != nullptr) {
        initializer->bind(instance)->call(interpreter, arguments);
    }
    
    return instance;
}
```

1. Create a fresh `LoxInstance` pointing to this class
2. If there's an `init` method, **bind** it to the instance and call it
3. Return the instance (not the return value of `init`)

---

## 5. Property Access: Get and Set

### Get (`instance.property`)

```cpp
any Interpreter::visitGetExpr(Get* expr) {
    any object = evaluate(expr->object.get());
    if (object.type() == typeid(shared_ptr<LoxInstance>)) {
        return instance->get(expr->name);
    }
    throw RuntimeError(expr->name, "Only instances have properties.");
}
```

Inside `LoxInstance::get()`:
1. Check `fields` map first
2. If not found, look for a method via `klass->findMethod()`
3. If found, **bind** the method to `this` and return it
4. If neither exists, throw a runtime error

### Set (`instance.property = value`)

```cpp
any Interpreter::visitSetExpr(Set* expr) {
    // Evaluate object, verify it's an instance
    // Evaluate value
    // instance->set(name, value)  -- always creates/updates a field
    return value;
}
```

Set always writes to the **fields** map (never touches methods).

---

## 6. The `this` Keyword

### The Problem

Inside a method, `this` should refer to the instance the method was called on:

```lox
class Person {
    sayName() { print this.name; }
}
var p = Person();
p.name = "Alice";
p.sayName(); // "Alice"
```

### The Solution: `bind()`

When a method is accessed on an instance, we create a **new** `LoxFunction` with a modified closure:

```cpp
shared_ptr<LoxFunction> LoxFunction::bind(shared_ptr<LoxInstance> instance) {
    auto env = make_shared<Environment>(closure);
    env->define("this", instance);
    return make_shared<LoxFunction>(declaration, env);
}
```

This creates a new environment wrapping the method's original closure, with `"this"` defined in it. The method body can then access `this` as a regular variable.

### Resolver Support

The resolver creates a scope for `this` inside class bodies:

```cpp
// In visitClassStmt:
beginScope();
scopes.back()["this"] = true;  // "this" is always available in methods
for (auto& method : stmt->methods) resolveFunction(method.get());
endScope();
```

When the resolver encounters a `this` expression, it resolves it like any other variable, finding it at a known depth.

---

## 7. Inheritance

### Method Resolution

`findMethod()` implements the method resolution order (MRO):

```cpp
shared_ptr<LoxFunction> LoxClass::findMethod(const string& name) const {
    auto it = methods.find(name);
    if (it != methods.end()) return it->second;        // found in this class
    if (superclass != nullptr) return superclass->findMethod(name);  // walk up
    return nullptr;
}
```

This is a simple linear search up the single-inheritance chain. The first match wins (subclass methods override superclass methods).

### The `super` Keyword

`super.method()` bypasses the instance's class and starts the method lookup at the **superclass**:

```cpp
any Interpreter::visitSuperExpr(Super* expr) {
    int distance = locals[expr];
    auto superclass = any_cast<shared_ptr<LoxClass>>(
        environment->getAt(distance, "super"));
    auto instance = any_cast<shared_ptr<LoxInstance>>(
        environment->getAt(distance - 1, "this"));
    
    auto method = superclass->findMethod(expr->method.lexeme);
    return shared_ptr<LoxCallable>(method->bind(instance));
}
```

Key insight: `super` is resolved **statically** (at the class where it lexically appears), not dynamically. The resolver places `super` exactly one scope outside `this`:

```
[... outer scopes ...] → [super: LoxClass] → [this: LoxInstance] → [method params]
```

So `distance - 1` always finds `this`.

---

## 8. Environment Layout During Method Execution

When calling `derived.greet()` where `Derived < Base`:

```
Global:     { Base: <class>, Derived: <class>, d: <instance> }
    ↑
Super:      { super: Base }        ← created at class definition time
    ↑
This:       { this: <instance> }   ← created at bind() time
    ↑
Method:     { <params> }           ← created at call() time
```

The resolver calculates depths relative to the method body:
- `this` is at depth 1 (one hop: method → this-env)
- `super` is at depth 2 (two hops: method → this-env → super-env)

---

## 9. Type System Considerations

Since C++ `std::any` uses **exact type matching**, we must be careful about what types we store:

| Value | Stored type in `std::any` |
|-------|---------------------------|
| Class | `shared_ptr<LoxCallable>` (upcast) |
| Instance | `shared_ptr<LoxInstance>` |
| Method (bound) | `shared_ptr<LoxCallable>` (upcast) |
| `super` | `shared_ptr<LoxClass>` |
| `this` | `shared_ptr<LoxInstance>` |

This is why `visitGetExpr` checks for `shared_ptr<LoxInstance>` and `visitCallExpr` checks for `shared_ptr<LoxCallable>`.

---

## 10. Complete Example: Shapes

```lox
class Shape {
    init(color) {
        this.color = color;
    }
    describe() {
        print this.color + " " + this.type();
    }
}

class Circle < Shape {
    init(color, radius) {
        super.init(color);
        this.radius = radius;
    }
    type() { return "circle"; }
    area() { return 3.14159 * this.radius * this.radius; }
}

var c = Circle("red", 5);
c.describe();  // "red circle"
print c.area(); // 78.53975
```

Execution flow:
1. `Shape` class is created and stored globally
2. `Circle` class is created with `superclass = Shape`; a `super` scope is captured
3. `Circle("red", 5)` → creates instance, calls `Circle.init`
4. Inside `Circle.init`: `super.init(color)` → finds `Shape.init` via superclass, binds to instance, calls it
5. `c.describe()` → `Circle` has no `describe`, walks up to `Shape.describe`
6. `Shape.describe` calls `this.type()` → `this` is the Circle instance, finds `Circle.type`

This demonstrates **dynamic dispatch**: `describe()` is defined on Shape but `this.type()` resolves to the subclass method.

---

## 11. Summary of New Files and Changes

| File | Role |
|------|------|
| `LoxClass.h` | Declares `LoxClass` and `LoxInstance` |
| `LoxClass.cpp` | Implements `call()`, `get()`, `set()`, `bind()` |
| `Interpreter.cpp` | Implements `visitClassStmt`, `visitGetExpr`, `visitSetExpr`, `visitThisExpr`, `visitSuperExpr` |
| `Resolver.cpp` | `visitClassStmt` creates scopes for `this` and `super`; resolves `this`/`super` as local variables |
| `LoxCallable.h` | Added `LoxFunction::bind()` declaration |

---

## 12. Key Takeaways

1. **Classes are callable** — calling a class creates an instance (constructor pattern)
2. **`this` is a closure variable** — `bind()` creates a new environment with `this` defined
3. **Inheritance is method delegation** — `findMethod()` walks the superclass chain
4. **`super` is statically resolved** — it always refers to the superclass of the class where `super` lexically appears
5. **Fields shadow methods** — property access checks the instance's fields before the class's methods
6. **`init` is special** — it's called automatically during construction, and the class's arity matches `init`'s parameter count
