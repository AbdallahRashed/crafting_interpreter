CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Build directory
BUILD_DIR = build

# Object files
SCANNER_OBJS = $(BUILD_DIR)/TokenType.o $(BUILD_DIR)/Scanner.o $(BUILD_DIR)/main.o
PARSER_OBJS = $(BUILD_DIR)/TokenType.o $(BUILD_DIR)/Scanner.o $(BUILD_DIR)/Parser.o $(BUILD_DIR)/parser_test.o
INTERPRETER_OBJS = $(BUILD_DIR)/TokenType.o $(BUILD_DIR)/Scanner.o $(BUILD_DIR)/Parser.o $(BUILD_DIR)/Interpreter.o $(BUILD_DIR)/LoxClass.o $(BUILD_DIR)/interpreter_test.o
STATEMENT_OBJS = $(BUILD_DIR)/TokenType.o $(BUILD_DIR)/Scanner.o $(BUILD_DIR)/Parser.o $(BUILD_DIR)/Interpreter.o $(BUILD_DIR)/LoxClass.o $(BUILD_DIR)/Resolver.o $(BUILD_DIR)/statement_test.o

# Target executables
SCANNER_TARGET = $(BUILD_DIR)/scanner_test
PARSER_TARGET = $(BUILD_DIR)/parser_test
INTERPRETER_TARGET = $(BUILD_DIR)/interpreter_test
STATEMENT_TARGET = $(BUILD_DIR)/statement_test

# Default target
all: $(SCANNER_TARGET) $(PARSER_TARGET) $(INTERPRETER_TARGET) $(STATEMENT_TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files to create executables
$(SCANNER_TARGET): $(SCANNER_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(SCANNER_TARGET) $(SCANNER_OBJS)

$(PARSER_TARGET): $(PARSER_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(PARSER_TARGET) $(PARSER_OBJS)

$(INTERPRETER_TARGET): $(INTERPRETER_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(INTERPRETER_TARGET) $(INTERPRETER_OBJS)

$(STATEMENT_TARGET): $(STATEMENT_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(STATEMENT_TARGET) $(STATEMENT_OBJS)

# Compile TokenType.cpp
$(BUILD_DIR)/TokenType.o: TokenType.cpp TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c TokenType.cpp -o $(BUILD_DIR)/TokenType.o

# Compile Scanner.cpp
$(BUILD_DIR)/Scanner.o: Scanner.cpp Scanner.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c Scanner.cpp -o $(BUILD_DIR)/Scanner.o

# Compile main.cpp
$(BUILD_DIR)/main.o: main.cpp Scanner.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c main.cpp -o $(BUILD_DIR)/main.o

# Compile Parser.cpp
$(BUILD_DIR)/Parser.o: Parser.cpp Parser.h Stmt.h exp.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c Parser.cpp -o $(BUILD_DIR)/Parser.o

# Compile parser_test.cpp
$(BUILD_DIR)/parser_test.o: parser_test.cpp Parser.h Stmt.h Scanner.h exp.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c parser_test.cpp -o $(BUILD_DIR)/parser_test.o

# Compile Interpreter.cpp
$(BUILD_DIR)/Interpreter.o: Interpreter.cpp Interpreter.h Environment.h Stmt.h exp.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c Interpreter.cpp -o $(BUILD_DIR)/Interpreter.o

# Compile LoxClass.cpp
$(BUILD_DIR)/LoxClass.o: LoxClass.cpp LoxClass.h LoxCallable.h Interpreter.h Environment.h Stmt.h exp.h Token.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c LoxClass.cpp -o $(BUILD_DIR)/LoxClass.o

# Compile Resolver.cpp
$(BUILD_DIR)/Resolver.o: Resolver.cpp Resolver.h Interpreter.h Environment.h Stmt.h exp.h Token.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c Resolver.cpp -o $(BUILD_DIR)/Resolver.o

# Compile interpreter_test.cpp
$(BUILD_DIR)/interpreter_test.o: interpreter_test.cpp Interpreter.h Parser.h Scanner.h exp.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c interpreter_test.cpp -o $(BUILD_DIR)/interpreter_test.o

# Compile statement_test.cpp
$(BUILD_DIR)/statement_test.o: statement_test.cpp Interpreter.h Environment.h Stmt.h Parser.h Scanner.h exp.h Token.h TokenType.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c statement_test.cpp -o $(BUILD_DIR)/statement_test.o

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run the programs
run-scanner: $(SCANNER_TARGET)
	$(SCANNER_TARGET)

run-parser: $(PARSER_TARGET)
	$(PARSER_TARGET)

run-interpreter: $(INTERPRETER_TARGET)
	$(INTERPRETER_TARGET)

run-statement: $(STATEMENT_TARGET)
	$(STATEMENT_TARGET)

run: run-scanner run-parser run-interpreter run-statement

.PHONY: all clean run run-scanner run-parser run-interpreter run-statement
