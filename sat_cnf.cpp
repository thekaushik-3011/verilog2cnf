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

// ---------------- Gate ----------------
class Gate {
public:
    enum class Type { AND, OR, NOT, XOR, XNOR, NAND, NOR, BUF, MUX };
    Type type;
    vector<string> inputs;
    string output;

    Gate(Type t, vector<string> in, string out) : type(t), inputs(in), output(out) {}
};

// ---------------- LogicCircuit ----------------
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
        for (const auto& in : gate.inputs) {
            if (wires.find(in) == wires.end()) {
                inputs.insert(in);
            }
            wires.insert(in);
        }
    }

    vector<string> getOutputs() const {
        unordered_set<string> allInputs;
        for (const auto& gate : gates) {
            for (const auto& in : gate.inputs) {
                allInputs.insert(in);
            }
        }

        vector<string> result;
        for (const auto& wire : wires) {
            if (allInputs.find(wire) == allInputs.end()) {
                result.push_back(wire);
            }
        }

        for (const auto& out : outputs) {
            if (find(result.begin(), result.end(), out) == result.end()) {
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

// ---------------- CNFConverter ----------------
class CNFConverter {
private:
    int variableCounter;
    unordered_map<string, int> variableMap;

    int getVariable(const string& name) {
        if (variableMap.find(name) == variableMap.end()) {
            variableCounter++;
            variableMap[name] = variableCounter;
        }
        return variableMap[name];
    }

    void resetVariables() {
        variableCounter = 0;
        variableMap.clear();
    }

    vector<vector<int>> gateToCNF(const Gate& gate) {
        vector<vector<int>> clauses;
        int outputVar = getVariable(gate.output);
        vector<int> inputVars;
        for (const auto& in : gate.inputs) {
            inputVars.push_back(getVariable(in));
        }

        if (gate.type == Gate::Type::AND) {
            for (int inpVar : inputVars) {
                clauses.push_back({-outputVar, inpVar});
            }
            vector<int> clause = {outputVar};
            for (int inpVar : inputVars) {
                clause.push_back(-inpVar);
            }
            clauses.push_back(clause);
        }
        else if (gate.type == Gate::Type::OR) {
            for (int inpVar : inputVars) {
                clauses.push_back({inpVar, -outputVar});
            }
            vector<int> clause = {-outputVar};
            clause.insert(clause.end(), inputVars.begin(), inputVars.end());
            clauses.push_back(clause);
        }
        else if (gate.type == Gate::Type::NOT) {
            int inpVar = inputVars[0];
            clauses.push_back({-outputVar, -inpVar});
            clauses.push_back({outputVar, inpVar});
        }
        else if (gate.type == Gate::Type::XOR) {
            int a = inputVars[0];
            int b = inputVars[1];
            clauses.push_back({-a, -b, -outputVar});
            clauses.push_back({a, b, -outputVar});
            clauses.push_back({a, -b, outputVar});
            clauses.push_back({-a, b, outputVar});
        }
        else if (gate.type == Gate::Type::XNOR) {
            int a = inputVars[0];
            int b = inputVars[1];
            clauses.push_back({a, b, -outputVar});
            clauses.push_back({-a, -b, -outputVar});
            clauses.push_back({-a, b, outputVar});
            clauses.push_back({a, -b, outputVar});
        }
        else if (gate.type == Gate::Type::NAND) {
            int a = inputVars[0];
            int b = inputVars[1];
            clauses.push_back({-a, -b, outputVar});
            clauses.push_back({-outputVar, a});
            clauses.push_back({-outputVar, b});
        }
        else if (gate.type == Gate::Type::NOR) {
            int a = inputVars[0];
            int b = inputVars[1];
            clauses.push_back({-a, -b, outputVar});
            clauses.push_back({-outputVar, -a});
            clauses.push_back({-outputVar, -b});
        }
        else if (gate.type == Gate::Type::MUX) {
            int a = inputVars[0];   // input a
            int b = inputVars[1];   // input b
            int sel = inputVars[2]; // select
            // y = (sel & b) | (~sel & a)
            clauses.push_back({-sel, a, outputVar});  // (~sel & a => y)
            clauses.push_back({sel, b, outputVar});   // (sel & b => y)
            clauses.push_back({-outputVar, -sel, a}); // y => (~sel & a) OR ...
            clauses.push_back({-outputVar, sel, b});  // y => (sel & b)
        }

        return clauses;
    }

public:
    CNFConverter() : variableCounter(0) {}

    vector<vector<int>> circuitToCNF(const LogicCircuit& circuit) {
        resetVariables();
        vector<vector<int>> clauses;

        for (const auto& wire : circuit.wires) {
            getVariable(wire);
        }

        for (const auto& gate : circuit.gates) {
            auto getClauses = gateToCNF(gate);
            clauses.insert(clauses.end(), getClauses.begin(), getClauses.end());
        }

        cout << "Variable mapping:\n";
        for (const auto& [name, num] : variableMap) {
            cout << name << " -> " << num << "\n";
        }
        
        return clauses;
    }

    unordered_map<string, int> getVariableMap() const {
        return variableMap;
    }

    int getNumVariables() const {
        return variableCounter;
    }
};

// ---------------- VerilogParser ----------------
class VerilogParser {
public:
    static LogicCircuit parse(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file");
        }

        LogicCircuit circuit;
        string line;
        while (getline(file, line)) {
            auto commentPos = line.find("//");
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }
            if (line.find("module") != string::npos) continue;
            if (line.find("endmodule") != string::npos) continue;

            if (line.find("input") != string::npos) {
                parseIO(line, circuit.inputs);
            }
            else if (line.find("output") != string::npos) {
                parseIO(line, circuit.outputs);
            }
            else if (line.find("assign") != string::npos) {
                parseAssignment(line, circuit);
            }
        }
        return circuit;
    }

private:
    static void parseIO(const string& line, unordered_set<string>& container) {
        string cleaned = line;
        cleaned.erase(remove_if(cleaned.begin(), cleaned.end(),
                                [](unsigned char c) { return c == ',' || c == ';'; }),
                      cleaned.end());

        stringstream ss(cleaned);
        string word;
        ss >> word; // "input" or "output"

        int msb = -1, lsb = -1;
        if (ss.peek() == '[') {
            char ch;
            ss >> ch; // '['
            ss >> msb;
            ss >> ch; // ':'
            ss >> lsb;
            ss >> ch; // ']'
        }

        while (ss >> word) {
            if (msb != -1 && lsb != -1) {
                int step = (msb > lsb) ? -1 : 1;
                for (int i = msb; i != lsb + step; i += step) {
                    container.insert(word + "[" + to_string(i) + "]");
                }
            } else {
                container.insert(word);
            }
        }
    }

    static void parseAssignment(const string& line, LogicCircuit& circuit) {
        string cleaned = line;
        cleaned.erase(remove_if(cleaned.begin(), cleaned.end(),
                                [](unsigned char c) { return c == ';'; }),
                    cleaned.end());

        size_t pos = cleaned.find('=');
        if (pos == string::npos) return;

        string lhs = cleaned.substr(0, pos);
        string rhs = cleaned.substr(pos + 1);

        if (lhs.find("assign") != string::npos) {
            lhs = lhs.substr(lhs.find("assign") + 6);
        }

        lhs.erase(remove_if(lhs.begin(), lhs.end(), ::isspace), lhs.end());
        rhs.erase(remove_if(rhs.begin(), rhs.end(), ::isspace), rhs.end());
        
        // Handle 2-to-1 MUX: sel ? b : a
        size_t quesPos = rhs.find('?');
        size_t colonPos = rhs.find(':');
        if (quesPos != string::npos && colonPos != string::npos) {
            string sel = rhs.substr(0, quesPos);
            string b   = rhs.substr(quesPos + 1, colonPos - quesPos - 1);
            string a   = rhs.substr(colonPos + 1);
            sel.erase(remove_if(sel.begin(), sel.end(), ::isspace), sel.end());
            a.erase(remove_if(a.begin(), a.end(), ::isspace), a.end());
            b.erase(remove_if(b.begin(), b.end(), ::isspace), b.end());
            circuit.addGate(Gate(Gate::Type::MUX, {a, b, sel}, lhs));
            return;
        }

        // Handle XNOR, NOR, NAND: ~(a ^ b), ~(a | b), ~(a & b)
        if (!rhs.empty() && rhs[0] == '~' && rhs[1] == '(' && rhs.back() == ')') {
            string inner = rhs.substr(2, rhs.size() - 3); // remove ~( and )
            size_t xorPos = inner.find('^');
            size_t orPos  = inner.find('|');
            size_t andPos = inner.find('&');

            if (xorPos != string::npos) {
                string left = inner.substr(0, xorPos);
                string right = inner.substr(xorPos + 1);
                left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                circuit.addGate(Gate(Gate::Type::XNOR, {left, right}, lhs));
                return;
            } else if (orPos != string::npos) {
                string left = inner.substr(0, orPos);
                string right = inner.substr(orPos + 1);
                left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                circuit.addGate(Gate(Gate::Type::NOR, {left, right}, lhs));
                return;
            } else if (andPos != string::npos) {
                string left = inner.substr(0, andPos);
                string right = inner.substr(andPos + 1);
                left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                circuit.addGate(Gate(Gate::Type::NAND, {left, right}, lhs));
                return;
            }
        }

        // Handle unary NOT
        if (!rhs.empty() && (rhs[0] == '~' || rhs[0] == '!')) {
            string operand = rhs.substr(1);
            circuit.addGate(Gate(Gate::Type::NOT, {operand}, lhs));
            return;
        }

        // Handle binary operators: AND, OR, XOR
        vector<string> operators = {"&", "|", "^"};
        for (const auto& op : operators) {
            size_t opPos = rhs.find(op);
            if (opPos != string::npos) {
                string left  = rhs.substr(0, opPos);
                string right = rhs.substr(opPos + op.length());

                left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());

                if (!left.empty() && !right.empty()) {
                    Gate::Type gateType;
                    if (op == "&") gateType = Gate::Type::AND;
                    else if (op == "|") gateType = Gate::Type::OR;
                    else if (op == "^") gateType = Gate::Type::XOR;

                    circuit.addGate(Gate(gateType, {left, right}, lhs));
                    return;
                }
            }
        }
    }
};

// ---------------- MAIN ----------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./sat_cnf <verilog_file>\n";
        return 1;
    }
    string filename = argv[1];
    LogicCircuit circuit = VerilogParser::parse(filename);

    CNFConverter converter;
    auto cnf = converter.circuitToCNF(circuit);

    ofstream out("circuit.cnf");
    out << "p cnf " << converter.getNumVariables() << " " << cnf.size() << "\n";
    for (const auto& clause : cnf) {
        for (int lit : clause) out << lit << " ";
        out << "0\n";
    }
    out.close();

    cout << "CNF written to circuit.cnf\n";
    return 0;
}

