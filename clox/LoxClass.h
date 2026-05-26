#pragma once
#include "LoxCallable.h"
#include <unordered_map>
#include <string>
#include <any>
#include <memory>

class LoxInstance;

class LoxClass : public LoxCallable, public std::enable_shared_from_this<LoxClass> {
public:
    const std::string name;
    std::shared_ptr<LoxClass> superclass;
    std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods;

    LoxClass(const std::string& name,
             std::shared_ptr<LoxClass> superclass,
             std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods)
        : name(name), superclass(superclass), methods(methods) {}

    std::shared_ptr<LoxFunction> findMethod(const std::string& name) const {
        auto it = methods.find(name);
        if (it != methods.end()) return it->second;
        if (superclass != nullptr) return superclass->findMethod(name);
        return nullptr;
    }

    int arity() const override;
    std::any call(Interpreter& interpreter, const std::vector<std::any>& arguments) override;
    std::string toString() const override { return name; }
};

class LoxInstance : public std::enable_shared_from_this<LoxInstance> {
public:
    std::shared_ptr<LoxClass> klass;

    LoxInstance(std::shared_ptr<LoxClass> klass) : klass(klass) {}

    std::any get(const Token& name);
    void set(const Token& name, std::any value);
    std::string toString() const { return klass->name + " instance"; }

private:
    std::unordered_map<std::string, std::any> fields;
};
