#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <regex>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cassert>

using namespace std;

class Gate {
public:
    enum class Type {
        AND, OR, NOT, XOR, XNOR, NAND, NOR, BUF, MUX
    };

    Type type;
    vector<string> inputs;
    string output;

    Gate(Type t, const vector<string>& in, const string& out) : type(t), inputs(in), output(out) {}

    static Type stringToType(const string& op) {
        if(op == "&" || op == "&&") return Type::AND;
        if(op == "|" || op == "||") return Type::OR;
        if(op == "!" || op == "~") return Type::NOT;
        if(op == "^") return Type::XOR;
        if(op == "~^" || op == "^~") return Type::XNOR;
        if(op == "~&") return Type::NAND;
        if(op == "~|") return Type::NOR;
        if(op == "?") return Type:: MUX;
        return Type::BUF;
    }
};

class LogicCircuit {
public:
    string name;
    vector<Gate> gates;
    unordered_set<string> inputs;
    unordered_set<string> outputs;
    unordered_set<string> wires;

    void addGate(const Gate& gate) {
        gates.push_back(gate);
        wires.insert(gate.output);
        for(const auto& in : gate.inputs) {
            if(wires.find(in) == wires.end()) {
                inputs.insert(in);
            }
            wires.insert(in);
        }
    }

    vector<string> getOutputs() const {
        unordered_set<string> allInputs;
        for(const auto& gate : gates) {
            for(const auto& in : gate.inputs) {
                allInputs.insert(in);
            }
        }

        vector<string> result;
        for(const auto& wire : wires) {
            if(allInputs.find(wire) == allInputs.end()) {
                result.push_back(wire);
            }
        }

        for(const auto& out : outputs) {
            if(find(result.begin(), result.end(), out) == result.end()) {
                result.push_back(out);
            }
        }

        sort(result.begin(), result.end());
        return result;
    }

    vector<string> getInputs() const {
        vector<string> result(inputs.begin(), inputs.end());
        sort(result.begin(), result.end());
        return result;
    }
};

class CNFConverter {
private:
    int variablecounter;
    unordered_map<string, int> variableMap;
    unordered_map<string> reverseVariableMap;

    int getVariable(const string& name) {
        
    }
}
