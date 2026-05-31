#pragma once
#include "ast.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <map>
#include <string>
#include <vector>

// Lox runtime values are represented as tagged doubles (NaN boxing) at the LLVM level,
// but for simplicity this compiler uses a boxed representation:
//   - Numbers: double
//   - Booleans: i1
//   - Strings: i8*
//   - nil: special null ptr
// All values are passed as a generic { i32 tag, i64 payload } struct ("LoxValue").

class CodeGen {
public:
    CodeGen();
    void compile(const std::vector<StmtPtr>& program);
    void dumpIR();
    int writeObject(const std::string& filename);

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Types
    llvm::StructType* loxValueTy;   // { i32 tag, double numVal, i8* strVal }
    llvm::FunctionType* loxFunTy;   // common function signature

    // Tag constants
    enum LoxTag { TAG_NIL = 0, TAG_BOOL = 1, TAG_NUMBER = 2, TAG_STRING = 3 };

    // Variable storage: name → alloca
    struct Scope {
        std::map<std::string, llvm::AllocaInst*> vars;
    };
    std::vector<Scope> scopes;

    // Functions
    std::map<std::string, llvm::Function*> functions;
    llvm::Function* currentFn = nullptr;
    llvm::Function* printfFn = nullptr;
    llvm::Function* printLoxFn = nullptr;

    // Helpers
    void pushScope();
    void popScope();
    llvm::AllocaInst* findVar(const std::string& name);
    llvm::AllocaInst* createEntryAlloca(llvm::Function* fn, const std::string& name, llvm::Type* ty);

    // Value constructors
    llvm::Value* makeNumber(double val);
    llvm::Value* makeBool(bool val);
    llvm::Value* makeString(const std::string& val);
    llvm::Value* makeNil();

    // Value accessors
    llvm::Value* extractTag(llvm::Value* loxVal);
    llvm::Value* extractNumber(llvm::Value* loxVal);
    llvm::Value* extractBool(llvm::Value* loxVal);
    llvm::Value* extractString(llvm::Value* loxVal);
    llvm::Value* isTruthy(llvm::Value* loxVal);

    // Code generation
    void genStmt(Stmt* stmt);
    llvm::Value* genExpr(Expr* expr);
    void genBlock(const std::vector<StmtPtr>& stmts);

    void genPrint(PrintStmt* stmt);
    void genVar(VarStmt* stmt);
    void genIf(IfStmt* stmt);
    void genWhile(WhileStmt* stmt);
    void genFun(FunStmt* stmt);
    void genReturn(ReturnStmt* stmt);
    void genClass(ClassStmt* stmt);

    llvm::Value* genBinary(BinaryExpr* expr);
    llvm::Value* genUnary(UnaryExpr* expr);
    llvm::Value* genCall(CallExpr* expr);
    llvm::Value* genLogical(LogicalExpr* expr);

    // Wrap raw LLVM values into LoxValue structs
    llvm::Value* makeNumberFromValue(llvm::Value* num);
    llvm::Value* makeBoolFromValue(llvm::Value* b);

    // Runtime support
    void declareRuntime();
    void createPrintLox();
};
