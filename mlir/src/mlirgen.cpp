#include "mlirgen.h"

#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/ControlFlow/IR/ControlFlow.h>
#include <mlir/Dialect/ControlFlow/IR/ControlFlowOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>
#include <mlir/Dialect/MemRef/IR/MemRef.h>
#include <mlir/Dialect/SCF/IR/SCF.h>

#include <mlir/Conversion/ArithToLLVM/ArithToLLVM.h>
#include <mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h>
#include <mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h>
#include <mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h>
#include <mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h>
#include <mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h>
#include <mlir/Conversion/Passes.h>

#include <mlir/ExecutionEngine/ExecutionEngine.h>
#include <mlir/ExecutionEngine/OptUtils.h>
#include <mlir/IR/BuiltinTypes.h>
#include <mlir/IR/Verifier.h>
#include <mlir/Pass/Pass.h>
#include <mlir/Pass/PassManager.h>
#include <mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h>
#include <mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h>
#include <mlir/Target/LLVMIR/Export.h>
#include <mlir/Transforms/Passes.h>

#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <cmath>

// Lox values encoded as f64:
// - Numbers: direct value
// - Booleans: 0.0 = false, 1.0 = true
// - nil: NaN (quiet NaN with specific payload)
static constexpr double NIL_VALUE = std::numeric_limits<double>::quiet_NaN();

MLIRGen::MLIRGen() : builder(&context) {
    context.loadDialect<mlir::arith::ArithDialect>();
    context.loadDialect<mlir::func::FuncDialect>();
    context.loadDialect<mlir::scf::SCFDialect>();
    context.loadDialect<mlir::cf::ControlFlowDialect>();
    context.loadDialect<mlir::memref::MemRefDialect>();
    context.loadDialect<mlir::LLVM::LLVMDialect>();

    module = mlir::ModuleOp::create(builder.getUnknownLoc());
}

mlir::Location MLIRGen::loc(int line) {
    return mlir::FileLineColLoc::get(builder.getStringAttr("lox"), line, 0);
}

void MLIRGen::pushScope() { scopes.push_back({}); }
void MLIRGen::popScope() { scopes.pop_back(); }

mlir::Value MLIRGen::findVar(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        auto it = scopes[i].vars.find(name);
        if (it != scopes[i].vars.end()) return it->second;
    }
    return nullptr;
}

mlir::Value MLIRGen::nilValue() {
    return builder.create<mlir::arith::ConstantFloatOp>(
        loc(0), llvm::APFloat(NIL_VALUE), builder.getF64Type());
}

mlir::Value MLIRGen::boolValue(bool v) {
    return builder.create<mlir::arith::ConstantFloatOp>(
        loc(0), llvm::APFloat(v ? 1.0 : 0.0), builder.getF64Type());
}

// ==================== Generation Entry ====================

void MLIRGen::generate(const std::vector<StmtPtr>& program) {
    builder.setInsertionPointToEnd(module->getBody());

    // Declare printf as an external function: (ptr) -> i32
    // We'll use LLVM dialect for printf declaration
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    auto i32Ty = mlir::IntegerType::get(&context, 32);
    auto printfType = mlir::LLVM::LLVMFunctionType::get(i32Ty, {ptrTy}, /*isVarArg=*/true);
    builder.create<mlir::LLVM::LLVMFuncOp>(loc(0), "printf", printfType);

    // Create main function: () -> i32
    auto mainType = builder.getFunctionType({}, {builder.getI32Type()});
    auto mainFn = mlir::func::FuncOp::create(loc(0), "main", mainType);
    module->push_back(mainFn);
    auto* entryBlock = mainFn.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);
    currentFunc = mainFn;

    pushScope();
    for (auto& stmt : program) genStmt(stmt.get());
    popScope();

    auto zero = builder.create<mlir::arith::ConstantIntOp>(loc(0), 0, 32);
    builder.create<mlir::func::ReturnOp>(loc(0), mlir::ValueRange{zero});
}

void MLIRGen::dump() {
    module->dump();
}

// ==================== Statement Generation ====================

void MLIRGen::genStmt(Stmt* stmt) {
    if (auto* s = dynamic_cast<PrintStmt*>(stmt)) {
        mlir::Value val = genExpr(s->expression.get());
        // Use LLVM dialect to call printf with "%g\n"
        auto location = loc(s->line);
        // Create global string for format
        auto parentModule = currentFunc->getParentOfType<mlir::ModuleOp>();
        mlir::LLVM::GlobalOp global;
        {
            // Check if we already created the format string
            global = parentModule.lookupSymbol<mlir::LLVM::GlobalOp>("printfmt");
            if (!global) {
                mlir::OpBuilder::InsertionGuard guard(builder);
                builder.setInsertionPointToStart(parentModule.getBody());
                auto strType = mlir::LLVM::LLVMArrayType::get(
                    mlir::IntegerType::get(&context, 8), 4); // "%g\n\0"
                global = builder.create<mlir::LLVM::GlobalOp>(
                    location, strType, /*isConstant=*/true,
                    mlir::LLVM::Linkage::Private, "printfmt",
                    builder.getStringAttr(llvm::StringRef("%g\n\0", 4)));
            }
        }
        // Get pointer to format string
        auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
        auto fmtPtr = builder.create<mlir::LLVM::AddressOfOp>(location, ptrTy, "printfmt");

        // Call printf — need to convert f64 val via unrealized conversion to work with LLVM call
        // Since we're mixing func dialect with LLVM dialect calls, use LLVM::CallOp
        auto i32Ty = builder.getI32Type();
        auto printfType = mlir::LLVM::LLVMFunctionType::get(i32Ty, {ptrTy}, /*isVarArg=*/true);
        auto callOp = builder.create<mlir::LLVM::CallOp>(
            location, mlir::TypeRange{i32Ty}, "printf",
            mlir::ValueRange{fmtPtr, val});
        callOp.setVarCalleeType(printfType);
        return;
    }

    if (auto* s = dynamic_cast<VarStmt*>(stmt)) {
        mlir::Value init = s->initializer ? genExpr(s->initializer.get()) : nilValue();
        // Use memref.alloca for mutable variables
        auto memrefTy = mlir::MemRefType::get({}, builder.getF64Type());
        auto alloca = builder.create<mlir::memref::AllocaOp>(loc(s->line), memrefTy);
        builder.create<mlir::memref::StoreOp>(loc(s->line), init, alloca);
        scopes.back().vars[s->name] = alloca;
        return;
    }

    if (auto* s = dynamic_cast<ExprStmt*>(stmt)) {
        genExpr(s->expression.get());
        return;
    }

    if (auto* s = dynamic_cast<BlockStmt*>(stmt)) {
        pushScope();
        genBlock(s->statements);
        popScope();
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(stmt)) {
        mlir::Value cond = genExpr(s->condition.get());
        // truthy: != 0.0 and not NaN
        auto zero = builder.create<mlir::arith::ConstantFloatOp>(
            loc(s->line), llvm::APFloat(0.0), builder.getF64Type());
        auto cmp = builder.create<mlir::arith::CmpFOp>(
            loc(s->line), mlir::arith::CmpFPredicate::ONE, cond, zero);

        // Create blocks for then/else/merge
        auto* thenBlock = currentFunc.addBlock();
        auto* elseBlock = currentFunc.addBlock();
        auto* mergeBlock = currentFunc.addBlock();

        builder.create<mlir::cf::CondBranchOp>(loc(s->line), cmp, thenBlock, elseBlock);

        // Then
        builder.setInsertionPointToStart(thenBlock);
        genStmt(s->thenBranch.get());
        if (!thenBlock->back().hasTrait<mlir::OpTrait::IsTerminator>())
            builder.create<mlir::cf::BranchOp>(loc(s->line), mergeBlock);

        // Else
        builder.setInsertionPointToStart(elseBlock);
        if (s->elseBranch) genStmt(s->elseBranch.get());
        if (!elseBlock->back().hasTrait<mlir::OpTrait::IsTerminator>())
            builder.create<mlir::cf::BranchOp>(loc(s->line), mergeBlock);

        builder.setInsertionPointToStart(mergeBlock);
        return;
    }

    if (auto* s = dynamic_cast<WhileStmt*>(stmt)) {
        auto* condBlock = currentFunc.addBlock();
        auto* bodyBlock = currentFunc.addBlock();
        auto* endBlock = currentFunc.addBlock();

        builder.create<mlir::cf::BranchOp>(loc(s->line), condBlock);

        builder.setInsertionPointToStart(condBlock);
        mlir::Value cond = genExpr(s->condition.get());
        auto zero = builder.create<mlir::arith::ConstantFloatOp>(
            loc(s->line), llvm::APFloat(0.0), builder.getF64Type());
        auto cmp = builder.create<mlir::arith::CmpFOp>(
            loc(s->line), mlir::arith::CmpFPredicate::ONE, cond, zero);
        builder.create<mlir::cf::CondBranchOp>(loc(s->line), cmp, bodyBlock, endBlock);

        builder.setInsertionPointToStart(bodyBlock);
        genStmt(s->body.get());
        if (bodyBlock->empty() || !bodyBlock->back().hasTrait<mlir::OpTrait::IsTerminator>())
            builder.create<mlir::cf::BranchOp>(loc(s->line), condBlock);

        builder.setInsertionPointToStart(endBlock);
        return;
    }

    if (auto* s = dynamic_cast<FunStmt*>(stmt)) {
        // Create function
        std::vector<mlir::Type> paramTypes(s->params.size(), builder.getF64Type());
        auto fnType = builder.getFunctionType(paramTypes, {builder.getF64Type()});

        mlir::OpBuilder::InsertionGuard guard(builder);
        builder.setInsertionPointToEnd(module->getBody());
        auto fn = mlir::func::FuncOp::create(loc(s->line), s->name, fnType);
        module->push_back(fn);
        functions[s->name] = fn;

        auto* entry = fn.addEntryBlock();
        builder.setInsertionPointToStart(entry);

        auto prevFunc = currentFunc;
        currentFunc = fn;
        pushScope();

        for (size_t i = 0; i < s->params.size(); i++) {
            auto memrefTy = mlir::MemRefType::get({}, builder.getF64Type());
            auto alloca = builder.create<mlir::memref::AllocaOp>(loc(s->line), memrefTy);
            builder.create<mlir::memref::StoreOp>(loc(s->line), entry->getArgument(i), alloca);
            scopes.back().vars[s->params[i]] = alloca;
        }

        genBlock(s->body);

        // Default return nil
        if (entry->empty() || !fn.getBody().back().back().hasTrait<mlir::OpTrait::IsTerminator>()) {
            builder.create<mlir::func::ReturnOp>(loc(s->line), mlir::ValueRange{nilValue()});
        }

        popScope();
        currentFunc = prevFunc;
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(stmt)) {
        mlir::Value val = s->value ? genExpr(s->value.get()) : nilValue();
        builder.create<mlir::func::ReturnOp>(loc(s->line), mlir::ValueRange{val});
        return;
    }
}

void MLIRGen::genBlock(const std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts) genStmt(s.get());
}

// ==================== Expression Generation ====================

mlir::Value MLIRGen::genExpr(Expr* expr) {
    if (auto* e = dynamic_cast<NumberExpr*>(expr)) {
        return builder.create<mlir::arith::ConstantFloatOp>(
            loc(e->line), llvm::APFloat(e->value), builder.getF64Type());
    }

    if (auto* e = dynamic_cast<BoolExpr*>(expr)) {
        return boolValue(e->value);
    }

    if (dynamic_cast<NilExpr*>(expr)) {
        return nilValue();
    }

    if (auto* e = dynamic_cast<StringExpr*>(expr)) {
        // Strings not fully supported — return nil
        (void)e;
        return nilValue();
    }

    if (auto* e = dynamic_cast<IdentifierExpr*>(expr)) {
        mlir::Value memref = findVar(e->name);
        if (memref)
            return builder.create<mlir::memref::LoadOp>(loc(e->line), memref);
        return nilValue();
    }

    if (auto* e = dynamic_cast<AssignExpr*>(expr)) {
        mlir::Value val = genExpr(e->value.get());
        mlir::Value memref = findVar(e->name);
        if (memref)
            builder.create<mlir::memref::StoreOp>(loc(e->line), val, memref);
        return val;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr)) {
        mlir::Value left = genExpr(e->left.get());
        mlir::Value right = genExpr(e->right.get());
        auto location = loc(e->line);

        switch (e->op) {
            case TokenType::PLUS:
                return builder.create<mlir::arith::AddFOp>(location, left, right);
            case TokenType::MINUS:
                return builder.create<mlir::arith::SubFOp>(location, left, right);
            case TokenType::STAR:
                return builder.create<mlir::arith::MulFOp>(location, left, right);
            case TokenType::SLASH:
                return builder.create<mlir::arith::DivFOp>(location, left, right);
            case TokenType::GREATER: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::OGT, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            case TokenType::GREATER_EQUAL: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::OGE, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            case TokenType::LESS: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::OLT, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            case TokenType::LESS_EQUAL: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::OLE, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            case TokenType::EQUAL_EQUAL: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::OEQ, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            case TokenType::BANG_EQUAL: {
                auto cmp = builder.create<mlir::arith::CmpFOp>(
                    location, mlir::arith::CmpFPredicate::ONE, left, right);
                return builder.create<mlir::arith::UIToFPOp>(location, builder.getF64Type(), cmp);
            }
            default: return nilValue();
        }
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(expr)) {
        mlir::Value operand = genExpr(e->operand.get());
        if (e->op == TokenType::MINUS)
            return builder.create<mlir::arith::NegFOp>(loc(e->line), operand);
        if (e->op == TokenType::BANG) {
            // !x: x == 0.0 → 1.0, else → 0.0
            auto zero = builder.create<mlir::arith::ConstantFloatOp>(
                loc(e->line), llvm::APFloat(0.0), builder.getF64Type());
            auto cmp = builder.create<mlir::arith::CmpFOp>(
                loc(e->line), mlir::arith::CmpFPredicate::OEQ, operand, zero);
            return builder.create<mlir::arith::UIToFPOp>(loc(e->line), builder.getF64Type(), cmp);
        }
        return nilValue();
    }

    if (auto* e = dynamic_cast<CallExpr*>(expr)) {
        auto* id = dynamic_cast<IdentifierExpr*>(e->callee.get());
        if (!id) return nilValue();

        std::vector<mlir::Value> args;
        for (auto& a : e->args) args.push_back(genExpr(a.get()));

        auto it = functions.find(id->name);
        if (it != functions.end()) {
            auto call = builder.create<mlir::func::CallOp>(
                loc(e->line), it->second, args);
            return call.getResult(0);
        }
        return nilValue();
    }

    if (auto* e = dynamic_cast<LogicalExpr*>(expr)) {
        // Simplified: evaluate both and select
        mlir::Value left = genExpr(e->left.get());
        mlir::Value right = genExpr(e->right.get());
        auto zero = builder.create<mlir::arith::ConstantFloatOp>(
            loc(e->line), llvm::APFloat(0.0), builder.getF64Type());
        auto leftTruthy = builder.create<mlir::arith::CmpFOp>(
            loc(e->line), mlir::arith::CmpFPredicate::ONE, left, zero);

        if (e->op == TokenType::OR) {
            // or: if left truthy, return left; else return right
            return builder.create<mlir::arith::SelectOp>(loc(e->line), leftTruthy, left, right);
        } else {
            // and: if left truthy, return right; else return left
            return builder.create<mlir::arith::SelectOp>(loc(e->line), leftTruthy, right, left);
        }
    }

    return nilValue();
}

// ==================== Lowering & Emission ====================

int MLIRGen::lowerAndEmit(const std::string& filename) {
    // Verify the module first
    if (mlir::failed(mlir::verify(*module))) {
        std::cerr << "MLIR module verification failed\n";
        module->dump();
        return 1;
    }

    // Lower: SCF → CF, then all to LLVM
    mlir::PassManager pm(&context);
    pm.addPass(mlir::createConvertSCFToCFPass());
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());
    pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
    pm.addPass(mlir::createConvertFuncToLLVMPass());
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    if (mlir::failed(pm.run(*module))) {
        std::cerr << "MLIR pass pipeline failed\n";
        module->dump();
        return 1;
    }

    // Translate to LLVM IR
    mlir::registerLLVMDialectTranslation(context);
    mlir::registerBuiltinDialectTranslation(context);
    llvm::LLVMContext llvmContext;
    auto llvmModule = mlir::translateModuleToLLVMIR(*module, llvmContext);
    if (!llvmModule) {
        std::cerr << "Failed to translate MLIR to LLVM IR\n";
        return 1;
    }

    // Print LLVM IR (for now, until we add object file emission)
    std::error_code ec;
    llvm::raw_fd_ostream out(filename, ec);
    if (ec) {
        std::cerr << "Cannot open output: " << ec.message() << "\n";
        return 1;
    }
    llvmModule->print(out, nullptr);
    return 0;
}

int MLIRGen::runJIT() {
    if (mlir::failed(mlir::verify(*module))) {
        std::cerr << "MLIR module verification failed\n";
        return 1;
    }

    // Lower everything
    mlir::PassManager pm(&context);
    pm.addPass(mlir::createConvertSCFToCFPass());
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());
    pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
    pm.addPass(mlir::createConvertFuncToLLVMPass());
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    if (mlir::failed(pm.run(*module))) {
        std::cerr << "Lowering failed\n";
        return 1;
    }

    mlir::registerLLVMDialectTranslation(context);
    mlir::registerBuiltinDialectTranslation(context);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto maybeEngine = mlir::ExecutionEngine::create(*module);
    if (!maybeEngine) {
        std::cerr << "Failed to create execution engine\n";
        return 1;
    }

    auto& engine = *maybeEngine;
    auto result = engine->invokePacked("main");
    if (result) {
        std::cerr << "JIT execution failed\n";
        return 1;
    }
    return 0;
}
