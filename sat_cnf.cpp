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
    enum class Type { AND, OR, NOT, XOR, XNOR, NAND, NOR, BUF, MUX };
    Type type;
    vector<string> inputs;
    string output;

    Gate(Type t, vector<string> in, string out) : type(t), inputs(in), output(out) {}
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
        for (const auto& in : gate.inputs) {
            if (wires.find(in) == wires.end()) {
                inputs.insert(in);
            }
            wires.insert(in);
        }
    }
};

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
        return clauses;
    }

    unordered_map<string, int> getVariableMap() const {
        return variableMap;
    }

    int getNumVariables() const {
        return variableCounter;
    }
};

// ---------------- PARSER ----------------
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
        ss >> word;

        while (ss >> word) {
            container.insert(word);
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

        if (!rhs.empty() && (rhs[0] == '~' || rhs[0] == '!')) {
            string operand = rhs.substr(1);
            circuit.addGate(Gate(Gate::Type::NOT, {operand}, lhs));
            return;
        }

        vector<string> operators = {"&", "|", "^"};
        for (const auto& op : operators) {
            size_t pos = rhs.find(op);
            if (pos != string::npos) {
                string left = rhs.substr(0, pos);
                string right = rhs.substr(pos + op.length());
                left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());

                if (!left.empty() && !right.empty()) {
                    Gate::Type gateType;
                    if (op == "&") gateType = Gate::Type::AND;
                    else if (op == "|") gateType = Gate::Type::OR;
                    else gateType = Gate::Type::XOR;

                    circuit.addGate(Gate(gateType, {left, right}, lhs));
                    return;
                }
            }
        }
    }
};

// ---------------- MAIN ----------------
int main() {
    string filename = "and_gate.v"; 
    LogicCircuit circuit = VerilogParser::parse(filename);

    CNFConverter converter;
    auto cnf = converter.circuitToCNF(circuit);
    auto varMap = converter.getVariableMap();

    cout << "c Variable mapping (signal_name -> variable_number):\n";
    for (const auto& kv : varMap) {
        cout << "c " << kv.first << " -> " << kv.second << "\n";
    }

    // Ask constraints
    cout << "\nAdd constraints on outputs (e.g., y=1 or y=0). Enter 'done' when finished.\n";
    string constraint;
    while (true) {
        cout << "Constraint: ";
        getline(cin, constraint);

        if (constraint == "done") break;
        size_t eqPos = constraint.find('=');
        if (eqPos == string::npos) {
            cout << "Invalid format. Use signal=value\n";
            continue;
        }

        string signal = constraint.substr(0, eqPos);
        string value = constraint.substr(eqPos + 1);
        signal.erase(remove_if(signal.begin(), signal.end(), ::isspace), signal.end());
        value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());

        if (varMap.find(signal) == varMap.end()) {
            cout << "Unknown signal: " << signal << "\n";
            continue;
        }

        int var = varMap.at(signal);
        if (value == "1") {
            cnf.push_back({var});
        } else if (value == "0") {
            cnf.push_back({-var});
        } else {
            cout << "Invalid value. Use 0 or 1.\n";
        }
    }

    // Save CNF to circuit.cnf
    ofstream out("circuit.cnf");
    out << "p cnf " << converter.getNumVariables() << " " << cnf.size() << "\n";
    for (const auto& clause : cnf) {
        for (int lit : clause) {
            out << lit << " ";
        }
        out << "0\n";
    }
    out.close();

    cout << "\nCNF written to circuit.cnf\n";
    return 0;
}

