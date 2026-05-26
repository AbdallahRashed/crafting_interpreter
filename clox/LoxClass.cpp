#include "LoxClass.h"
#include "Interpreter.h"

// ==================== LoxClass ====================

int LoxClass::arity() const {
    auto initializer = findMethod("init");
    if (initializer != nullptr) return initializer->arity();
    return 0;
}

std::any LoxClass::call(Interpreter& interpreter, const std::vector<std::any>& arguments) {
    auto self = std::const_pointer_cast<LoxClass>(
        std::static_pointer_cast<const LoxClass>(shared_from_this()));
    auto instance = std::make_shared<LoxInstance>(self);

    // If there's an init() method, call it immediately
    auto initializer = findMethod("init");
    if (initializer != nullptr) {
        initializer->bind(instance)->call(interpreter, arguments);
    }

    return std::static_pointer_cast<LoxInstance>(instance);
}

// ==================== LoxInstance ====================

std::any LoxInstance::get(const Token& name) {
    // Fields first
    auto it = fields.find(name.lexeme);
    if (it != fields.end()) return it->second;

    // Then methods
    auto method = klass->findMethod(name.lexeme);
    if (method != nullptr) {
        return std::shared_ptr<LoxCallable>(method->bind(shared_from_this()));
    }

    throw std::runtime_error("Undefined property '" + name.lexeme + "'.");
}

void LoxInstance::set(const Token& name, std::any value) {
    fields[name.lexeme] = value;
}

// ==================== LoxFunction::bind ====================

std::shared_ptr<LoxFunction> LoxFunction::bind(std::shared_ptr<LoxInstance> instance) {
    auto env = std::make_shared<Environment>(closure);
    env->define("this", std::static_pointer_cast<LoxInstance>(instance));
    return std::make_shared<LoxFunction>(declaration, env);
}
