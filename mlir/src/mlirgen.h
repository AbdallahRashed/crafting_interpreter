#pragma once
#include "ast.h"

#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/IR/OwningOpRef.h>
#include <mlir/IR/Value.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>

#include <map>
#include <string>
#include <vector>

/// Generates MLIR from the Lox AST using standard dialects (arith, func, scf, cf, memref).
/// All Lox values are represented as f64 (numbers, booleans as 0.0/1.0, nil as NaN sentinel).
/// Strings are not supported in this simplified MLIR backend.
class MLIRGen {
public:
    MLIRGen();
    void generate(const std::vector<StmtPtr>& program);
    void dump();
    int lowerAndEmit(const std::string& filename);
    int runJIT();

private:
    mlir::MLIRContext context;
    mlir::OpBuilder builder;
    mlir::OwningOpRef<mlir::ModuleOp> module;

    // Variable storage: name → memref alloc
    struct Scope {
        std::map<std::string, mlir::Value> vars;
    };
    std::vector<Scope> scopes;

    // Functions
    std::map<std::string, mlir::func::FuncOp> functions;
    mlir::func::FuncOp currentFunc;

    // Helpers
    void pushScope();
    void popScope();
    mlir::Value findVar(const std::string& name);
    mlir::Location loc(int line);

    // Special constants
    mlir::Value nilValue();
    mlir::Value boolValue(bool v);

    // Code generation
    void genStmt(Stmt* stmt);
    mlir::Value genExpr(Expr* expr);
    void genBlock(const std::vector<StmtPtr>& stmts);
};
