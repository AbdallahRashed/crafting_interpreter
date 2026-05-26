/**
 * Statement Execution Test
 * 
 * Tests the complete pipeline with statement execution:
 * - Variable declarations
 * - Print statements
 * - Blocks and scoping
 * - If statements
 * - While loops
 */

#include "Scanner.h"
#include "Parser.h"
#include "Interpreter.h"
#include "Resolver.h"
#include <iostream>

void runProgram(const std::string& source, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "Source:\n" << source << std::endl;
    std::cout << "Output:" << std::endl;
    
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    Parser parser(tokens);
    auto statements = parser.parse();
    
    Interpreter interpreter;
    interpreter.interpret();           // register native functions (clock, etc.)

    Resolver resolver(interpreter);
    resolver.resolve(statements);      // resolve variable depths before interpreting

    interpreter.interpret(statements);
}

int main() {
    std::cout << "=== Statement Execution Tests ===" << std::endl;
    
    // Test 1: Variable declaration and print
    runProgram(R"(
var x = 10;
print x;
)", "Variable declaration and print");

    // Test 2: Variable without initializer
    runProgram(R"(
var y;
print y;
)", "Uninitialized variable (should print nil)");

    // Test 3: Variable assignment
    runProgram(R"(
var a = 5;
print a;
a = 10;
print a;
)", "Variable assignment");

    // Test 4: Expression statements
    runProgram(R"(
var x = 1;
print x;
x = x + 1;
print x;
)", "Expression statements with arithmetic");

    // Test 5: Block scoping
    runProgram(R"(
var x = 10;
{
    var x = 20;
    print x;
}
print x;
)", "Block scoping (shadowing)");

    // Test 6: Nested blocks
    runProgram(R"(
var a = "outer";
{
    var a = "middle";
    {
        var a = "inner";
        print a;
    }
    print a;
}
print a;
)", "Nested blocks");

    // Test 7: If statement (true branch)
    runProgram(R"(
if (true) {
    print "yes";
}
)", "If statement - true branch");

    // Test 8: If-else (false branch)
    runProgram(R"(
if (false) {
    print "no";
} else {
    print "yes";
}
)", "If-else statement - else branch");

    // Test 9: If with condition
    runProgram(R"(
var x = 10;
if (x > 5) {
    print "big";
} else {
    print "small";
}
)", "If with comparison");

    // Test 10: While loop
    runProgram(R"(
var i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}
)", "While loop");

    // Test 11: Logical operators (short-circuit)
    runProgram(R"(
print true or false;
print false or true;
print true and false;
print true and true;
)", "Logical operators");

    // Test 12: For loop (desugared to while)
    runProgram(R"(
for (var i = 0; i < 3; i = i + 1) {
    print i;
}
)", "For loop");

    // Test 13: Complex program
    runProgram(R"(
var a = 0;
var temp = 1;
for (var i = 0; i < 10; i = i + 1) {
    print a;
    var next = a + temp;
    a = temp;
    temp = next;
}
)", "Fibonacci sequence (first 10 numbers)");

    // Test 14: Error case - undefined variable
    runProgram(R"(
print undefinedVar;
)", "ERROR: Undefined variable");

    // Test 15: Error case - assign to undefined
    runProgram(R"(
undefinedVar = 10;
)", "ERROR: Assign to undefined variable");

    // ==================== FUNCTION TESTS ====================

    // Test 16: Simple function declaration and call
    runProgram(R"(
fun sayHi() {
    print "hi";
}
sayHi();
)", "Simple function call");

    // Test 17: Function with parameters
    runProgram(R"(
fun add(a, b) {
    print a + b;
}
add(3, 4);
)", "Function with parameters");

    // Test 18: Function called multiple times
    runProgram(R"(
fun greet(name) {
    print "Hello, " + name + "!";
}
greet("Alice");
greet("Bob");
)", "Function called multiple times");

    // Test 19: Recursive function (countdown)
    runProgram(R"(
fun countdown(n) {
    if (n <= 0) {
        print "done";
    } else {
        print n;
        countdown(n - 1);
    }
}
countdown(3);
)", "Recursive function");

    // Test 20: Native clock function
    runProgram(R"(
var t = clock();
print t > 0;
)", "Native clock() returns positive number");

    // Test 21: Function with return value
    runProgram(R"(
fun add(a, b) {
    return a + b;
}
print add(3, 4);
)", "Function return value");

    // Test 22: Early return
    runProgram(R"(
fun sign(n) {
    if (n < 0) return "negative";
    if (n > 0) return "positive";
    return "zero";
}
print sign(-5);
print sign(0);
print sign(3);
)", "Early return from function");

    // Test 23: Recursive with return
    runProgram(R"(
fun fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
print fib(7);
)", "Recursive fibonacci with return");
    // ==================== CLOSURE TESTS ====================

    // Test 24: Counter closure — inner function captures outer variable
    runProgram(R"(
fun makeCounter() {
    var count = 0;
    fun increment() {
        count = count + 1;
        return count;
    }
    return increment;
}
var counter = makeCounter();
print counter();
print counter();
print counter();
)", "Counter closure (should print 1, 2, 3)");

    // Test 25: Two independent closures don't share state
    runProgram(R"(
fun makeCounter() {
    var count = 0;
    fun increment() {
        count = count + 1;
        return count;
    }
    return increment;
}
var c1 = makeCounter();
var c2 = makeCounter();
print c1();
print c1();
print c2();
print c1();
)", "Independent closures (c1:1,2 c2:1 c1:3)");

    // Test 26: Closure captures variable by reference (shared state)
    runProgram(R"(
fun makeAdder(x) {
    fun add(y) {
        return x + y;
    }
    return add;
}
var add5 = makeAdder(5);
var add10 = makeAdder(10);
print add5(3);
print add10(3);
)", "Adder closure (should print 8, 13)");

    // ============== CLASS TESTS ==============

    // Test: Basic class and instance
    runProgram(R"(
class Foo {}
var foo = Foo();
print foo;
)", "Basic class instantiation (should print 'Foo instance')");

    // Test: Fields on instances
    runProgram(R"(
class Point {}
var p = Point();
p.x = 10;
p.y = 20;
print p.x;
print p.y;
)", "Instance fields (should print 10, 20)");

    // Test: Methods
    runProgram(R"(
class Greeter {
    greet() {
        print "Hello!";
    }
}
var g = Greeter();
g.greet();
)", "Class methods (should print 'Hello!')");

    // Test: this in methods
    runProgram(R"(
class Person {
    sayName() {
        print this.name;
    }
}
var p = Person();
p.name = "Alice";
p.sayName();
)", "This binding (should print 'Alice')");

    // Test: Constructor (init)
    runProgram(R"(
class Circle {
    init(radius) {
        this.radius = radius;
    }
    area() {
        return 3.14159 * this.radius * this.radius;
    }
}
var c = Circle(5);
print c.radius;
print c.area();
)", "Constructor and methods (should print 5, 78.53975)");

    // Test: Inheritance
    runProgram(R"(
class Animal {
    speak() {
        print "...";
    }
}
class Dog < Animal {
    speak() {
        print "Woof!";
    }
}
class Cat < Animal {}
var d = Dog();
d.speak();
var c = Cat();
c.speak();
)", "Inheritance (should print 'Woof!', '...')");

    // Test: super
    runProgram(R"(
class Base {
    greet() {
        print "Hello from Base";
    }
}
class Derived < Base {
    greet() {
        super.greet();
        print "Hello from Derived";
    }
}
var d = Derived();
d.greet();
)", "Super calls (should print 'Hello from Base', 'Hello from Derived')");

    // Test: init returns instance
    runProgram(R"(
class Foo {
    init() {
        this.x = 42;
    }
}
var f = Foo();
print f.x;
)", "Init returns instance with field (should print 42)");

    // Test: Inheritance with constructors
    runProgram(R"(
class Shape {
    init(color) {
        this.color = color;
    }
}
class Square < Shape {
    init(color, size) {
        super.init(color);
        this.size = size;
    }
    area() {
        return this.size * this.size;
    }
}
var sq = Square("red", 4);
print sq.color;
print sq.size;
print sq.area();
)", "Inheritance with super.init (should print 'red', 4, 16)");

    // Test: Methods from superclass can access subclass fields
    runProgram(R"(
class Base {
    describe() {
        print this.name;
    }
}
class Child < Base {
    init(name) {
        this.name = name;
    }
}
var c = Child("test");
c.describe();
)", "Inherited method accessing subclass field (should print 'test')");

    std::cout << "\n=== All tests completed ===" << std::endl;
    return 0;
}
