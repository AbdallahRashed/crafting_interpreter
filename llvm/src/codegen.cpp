#include "codegen.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include <iostream>

CodeGen::CodeGen() {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("lox", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    // LoxValue = { i32 tag, double numVal, i8* strVal }
    loxValueTy = llvm::StructType::create(*context,
        {llvm::Type::getInt32Ty(*context),
         llvm::Type::getDoubleTy(*context),
         llvm::PointerType::getUnqual(*context)},
        "LoxValue");

    declareRuntime();
    createPrintLox();
}

void CodeGen::declareRuntime() {
    // printf
    auto* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        {llvm::PointerType::getUnqual(*context)},
        true);
    printfFn = llvm::Function::Create(printfType,
        llvm::Function::ExternalLinkage, "printf", module.get());
}

void CodeGen::createPrintLox() {
    // printLox(LoxValue val) — prints based on tag
    auto* fnType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context), {loxValueTy}, false);
    printLoxFn = llvm::Function::Create(fnType,
        llvm::Function::InternalLinkage, "printLox", module.get());

    auto* entry = llvm::BasicBlock::Create(*context, "entry", printLoxFn);
    auto* numBB = llvm::BasicBlock::Create(*context, "num", printLoxFn);
    auto* boolBB = llvm::BasicBlock::Create(*context, "bool", printLoxFn);
    auto* strBB = llvm::BasicBlock::Create(*context, "str", printLoxFn);
    auto* nilBB = llvm::BasicBlock::Create(*context, "nil", printLoxFn);
    auto* endBB = llvm::BasicBlock::Create(*context, "end", printLoxFn);

    builder->SetInsertPoint(entry);
    llvm::Value* val = printLoxFn->getArg(0);
    llvm::Value* tag = builder->CreateExtractValue(val, {0}, "tag");

    auto* sw = builder->CreateSwitch(tag, nilBB, 3);
    sw->addCase(builder->getInt32(TAG_NUMBER), numBB);
    sw->addCase(builder->getInt32(TAG_BOOL), boolBB);
    sw->addCase(builder->getInt32(TAG_STRING), strBB);

    // Number
    builder->SetInsertPoint(numBB);
    llvm::Value* numVal = builder->CreateExtractValue(val, {1}, "numval");
    auto* numFmt = builder->CreateGlobalString("%g\n", "numfmt");
    builder->CreateCall(printfFn, {numFmt, numVal});
    builder->CreateBr(endBB);

    // Bool
    builder->SetInsertPoint(boolBB);
    llvm::Value* boolVal = builder->CreateExtractValue(val, {1}, "boolraw");
    llvm::Value* boolInt = builder->CreateFPToUI(boolVal, llvm::Type::getInt1Ty(*context));
    auto* trueFmt = builder->CreateGlobalString("true\n", "truefmt");
    auto* falseFmt = builder->CreateGlobalString("false\n", "falsefmt");
    llvm::Value* boolStr = builder->CreateSelect(boolInt, trueFmt, falseFmt);
    builder->CreateCall(printfFn, {boolStr});
    builder->CreateBr(endBB);

    // String
    builder->SetInsertPoint(strBB);
    llvm::Value* strPtr = builder->CreateExtractValue(val, {2}, "strval");
    auto* strFmt = builder->CreateGlobalString("%s\n", "strfmt");
    builder->CreateCall(printfFn, {strFmt, strPtr});
    builder->CreateBr(endBB);

    // Nil
    builder->SetInsertPoint(nilBB);
    auto* nilFmt = builder->CreateGlobalString("nil\n", "nilfmt");
    builder->CreateCall(printfFn, {nilFmt});
    builder->CreateBr(endBB);

    // End
    builder->SetInsertPoint(endBB);
    builder->CreateRetVoid();
}

// ==================== Value Constructors ====================

llvm::Value* CodeGen::makeNumber(double val) {
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_NUMBER), {0});
    result = builder->CreateInsertValue(result, llvm::ConstantFP::get(*context, llvm::APFloat(val)), {1});
    result = builder->CreateInsertValue(result, llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)), {2});
    return result;
}

llvm::Value* CodeGen::makeBool(bool val) {
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_BOOL), {0});
    result = builder->CreateInsertValue(result, llvm::ConstantFP::get(*context, llvm::APFloat(val ? 1.0 : 0.0)), {1});
    result = builder->CreateInsertValue(result, llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)), {2});
    return result;
}

llvm::Value* CodeGen::makeString(const std::string& val) {
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_STRING), {0});
    result = builder->CreateInsertValue(result, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), {1});
    result = builder->CreateInsertValue(result, builder->CreateGlobalString(val), {2});
    return result;
}

llvm::Value* CodeGen::makeNil() {
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_NIL), {0});
    result = builder->CreateInsertValue(result, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), {1});
    result = builder->CreateInsertValue(result, llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)), {2});
    return result;
}

// ==================== Value Accessors ====================

llvm::Value* CodeGen::extractTag(llvm::Value* loxVal) {
    return builder->CreateExtractValue(loxVal, {0}, "tag");
}

llvm::Value* CodeGen::extractNumber(llvm::Value* loxVal) {
    return builder->CreateExtractValue(loxVal, {1}, "num");
}

llvm::Value* CodeGen::extractBool(llvm::Value* loxVal) {
    llvm::Value* raw = builder->CreateExtractValue(loxVal, {1});
    return builder->CreateFCmpONE(raw, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "boolval");
}

llvm::Value* CodeGen::extractString(llvm::Value* loxVal) {
    return builder->CreateExtractValue(loxVal, {2}, "str");
}

llvm::Value* CodeGen::isTruthy(llvm::Value* loxVal) {
    llvm::Value* tag = extractTag(loxVal);
    // nil is falsey
    llvm::Value* isNil = builder->CreateICmpEQ(tag, builder->getInt32(TAG_NIL));
    // false is falsey
    llvm::Value* isBool = builder->CreateICmpEQ(tag, builder->getInt32(TAG_BOOL));
    llvm::Value* boolVal = extractBool(loxVal);
    llvm::Value* isFalseBool = builder->CreateAnd(isBool, builder->CreateNot(boolVal));
    // falsey = nil or (bool and !value)
    llvm::Value* falsey = builder->CreateOr(isNil, isFalseBool);
    return builder->CreateNot(falsey, "truthy");
}

// ==================== Scope ====================

void CodeGen::pushScope() { scopes.push_back({}); }
void CodeGen::popScope() { scopes.pop_back(); }

llvm::AllocaInst* CodeGen::findVar(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        auto it = scopes[i].vars.find(name);
        if (it != scopes[i].vars.end()) return it->second;
    }
    return nullptr;
}

llvm::AllocaInst* CodeGen::createEntryAlloca(llvm::Function* fn, const std::string& name, llvm::Type* ty) {
    llvm::IRBuilder<> tmpB(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmpB.CreateAlloca(ty, nullptr, name);
}

// ==================== Compilation Entry ====================

void CodeGen::compile(const std::vector<StmtPtr>& program) {
    // Create main function
    auto* mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), false);
    auto* mainFn = llvm::Function::Create(mainType,
        llvm::Function::ExternalLinkage, "main", module.get());
    auto* entry = llvm::BasicBlock::Create(*context, "entry", mainFn);
    builder->SetInsertPoint(entry);
    currentFn = mainFn;

    pushScope();
    for (auto& stmt : program) genStmt(stmt.get());
    popScope();

    builder->CreateRet(builder->getInt32(0));
    llvm::verifyFunction(*mainFn);
}

void CodeGen::dumpIR() {
    module->print(llvm::outs(), nullptr);
}

int CodeGen::writeObject(const std::string& filename) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto tm = target->createTargetMachine(targetTriple, cpu, features, opt, llvm::Reloc::PIC_);
    module->setDataLayout(tm->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << "\n";
        return 1;
    }

    llvm::legacy::PassManager pass;
    if (tm->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "Target machine can't emit object file\n";
        return 1;
    }

    pass.run(*module);
    dest.flush();
    return 0;
}

// ==================== Statement Generation ====================

void CodeGen::genStmt(Stmt* stmt) {
    if (auto* s = dynamic_cast<PrintStmt*>(stmt)) return genPrint(s);
    if (auto* s = dynamic_cast<VarStmt*>(stmt)) return genVar(s);
    if (auto* s = dynamic_cast<ExprStmt*>(stmt)) { genExpr(s->expression.get()); return; }
    if (auto* s = dynamic_cast<BlockStmt*>(stmt)) { pushScope(); genBlock(s->statements); popScope(); return; }
    if (auto* s = dynamic_cast<IfStmt*>(stmt)) return genIf(s);
    if (auto* s = dynamic_cast<WhileStmt*>(stmt)) return genWhile(s);
    if (auto* s = dynamic_cast<FunStmt*>(stmt)) return genFun(s);
    if (auto* s = dynamic_cast<ReturnStmt*>(stmt)) return genReturn(s);
    if (auto* s = dynamic_cast<ClassStmt*>(stmt)) return genClass(s);
}

void CodeGen::genBlock(const std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts) genStmt(s.get());
}

void CodeGen::genPrint(PrintStmt* stmt) {
    llvm::Value* val = genExpr(stmt->expression.get());
    builder->CreateCall(printLoxFn, {val});
}

void CodeGen::genVar(VarStmt* stmt) {
    llvm::Value* init = stmt->initializer ? genExpr(stmt->initializer.get()) : makeNil();
    auto* alloca = createEntryAlloca(currentFn, stmt->name, loxValueTy);
    builder->CreateStore(init, alloca);
    scopes.back().vars[stmt->name] = alloca;
}

void CodeGen::genIf(IfStmt* stmt) {
    llvm::Value* cond = isTruthy(genExpr(stmt->condition.get()));
    auto* thenBB = llvm::BasicBlock::Create(*context, "then", currentFn);
    auto* elseBB = llvm::BasicBlock::Create(*context, "else", currentFn);
    auto* mergeBB = llvm::BasicBlock::Create(*context, "ifcont", currentFn);

    builder->CreateCondBr(cond, thenBB, elseBB);

    builder->SetInsertPoint(thenBB);
    genStmt(stmt->thenBranch.get());
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(mergeBB);

    builder->SetInsertPoint(elseBB);
    if (stmt->elseBranch) genStmt(stmt->elseBranch.get());
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(mergeBB);

    builder->SetInsertPoint(mergeBB);
}

void CodeGen::genWhile(WhileStmt* stmt) {
    auto* condBB = llvm::BasicBlock::Create(*context, "whilecond", currentFn);
    auto* bodyBB = llvm::BasicBlock::Create(*context, "whilebody", currentFn);
    auto* endBB = llvm::BasicBlock::Create(*context, "whileend", currentFn);

    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);
    llvm::Value* cond = isTruthy(genExpr(stmt->condition.get()));
    builder->CreateCondBr(cond, bodyBB, endBB);

    builder->SetInsertPoint(bodyBB);
    genStmt(stmt->body.get());
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(condBB);

    builder->SetInsertPoint(endBB);
}

void CodeGen::genFun(FunStmt* stmt) {
    // Create function type: (LoxValue, LoxValue, ...) -> LoxValue
    std::vector<llvm::Type*> paramTypes(stmt->params.size(), loxValueTy);
    auto* fnType = llvm::FunctionType::get(loxValueTy, paramTypes, false);
    auto* fn = llvm::Function::Create(fnType,
        llvm::Function::ExternalLinkage, stmt->name, module.get());
    functions[stmt->name] = fn;

    // Save state
    auto* prevFn = currentFn;
    auto* prevBB = builder->GetInsertBlock();

    currentFn = fn;
    auto* entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    pushScope();
    int i = 0;
    for (auto& arg : fn->args()) {
        arg.setName(stmt->params[i]);
        auto* alloca = createEntryAlloca(fn, stmt->params[i], loxValueTy);
        builder->CreateStore(&arg, alloca);
        scopes.back().vars[stmt->params[i]] = alloca;
        i++;
    }

    genBlock(stmt->body);

    // If no explicit return, return nil
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateRet(makeNil());

    llvm::verifyFunction(*fn);

    // Restore state
    currentFn = prevFn;
    builder->SetInsertPoint(prevBB);

    // Store function pointer as a variable in enclosing scope (for calling)
    // We don't do full closures here — functions are global
}

void CodeGen::genReturn(ReturnStmt* stmt) {
    llvm::Value* val = stmt->value ? genExpr(stmt->value.get()) : makeNil();
    builder->CreateRet(val);
}

void CodeGen::genClass(ClassStmt* stmt) {
    // Simplified: just compile methods as standalone functions prefixed with class name
    for (auto& method : stmt->methods) {
        auto mangledName = stmt->name + "." + method->name;
        std::vector<llvm::Type*> paramTypes;
        paramTypes.push_back(loxValueTy); // this
        for (size_t i = 0; i < method->params.size(); i++)
            paramTypes.push_back(loxValueTy);

        auto* fnType = llvm::FunctionType::get(loxValueTy, paramTypes, false);
        auto* fn = llvm::Function::Create(fnType,
            llvm::Function::ExternalLinkage, mangledName, module.get());
        functions[mangledName] = fn;

        auto* prevFn = currentFn;
        auto* prevBB = builder->GetInsertBlock();
        currentFn = fn;
        auto* entry = llvm::BasicBlock::Create(*context, "entry", fn);
        builder->SetInsertPoint(entry);

        pushScope();
        auto argIt = fn->arg_begin();
        argIt->setName("this");
        auto* thisAlloca = createEntryAlloca(fn, "this", loxValueTy);
        builder->CreateStore(&*argIt, thisAlloca);
        scopes.back().vars["this"] = thisAlloca;
        argIt++;

        for (size_t i = 0; i < method->params.size(); i++, argIt++) {
            argIt->setName(method->params[i]);
            auto* alloca = createEntryAlloca(fn, method->params[i], loxValueTy);
            builder->CreateStore(&*argIt, alloca);
            scopes.back().vars[method->params[i]] = alloca;
        }

        genBlock(method->body);
        if (!builder->GetInsertBlock()->getTerminator())
            builder->CreateRet(makeNil());
        llvm::verifyFunction(*fn);
        popScope();

        currentFn = prevFn;
        builder->SetInsertPoint(prevBB);
    }
}

// ==================== Expression Generation ====================

llvm::Value* CodeGen::genExpr(Expr* expr) {
    if (auto* e = dynamic_cast<NumberExpr*>(expr)) return makeNumber(e->value);
    if (auto* e = dynamic_cast<StringExpr*>(expr)) return makeString(e->value);
    if (auto* e = dynamic_cast<BoolExpr*>(expr)) return makeBool(e->value);
    if (dynamic_cast<NilExpr*>(expr)) return makeNil();

    if (auto* e = dynamic_cast<IdentifierExpr*>(expr)) {
        auto* alloca = findVar(e->name);
        if (alloca) return builder->CreateLoad(loxValueTy, alloca, e->name);
        // Might be a function — return nil for now
        return makeNil();
    }

    if (auto* e = dynamic_cast<AssignExpr*>(expr)) {
        llvm::Value* val = genExpr(e->value.get());
        auto* alloca = findVar(e->name);
        if (alloca) builder->CreateStore(val, alloca);
        return val;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr)) return genBinary(e);
    if (auto* e = dynamic_cast<UnaryExpr*>(expr)) return genUnary(e);
    if (auto* e = dynamic_cast<CallExpr*>(expr)) return genCall(e);
    if (auto* e = dynamic_cast<LogicalExpr*>(expr)) return genLogical(e);

    return makeNil();
}

llvm::Value* CodeGen::genBinary(BinaryExpr* expr) {
    llvm::Value* left = genExpr(expr->left.get());
    llvm::Value* right = genExpr(expr->right.get());

    llvm::Value* lNum = extractNumber(left);
    llvm::Value* rNum = extractNumber(right);

    switch (expr->op) {
        case TokenType::PLUS: {
            // Check if strings — for now just do number addition
            llvm::Value* lTag = extractTag(left);
            llvm::Value* isStr = builder->CreateICmpEQ(lTag, builder->getInt32(TAG_STRING));

            // Simple approach: assume numbers for arithmetic
            llvm::Value* sum = builder->CreateFAdd(lNum, rNum, "addtmp");
            // TODO: string concatenation
            (void)isStr;
            return makeNumberFromValue(sum);
        }
        case TokenType::MINUS: {
            llvm::Value* res = builder->CreateFSub(lNum, rNum, "subtmp");
            return makeNumberFromValue(res);
        }
        case TokenType::STAR: {
            llvm::Value* res = builder->CreateFMul(lNum, rNum, "multmp");
            return makeNumberFromValue(res);
        }
        case TokenType::SLASH: {
            llvm::Value* res = builder->CreateFDiv(lNum, rNum, "divtmp");
            return makeNumberFromValue(res);
        }
        case TokenType::GREATER: {
            llvm::Value* cmp = builder->CreateFCmpOGT(lNum, rNum, "gttmp");
            return makeBoolFromValue(cmp);
        }
        case TokenType::GREATER_EQUAL: {
            llvm::Value* cmp = builder->CreateFCmpOGE(lNum, rNum, "getmp");
            return makeBoolFromValue(cmp);
        }
        case TokenType::LESS: {
            llvm::Value* cmp = builder->CreateFCmpOLT(lNum, rNum, "lttmp");
            return makeBoolFromValue(cmp);
        }
        case TokenType::LESS_EQUAL: {
            llvm::Value* cmp = builder->CreateFCmpOLE(lNum, rNum, "letmp");
            return makeBoolFromValue(cmp);
        }
        case TokenType::EQUAL_EQUAL: {
            llvm::Value* cmp = builder->CreateFCmpOEQ(lNum, rNum, "eqtmp");
            return makeBoolFromValue(cmp);
        }
        case TokenType::BANG_EQUAL: {
            llvm::Value* cmp = builder->CreateFCmpONE(lNum, rNum, "netmp");
            return makeBoolFromValue(cmp);
        }
        default: return makeNil();
    }
}

llvm::Value* CodeGen::genUnary(UnaryExpr* expr) {
    llvm::Value* operand = genExpr(expr->operand.get());
    if (expr->op == TokenType::MINUS) {
        llvm::Value* num = extractNumber(operand);
        llvm::Value* neg = builder->CreateFNeg(num, "negtmp");
        return makeNumberFromValue(neg);
    }
    if (expr->op == TokenType::BANG) {
        llvm::Value* truthy = isTruthy(operand);
        llvm::Value* notVal = builder->CreateNot(truthy);
        return makeBoolFromValue(notVal);
    }
    return makeNil();
}

llvm::Value* CodeGen::genCall(CallExpr* expr) {
    auto* id = dynamic_cast<IdentifierExpr*>(expr->callee.get());
    if (!id) return makeNil(); // only support direct calls

    auto it = functions.find(id->name);
    if (it == functions.end()) {
        // Try to find it in module
        auto* fn = module->getFunction(id->name);
        if (!fn) return makeNil();
        std::vector<llvm::Value*> args;
        for (auto& a : expr->args) args.push_back(genExpr(a.get()));
        return builder->CreateCall(fn, args, "calltmp");
    }

    std::vector<llvm::Value*> args;
    for (auto& a : expr->args) args.push_back(genExpr(a.get()));
    return builder->CreateCall(it->second, args, "calltmp");
}

llvm::Value* CodeGen::genLogical(LogicalExpr* expr) {
    // Use alloca for result to avoid phi on struct types
    auto* resultAlloca = createEntryAlloca(currentFn, "logresult", loxValueTy);

    llvm::Value* left = genExpr(expr->left.get());
    builder->CreateStore(left, resultAlloca);
    llvm::Value* leftTruthy = isTruthy(left);

    auto* rhsBB = llvm::BasicBlock::Create(*context, "logrhs", currentFn);
    auto* mergeBB = llvm::BasicBlock::Create(*context, "logmerge", currentFn);

    if (expr->op == TokenType::OR) {
        // OR: if left is truthy, short-circuit (keep left); else evaluate right
        builder->CreateCondBr(leftTruthy, mergeBB, rhsBB);
    } else {
        // AND: if left is falsey, short-circuit (keep left); else evaluate right
        builder->CreateCondBr(leftTruthy, rhsBB, mergeBB);
    }

    builder->SetInsertPoint(rhsBB);
    llvm::Value* right = genExpr(expr->right.get());
    builder->CreateStore(right, resultAlloca);
    builder->CreateBr(mergeBB);

    builder->SetInsertPoint(mergeBB);
    return builder->CreateLoad(loxValueTy, resultAlloca, "logval");
}

// Helper: wrap a raw double Value into a LoxValue
llvm::Value* CodeGen::makeNumberFromValue(llvm::Value* num) {
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_NUMBER), {0});
    result = builder->CreateInsertValue(result, num, {1});
    result = builder->CreateInsertValue(result, llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)), {2});
    return result;
}

// Helper: wrap a raw i1 into a LoxValue
llvm::Value* CodeGen::makeBoolFromValue(llvm::Value* b) {
    llvm::Value* asDouble = builder->CreateUIToFP(b, llvm::Type::getDoubleTy(*context));
    llvm::Value* result = llvm::UndefValue::get(loxValueTy);
    result = builder->CreateInsertValue(result, builder->getInt32(TAG_BOOL), {0});
    result = builder->CreateInsertValue(result, asDouble, {1});
    result = builder->CreateInsertValue(result, llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)), {2});
    return result;
}
