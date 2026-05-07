#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <any>
#include <stdexcept>
#include "Token.h"

class Environment
{
private:
std::unordered_map<std::string, std::any> values;    
public:
    
    void define(const std::string& name,std::any value)
    {
        values[name]=value;
    }
    std::any get(const Token& name) {
        // Check current scope
        auto it = values.find(name.lexeme);
        if (it != values.end()) {
            return it->second;
        }
        throw std::runtime_error("Undefined variable '" + name.lexeme + "'.");
    }
};