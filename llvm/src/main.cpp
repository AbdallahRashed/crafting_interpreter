#include "scanner.h"
#include "ast.h"
#include "codegen.h"

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
        std::cerr << "Usage: loxc <script.lox> [--emit-ir] [-o output]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    bool emitIR = false;
    std::string outputFile = "output.o";

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--emit-ir") emitIR = true;
        else if (arg == "-o" && i + 1 < argc) outputFile = argv[++i];
    }

    // Read source
    std::string source = readFile(inputFile);

    // Scan
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();

    // Parse
    Parser parser(tokens);
    std::vector<StmtPtr> program;
    try {
        program = parser.parse();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // Codegen
    CodeGen codegen;
    codegen.compile(program);

    if (emitIR) {
        codegen.dumpIR();
        return 0;
    }

    // Write object file
    int rc = codegen.writeObject(outputFile);
    if (rc != 0) return rc;

    std::cout << "Wrote " << outputFile << "\n";
    return 0;
}
