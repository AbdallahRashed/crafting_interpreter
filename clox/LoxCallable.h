
#pragma once
#include <any>
#include <vector>
#include <string>
#include <ctime>
#include <memory>
#include "Stmt.h"
#include "Environment.h"

class Interpreter;

class LoxCallable {
public:
    virtual std::any call(Interpreter& interpreter, const std::vector<std::any>& arguments) = 0;
    virtual int arity() const = 0;
    virtual std::string toString() const { return "<native fn>"; }
    virtual ~LoxCallable() = default;
};

// Native function: clock() — returns seconds since epoch (for benchmarking)
class ClockCallable : public LoxCallable {
public:
    int arity() const override { return 0; }
    std::any call(Interpreter& interpreter, const std::vector<std::any>& arguments) override {
        return static_cast<double>(std::time(nullptr));
    }
    std::string toString() const override { return "<native fn clock>"; }
};

class LoxFunction : public LoxCallable {
    Function* declaration;
    std::shared_ptr<Environment> closure;
public:
    LoxFunction(Function* stmt, std::shared_ptr<Environment> closure)
        : declaration(stmt), closure(closure) {}

    int arity() const override {
        return static_cast<int>(declaration->params.size());
    }

    std::any call(Interpreter& interpreter, const std::vector<std::any>& arguments) override;

    // Create a new LoxFunction bound to a 'this' instance
    std::shared_ptr<LoxFunction> bind(std::shared_ptr<class LoxInstance> instance);

    std::string toString() const override {
        return "<fn " + declaration->name.lexeme + ">";
    }
};