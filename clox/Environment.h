/**
 * Environment.h - Variable storage and scope management
 * 
 * Implements lexical scoping using a chain of environments.
 * Each block creates a new environment pointing to its parent.
 * 
 * Example scope chain:
 * 
 *   Global Environment { x: 10, foo: <function> }
 *        ↑
 *   Block Environment { y: 20 }  ← current
 * 
 * Variable lookup walks up the chain until found or error.
 */

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <any>
#include <stdexcept>
#include "Token.h"

// Forward declare RuntimeError (defined in Interpreter.h)
class RuntimeError;

class Environment {
private:
    // Variable storage: name → value
    std::unordered_map<std::string, std::any> values;
    
    // Parent scope (nullptr for global scope)
    std::shared_ptr<Environment> enclosing;

public:
    /**
     * Create global environment (no parent)
     */
    Environment() : enclosing(nullptr) {}
    
    /**
     * Create nested environment with parent scope
     */
    Environment(std::shared_ptr<Environment> enclosing) 
        : enclosing(enclosing) {}
    
    /**
     * Define new variable in current scope
     * Allows redefinition (variables can be shadowed)
     */
    void define(const std::string& name, std::any value) {
        values[name] = value;
    }
    
    /**
     * Get variable value by walking up scope chain
     * Throws runtime_error if not found
     */
    std::any get(const Token& name) {
        // Check current scope
        auto it = values.find(name.lexeme);
        if (it != values.end()) {
            return it->second;
        }
        
        // Check parent scopes
        if (enclosing != nullptr) {
            return enclosing->get(name);
        }
        
        // Not found in any scope
        throw std::runtime_error("Undefined variable '" + name.lexeme + "'.");
    }
    
    /**
     * Assign to existing variable (walks up scope chain)
     * Throws runtime_error if variable doesn't exist
     */
    void assign(const Token& name, std::any value) {
        // Check current scope
        auto it = values.find(name.lexeme);
        if (it != values.end()) {
            it->second = value;
            return;
        }
        
        // Check parent scopes
        if (enclosing != nullptr) {
            enclosing->assign(name, value);
            return;
        }
        
        // Not found in any scope
        throw std::runtime_error("Undefined variable '" + name.lexeme + "'.");
    }

    // Get variable exactly 'depth' scopes up — used by resolver-aware interpreter
    std::any getAt(int depth, const std::string& name) {
        return ancestor(depth)->values[name];
    }

    // Assign variable exactly 'depth' scopes up
    void assignAt(int depth, const Token& name, std::any value) {
        ancestor(depth)->values[name.lexeme] = value;
    }

private:
    // Walk exactly 'depth' steps up the enclosing chain
    Environment* ancestor(int depth) {
        Environment* env = this;
        for (int i = 0; i < depth; ++i) env = env->enclosing.get();
        return env;
    }
};
