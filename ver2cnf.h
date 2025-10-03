#ifndef VER2CNF_H
#define VER2CNF_H

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cassert>

// ---------------- Gate ----------------
class Gate {
public:
    enum class Type { AND, OR, NOT, XOR, XNOR, NAND, NOR, BUF, MUX };
    Type type;
    std::vector<std::string> inputs;
    std::string output;

    Gate(Type t, std::vector<std::string> in, std::string out) : type(t), inputs(in), output(out) {}
};

// ---------------- LogicCircuit ----------------
class LogicCircuit {
public:
    std::string name;
    std::vector<Gate> gates;
    std::unordered_set<std::string> inputs;
    std::unordered_set<std::string> outputs;
    std::unordered_set<std::string> wires;

    void addGate(const Gate& gate);
    std::vector<std::string> getOutputs() const;
    std::vector<std::string> getInputs() const;
};

// ---------------- CNFConverter ----------------
class CNFConverter {
private:
    int variableCounter;
    std::unordered_map<std::string, int> variableMap;

    int getVariable(const std::string& name);
    void resetVariables();
    std::vector<std::vector<int>> gateToCNF(const Gate& gate);

public:
    CNFConverter();
    std::vector<std::vector<int>> circuitToCNF(const LogicCircuit& circuit);
    std::unordered_map<std::string, int> getVariableMap() const;
    int getNumVariables() const;
};

// ---------------- VerilogParser ----------------
class VerilogParser {
private:
    static std::string generateTempName(const std::string& base, int& counter);
    static std::vector<std::string> tokenize(const std::string& expr);
    static std::string parseExpression(const std::string& expr, const std::string& target, 
                                     LogicCircuit& circuit, int& tempCounter);
    static void parseIO(const std::string& line, std::unordered_set<std::string>& container);
    static void parseAssignment(const std::string& line, LogicCircuit& circuit);
    static void parseRange(const std::string& s, int& msb, int& lsb);
    
    // Helper functions for vector handling
    static std::string extractBaseName(const std::string& signal);
    static bool isVectorBase(const std::string& name, const LogicCircuit& circuit);
    static std::vector<std::string> getVectorBits(const std::string& baseName, const LogicCircuit& circuit);
    static std::string rewriteExpressionForBit(const std::string& expr, const std::string& baseName, 
                                             const std::string& bitSignal, const LogicCircuit& circuit);
    static int getVectorWidth(const std::string& baseName, const LogicCircuit& circuit);

public:
    static LogicCircuit parse(const std::string& filename);
};

#endif // VER2CNF_H