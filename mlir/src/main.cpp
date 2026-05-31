#include "scanner.h"
#include "ast.h"
#include "mlirgen.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file '" << path << "'\n";
        exit(1);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: lox-mlir <script.lox> [--dump-mlir] [--emit-llvm output.ll] [--jit]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    bool dumpMLIR = false;
    bool jit = false;
    std::string llvmOutput;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dump-mlir") dumpMLIR = true;
        else if (arg == "--jit") jit = true;
        else if (arg == "--emit-llvm" && i + 1 < argc) llvmOutput = argv[++i];
    }

    std::string source = readFile(inputFile);

    Scanner scanner(source);
    auto tokens = scanner.scanTokens();

    Parser parser(tokens);
    std::vector<StmtPtr> program;
    try {
        program = parser.parse();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    MLIRGen gen;
    gen.generate(program);

    if (dumpMLIR) {
        gen.dump();
        return 0;
    }

    if (!llvmOutput.empty()) {
        return gen.lowerAndEmit(llvmOutput);
    }

    if (jit) {
        return gen.runJIT();
    }

    // Default: dump MLIR
    gen.dump();
    return 0;
}
