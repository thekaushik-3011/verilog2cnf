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
    unordered_set<string> registers;

    void addGate(const Gate& gate) {
        gates.push_back(gate);
        wires.insert(gate.output);
        for (const auto& in : gate.inputs) {
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
            int a = inputVars[0];
            int b = inputVars[1];
            int sel = inputVars[2];
            clauses.push_back({-sel, -a, outputVar});
            clauses.push_back({sel, -b, outputVar});
            clauses.push_back({-outputVar, -sel, a});
            clauses.push_back({-outputVar, sel, b});
        } else if(gate.type==Gate::Type::BUF) {
            clauses.push_back({-outputVar,inputVars[0]});
            clauses.push_back({outputVar,-inputVars[0]});
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

// ---------------- VerilogParser ----------------
class VerilogParser {
private:
    static string generateTempName(const string& base, int& counter) {
        return base + "_temp_" + to_string(counter++);
    }

    static string trim(const string& s) {
        auto start = find_if(s.begin(), s.end(), [](unsigned char c) { return !isspace(c); });
        auto end = find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !isspace(c); }).base();
        return (start < end) ? string(start, end) : "";
    }

    static bool isVectorBase(const string& name, const LogicCircuit& circuit) {
        for (const auto& signal : circuit.inputs) {
            if (signal.length() > name.length() + 2 && 
                signal.substr(0, name.length()) == name && 
                signal[name.length()] == '[') {
                return true;
            }
        }
        for (const auto& signal : circuit.outputs) {
            if (signal.length() > name.length() + 2 && 
                signal.substr(0, name.length()) == name && 
                signal[name.length()] == '[') {
                return true;
            }
        }
        return false;
    }

    static vector<string> getVectorBits(const string& baseName, const LogicCircuit& circuit) {
        vector<string> bits;
        
        for (const auto& signal : circuit.outputs) {
            if (signal.length() > baseName.length() + 2 && 
                signal.substr(0, baseName.length()) == baseName && 
                signal[baseName.length()] == '[') {
                bits.push_back(signal);
            }
        }
        
        if (bits.empty()) {
            for (const auto& signal : circuit.inputs) {
                if (signal.length() > baseName.length() + 2 && 
                    signal.substr(0, baseName.length()) == baseName && 
                    signal[baseName.length()] == '[') {
                    bits.push_back(signal);
                }
            }
        }
        
        sort(bits.begin(), bits.end(), [](const string& a, const string& b) {
            size_t a_start = a.find('[') + 1;
            size_t a_end = a.find(']', a_start);
            size_t b_start = b.find('[') + 1;
            size_t b_end = b.find(']', b_start);
            
            if (a_start != string::npos && a_end != string::npos && 
                b_start != string::npos && b_end != string::npos) {
                int a_idx = stoi(a.substr(a_start, a_end - a_start));
                int b_idx = stoi(b.substr(b_start, b_end - b_start));
                return a_idx > b_idx;
            }
            return a < b;
        });
        
        return bits;
    }

    static string generateAdder(const string& leftOp, const string& rightOp, 
                               const string& target, LogicCircuit& circuit, int& tempCounter) {
        cout << "DEBUG ADDER: Generating adder for " << leftOp << " + " << rightOp << " -> " << target << endl;
        
        if (isVectorBase(target, circuit)) {
            vector<string> targetBits = getVectorBits(target, circuit);
            vector<string> leftBits = getVectorBits(leftOp, circuit);
            vector<string> rightBits = getVectorBits(rightOp, circuit);
            
            if (targetBits.empty() || leftBits.empty() || rightBits.empty()) {
                throw runtime_error("Cannot find vector bits for addition operands");
            }
            
            int width = targetBits.size();
            if (leftBits.size() != width || rightBits.size() != width) {
                throw runtime_error("Vector width mismatch in addition");
            }
            
            string carryIn = "";
            
            for (int i = width - 1; i >= 0; i--) {
                string a = leftBits[i];
                string b = rightBits[i];
                string sum = targetBits[i];
                string carryOut = (i > 0) ? generateTempName("carry", tempCounter) : "";
                
                if (carryIn.empty()) {
                    circuit.addGate(Gate(Gate::Type::XOR, {a, b}, sum));
                    if (!carryOut.empty()) {
                        circuit.addGate(Gate(Gate::Type::AND, {a, b}, carryOut));
                    }
                } else {
                    string xor1 = generateTempName("xor", tempCounter);
                    circuit.addGate(Gate(Gate::Type::XOR, {a, b}, xor1));
                    circuit.addGate(Gate(Gate::Type::XOR, {xor1, carryIn}, sum));
                    
                    if (!carryOut.empty()) {
                        string and1 = generateTempName("and", tempCounter);
                        string and2 = generateTempName("and", tempCounter);
                        circuit.addGate(Gate(Gate::Type::AND, {a, b}, and1));
                        circuit.addGate(Gate(Gate::Type::AND, {xor1, carryIn}, and2));
                        circuit.addGate(Gate(Gate::Type::OR, {and1, and2}, carryOut));
                    }
                }
                
                carryIn = carryOut;
            }
            
            return target;
        } else {
            throw runtime_error("Scalar addition not yet supported - use vector types");
        }
    }
    
    static string generateSubtractor(const string& leftOp, const string& rightOp,
                                    const string& target, LogicCircuit& circuit, int& tempCounter) {
        cout << "DEBUG SUBTRACTOR: Generating subtractor for " << leftOp << " - " << rightOp << " -> " << target << endl;
        
        if (isVectorBase(target, circuit)) {
            vector<string> targetBits = getVectorBits(target, circuit);
            vector<string> leftBits = getVectorBits(leftOp, circuit);
            vector<string> rightBits = getVectorBits(rightOp, circuit);
            
            if (targetBits.empty() || leftBits.empty() || rightBits.empty()) {
                throw runtime_error("Cannot find vector bits for subtraction operands");
            }
            
            int width = targetBits.size();
            if (leftBits.size() != width || rightBits.size() != width) {
                throw runtime_error("Vector width mismatch in subtraction");
            }
            
            vector<string> rightInverted(width);
            for (int i = 0; i < width; i++) {
                rightInverted[i] = generateTempName("inv", tempCounter);
                circuit.addGate(Gate(Gate::Type::NOT, {rightBits[i]}, rightInverted[i]));
            }
            
            string carryIn = "";
            
            for (int i = width - 1; i >= 0; i--) {
                string a = leftBits[i];
                string b = rightInverted[i];
                string sum = targetBits[i];
                string carryOut = (i > 0) ? generateTempName("carry", tempCounter) : "";
                
                if (i == width - 1) {
                    string xor1 = generateTempName("xor", tempCounter);
                    circuit.addGate(Gate(Gate::Type::XOR, {a, b}, xor1));
                    circuit.addGate(Gate(Gate::Type::NOT, {xor1}, sum));
                    
                    if (!carryOut.empty()) {
                        string and1 = generateTempName("and", tempCounter);
                        circuit.addGate(Gate(Gate::Type::AND, {a, b}, and1));
                        circuit.addGate(Gate(Gate::Type::OR, {and1, xor1}, carryOut));
                    }
                    carryIn = carryOut;
                } else {
                    string xor1 = generateTempName("xor", tempCounter);
                    circuit.addGate(Gate(Gate::Type::XOR, {a, b}, xor1));
                    circuit.addGate(Gate(Gate::Type::XOR, {xor1, carryIn}, sum));
                    
                    if (!carryOut.empty()) {
                        string and1 = generateTempName("and", tempCounter);
                        string and2 = generateTempName("and", tempCounter);
                        circuit.addGate(Gate(Gate::Type::AND, {a, b}, and1));
                        circuit.addGate(Gate(Gate::Type::AND, {xor1, carryIn}, and2));
                        circuit.addGate(Gate(Gate::Type::OR, {and1, and2}, carryOut));
                    }
                    carryIn = carryOut;
                }
            }
            
            return target;
        } else {
            throw runtime_error("Scalar subtraction not yet supported - use vector types");
        }
    }

    static vector<string> tokenize(const string& expr) {
        vector<string> tokens;
        string current;
        int parenCount = 0;
        
        for (size_t i = 0; i < expr.length(); i++) {
            char c = expr[i];
            if (isspace(c)) continue;
            
            if (c == '(') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                parenCount++;
                tokens.push_back("(");
            }
            else if (c == ')') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                tokens.push_back(")");
                parenCount--;
            }
            else if ((c == '&' || c == '|' || c == '^') && parenCount == 0) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                if (c == '&' && i + 1 < expr.length() && expr[i + 1] == '&') {
                    tokens.push_back("&&");
                    i++;
                }
                else if (c == '|' && i + 1 < expr.length() && expr[i + 1] == '|') {
                    tokens.push_back("||");
                    i++;
                }
                else if (c == '^' && i + 1 < expr.length() && expr[i + 1] == '~') {
                    tokens.push_back("^~");
                    i++;
                }
                else if (c == '~' && i + 1 < expr.length() && expr[i + 1] == '^') {
                    tokens.push_back("~^");
                    i++;
                }
                else {
                    tokens.push_back(string(1, c));
                }
            }
            else if (c == '~' || c == '!') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                tokens.push_back(string(1, c));
            }
            else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            tokens.push_back(current);
        }
        
        return tokens;
    }

    static string extractBaseName(const string& signal) {
        size_t pos = signal.find('[');
        if (pos != string::npos) {
            return signal.substr(0, pos);
        }
        return signal;
    }

    static string parseExpression(const string& expr, const string& target, 
                                LogicCircuit& circuit, int& tempCounter) {
        string cleaned = expr;
        cleaned.erase(remove_if(cleaned.begin(), cleaned.end(), ::isspace), cleaned.end());
        
        if (cleaned.empty()) return "";
        
        size_t plusPos = cleaned.find('+');
        if (plusPos != string::npos && plusPos > 0 && plusPos < cleaned.length() - 1) {
            int parenCount = 0;
            bool topLevel = true;
            for (size_t i = 0; i < plusPos; i++) {
                if (cleaned[i] == '(') parenCount++;
                else if (cleaned[i] == ')') parenCount--;
            }
            if (parenCount != 0) topLevel = false;
            
            if (topLevel) {
                string leftOp = cleaned.substr(0, plusPos);
                string rightOp = cleaned.substr(plusPos + 1);
                
                // Extract base names for vector operations
                string leftBase = extractBaseName(leftOp);
                string rightBase = extractBaseName(rightOp);
                string targetBase = extractBaseName(target);
                
                // Check if we're dealing with vectors
                if (isVectorBase(leftBase, circuit) && isVectorBase(rightBase, circuit)) {
                    return generateAdder(leftBase, rightBase, targetBase, circuit, tempCounter);
                } else {
                    return generateAdder(leftOp, rightOp, target, circuit, tempCounter);
                }
            }
        }
        
        size_t minusPos = cleaned.find('-');
        if (minusPos != string::npos && minusPos > 0 && minusPos < cleaned.length() - 1) {
            int parenCount = 0;
            bool topLevel = true;
            for (size_t i = 0; i < minusPos; i++) {
                if (cleaned[i] == '(') parenCount++;
                else if (cleaned[i] == ')') parenCount--;
            }
            if (parenCount != 0) topLevel = false;
            
            if (topLevel) {
                string leftOp = cleaned.substr(0, minusPos);
                string rightOp = cleaned.substr(minusPos + 1);
                
                // Extract base names for vector operations
                string leftBase = extractBaseName(leftOp);
                string rightBase = extractBaseName(rightOp);
                string targetBase = extractBaseName(target);
                
                // Check if we're dealing with vectors
                if (isVectorBase(leftBase, circuit) && isVectorBase(rightBase, circuit)) {
                    return generateSubtractor(leftBase, rightBase, targetBase, circuit, tempCounter);
                } else {
                    return generateSubtractor(leftOp, rightOp, target, circuit, tempCounter);
                }
            }
        }
        
        size_t quesPos = cleaned.find('?');
        size_t colonPos = cleaned.find(':', quesPos);
        if (quesPos != string::npos && colonPos != string::npos) {
            int parenCount = 0;
            bool validQues = true, validColon = true;
            for (size_t i = 0; i < quesPos; i++) {
                if (cleaned[i] == '(') parenCount++;
                else if (cleaned[i] == ')') parenCount--;
            }
            if (parenCount != 0) validQues = false;
            
            parenCount = 0;
            for (size_t i = 0; i <= colonPos; i++) {
                if (cleaned[i] == '(') parenCount++;
                else if (cleaned[i] == ')') parenCount--;
            }
            if (parenCount != 0) validColon = false;
            
            if (validQues && validColon) {
                string sel = cleaned.substr(0, quesPos);
                string b = cleaned.substr(quesPos + 1, colonPos - quesPos - 1);
                string a = cleaned.substr(colonPos + 1);
                string selParsed = parseExpression(sel, target + "_sel", circuit, tempCounter);
                string aParsed = parseExpression(a, target + "_a", circuit, tempCounter);
                string bParsed = parseExpression(b, target + "_b", circuit, tempCounter);
                circuit.addGate(Gate(Gate::Type::MUX, {aParsed, bParsed, selParsed}, target));
                return target;
            }
        }
        
        if (cleaned.size() >= 4 && cleaned[0] == '~' && cleaned[1] == '(' && cleaned.back() == ')') {
            string inner = cleaned.substr(2, cleaned.size() - 3);
            vector<string> innerTokens = tokenize(inner);
            for (size_t i = 0; i < innerTokens.size(); i++) {
                if (innerTokens[i] == "^") {
                    size_t opPos = inner.find('^');
                    if (opPos != string::npos) {
                        string left = inner.substr(0, opPos);
                        string right = inner.substr(opPos + 1);
                        left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                        right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                        if (!left.empty() && !right.empty()) {
                            string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                            string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                            circuit.addGate(Gate(Gate::Type::XNOR, {leftParsed, rightParsed}, target));
                            return target;
                        }
                    }
                }
                else if (innerTokens[i] == "|") {
                    size_t opPos = inner.find('|');
                    if (opPos != string::npos) {
                        string left = inner.substr(0, opPos);
                        string right = inner.substr(opPos + 1);
                        left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                        right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                        if (!left.empty() && !right.empty()) {
                            string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                            string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                            circuit.addGate(Gate(Gate::Type::NOR, {leftParsed, rightParsed}, target));
                            return target;
                        }
                    }
                }
                else if (innerTokens[i] == "&") {
                    size_t opPos = inner.find('&');
                    if (opPos != string::npos) {
                        string left = inner.substr(0, opPos);
                        string right = inner.substr(opPos + 1);
                        left.erase(remove_if(left.begin(), left.end(), ::isspace), left.end());
                        right.erase(remove_if(right.begin(), right.end(), ::isspace), right.end());
                        if (!left.empty() && !right.empty()) {
                            string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                            string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                            circuit.addGate(Gate(Gate::Type::NAND, {leftParsed, rightParsed}, target));
                            return target;
                        }
                    }
                }
            }
        }
        
        if (cleaned[0] == '~' || cleaned[0] == '!') {
            string operand = cleaned.substr(1);
            string operandParsed = parseExpression(operand, target + "_not", circuit, tempCounter);
            circuit.addGate(Gate(Gate::Type::NOT, {operandParsed}, target));
            return target;
        }
        
        if (cleaned.front() == '(' && cleaned.back() == ')') {
            int parenCount = 0;
            bool balanced = true;
            for (size_t i = 0; i < cleaned.size(); i++) {
                if (cleaned[i] == '(') parenCount++;
                else if (cleaned[i] == ')') parenCount--;
                if (parenCount == 0 && i < cleaned.size() - 1) {
                    balanced = false;
                    break;
                }
            }
            if (balanced) {
                return parseExpression(cleaned.substr(1, cleaned.size() - 2), target, circuit, tempCounter);
            }
        }
        
        vector<string> tokens = tokenize(cleaned);
        
        vector<string> xorParts;
        string currentPart;
        int parenCount = 0;
        for (const string& token : tokens) {
            if (token == "(") parenCount++;
            else if (token == ")") parenCount--;
            else if (token == "^" && parenCount == 0) {
                if (!currentPart.empty()) {
                    xorParts.push_back(currentPart);
                    currentPart = "";
                }
                continue;
            }
            if (!currentPart.empty()) currentPart += " ";
            currentPart += token;
        }
        if (!currentPart.empty()) xorParts.push_back(currentPart);
        
        if (xorParts.size() > 1) {
            string current = parseExpression(xorParts[0], target + "_xor0", circuit, tempCounter);
            for (size_t i = 1; i < xorParts.size(); i++) {
                string nextPart = parseExpression(xorParts[i], target + "_xor" + to_string(i), circuit, tempCounter);
                string tempName = (i == xorParts.size() - 1) ? target : generateTempName(target + "_xor", tempCounter);
                circuit.addGate(Gate(Gate::Type::XOR, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        vector<string> orParts;
        currentPart = "";
        parenCount = 0;
        for (const string& token : tokens) {
            if (token == "(") parenCount++;
            else if (token == ")") parenCount--;
            else if ((token == "|" || token == "||") && parenCount == 0) {
                if (!currentPart.empty()) {
                    orParts.push_back(currentPart);
                    currentPart = "";
                }
                continue;
            }
            if (!currentPart.empty()) currentPart += " ";
            currentPart += token;
        }
        if (!currentPart.empty()) orParts.push_back(currentPart);
        
        if (orParts.size() > 1) {
            string current = parseExpression(orParts[0], target + "_or0", circuit, tempCounter);
            for (size_t i = 1; i < orParts.size(); i++) {
                string nextPart = parseExpression(orParts[i], target + "_or" + to_string(i), circuit, tempCounter);
                string tempName = (i == orParts.size() - 1) ? target : generateTempName(target + "_or", tempCounter);
                circuit.addGate(Gate(Gate::Type::OR, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        vector<string> andParts;
        currentPart = "";
        parenCount = 0;
        for (const string& token : tokens) {
            if (token == "(") parenCount++;
            else if (token == ")") parenCount--;
            else if ((token == "&" || token == "&&") && parenCount == 0) {
                if (!currentPart.empty()) {
                    andParts.push_back(currentPart);
                    currentPart = "";
                }
                continue;
            }
            if (!currentPart.empty()) currentPart += " ";
            currentPart += token;
        }
        if (!currentPart.empty()) andParts.push_back(currentPart);
        
        if (andParts.size() > 1) {
            string current = parseExpression(andParts[0], target + "_and0", circuit, tempCounter);
            for (size_t i = 1; i < andParts.size(); i++) {
                string nextPart = parseExpression(andParts[i], target + "_and" + to_string(i), circuit, tempCounter);
                string tempName = (i == andParts.size() - 1) ? target : generateTempName(target + "_and", tempCounter);
                circuit.addGate(Gate(Gate::Type::AND, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        return cleaned;
    }

    static string readVerilogBlock(ifstream& file, string& firstLine) {
        string block = firstLine;
        string line;
        int depth = 0;

        if (firstLine.find("begin") != string::npos) {
            depth = 1;
        }

        while (depth > 0 && getline(file, line)) {
            auto commentPos = line.find("//");
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }

            if (line.find("begin") != string::npos) {
                depth++;
            }
            if (line.find("end") == 0 || 
                (line.find_first_not_of(" \t") != string::npos && 
                 line.find_first_not_of(" \t") == line.find("end"))) {
                depth--;
            }

            block += "\n" + line;
            if (depth == 0) break;
        }

        return block;
    }

    static void parseAlwaysBlock(const string& block, LogicCircuit& circuit) {
        cout << "DEBUG ALWAYS: Processing always block" << endl;
        
        size_t atPos = block.find("@(");
        if (atPos == string::npos) atPos = block.find("@*");
        
        if (atPos == string::npos || 
            (block.find("@(*)") == string::npos && block.find("@*") == string::npos)) {
            cout << "WARNING: Non-combinational always block skipped" << endl;
            return;
        }

        istringstream iss(block);
        string line;
        bool inBlock = false;
        
        while (getline(iss, line)) {
            auto commentPos = line.find("//");
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }
            
            string trimmed = trim(line);
            
            if (trimmed.find("always") != string::npos) {
                continue;
            }
            
            if (trimmed == "begin") {
                inBlock = true;
                continue;
            }
            
            if (trimmed == "end" || trimmed.find("end") == 0) {
                break;
            }
            
            if (trimmed.empty()) continue;
            
            if (trimmed.find('=') != string::npos) {
                parseAlwaysAssignment(trimmed, circuit);
            }
        }
    }

    static void parseAlwaysAssignment(const string& line, LogicCircuit& circuit) {
        string cleaned = line;
        cleaned.erase(remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());
        
        size_t eqPos = cleaned.find('=');
        if (eqPos == string::npos) return;
        
        if (eqPos > 0 && cleaned[eqPos-1] == '<') {
            cout << "WARNING: Non-blocking assignment in combinational logic - skipped" << endl;
            return;
        }

        string lhs = cleaned.substr(0, eqPos);
        string rhs = cleaned.substr(eqPos + 1);
        
        lhs = trim(lhs);
        rhs = trim(rhs);
        
        if (lhs.empty() || rhs.empty()) return;
        
        string fakeAssign = "assign " + lhs + " = " + rhs + ";";
        parseAssignment(fakeAssign, circuit);
    }

    static void parseGenerateBlock(const string& block, LogicCircuit& circuit) {
        cout << "DEBUG GENERATE: Processing generate block (not fully implemented)" << endl;
    }

    static string rewriteExpressionForBit(const string& expr, const string& baseName, 
                                         const string& bitSignal, const LogicCircuit& circuit) {
        string result = expr;
        size_t pos = 0;
        
        while ((pos = result.find(baseName, pos)) != string::npos) {
            bool beforeOk = (pos == 0) || !isalnum(static_cast<unsigned char>(result[pos-1]));
            size_t afterPos = pos + baseName.length();
            bool afterOk = (afterPos >= result.length()) || !isalnum(static_cast<unsigned char>(result[afterPos]));
            
            if (beforeOk && afterOk) {
                result.replace(pos, baseName.length(), bitSignal);
                pos += bitSignal.length();
            } else {
                pos += baseName.length();
            }
        }
        
        return result;
    }

    static void parseRange(const string& s, int& msb, int& lsb) {
        size_t lb = s.find('['), rb = s.find(']');
        string r = s.substr(lb+1, rb-lb-1);
        size_t colonPos = r.find(':');
        msb = stoi(r.substr(0, colonPos));
        lsb = stoi(r.substr(colonPos+1));
    }

    static void parseIO(const string& line, unordered_set<string>& container) {
        string cleaned = line;
        cleaned.erase(remove_if(cleaned.begin(), cleaned.end(),
                                [](unsigned char c) { return c == ',' || c == ';'; }),
                      cleaned.end());

        stringstream ss(cleaned);
        string word;
        ss >> word;

        string currentRange = "";
        
        while (ss >> word) {
            if (word[0] == '[') {
                currentRange = word;
            } else {
                if (!currentRange.empty()) {
                    string rangeClean = currentRange;
                    rangeClean.erase(remove(rangeClean.begin(), rangeClean.end(), '['), rangeClean.end());
                    rangeClean.erase(remove(rangeClean.begin(), rangeClean.end(), ']'), rangeClean.end());
                    
                    size_t colonPos = rangeClean.find(':');
                    if (colonPos != string::npos) {
                        int msb = stoi(rangeClean.substr(0, colonPos));
                        int lsb = stoi(rangeClean.substr(colonPos + 1));
                        for (int i = msb; i >= lsb; --i) {
                            string bit_signal = word + "[" + to_string(i) + "]";
                            container.insert(bit_signal);
                        }
                    } else {
                        string bit_signal = word + "[" + rangeClean + "]";
                        container.insert(bit_signal);
                    }
                    currentRange = "";
                } else {
                    container.insert(word);
                }
            }
        }
    }

    static void parseAssignment(const string& line, LogicCircuit& circuit) {
        cout << "DEBUG ASSIGN: Raw line = \"" << line << "\"" << endl;
        
        string cleaned = line;
        cleaned.erase(remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());
        
        size_t pos = cleaned.find("<=");
        bool isNonBlocking = false;
        if (pos == string::npos) {
            pos = cleaned.find('=');
        } else {
            isNonBlocking = true;
        }
        
        if (pos == string::npos) {
            cout << "DEBUG ASSIGN: No assignment operator found, skipping line" << endl;
            return;
        }

        string lhs = cleaned.substr(0, pos);
        string rhs = cleaned.substr(pos + (isNonBlocking ? 2 : 1));
        cout << "DEBUG ASSIGN: Raw LHS = \"" << lhs << "\", Raw RHS = \"" << rhs << "\"" << endl;

        if (lhs.find("assign") != string::npos) {
            size_t assignPos = lhs.find("assign");
            lhs = lhs.substr(assignPos + 6);
        }

        lhs = trim(lhs);
        rhs = trim(rhs);
        cout << "DEBUG ASSIGN: Trimmed LHS = \"" << lhs << "\", Trimmed RHS = \"" << rhs << "\"" << endl;

        if (lhs.empty() || rhs.empty()) {
            return;
        }

        auto lhsRangePos = lhs.find('[');
        auto rhsRangePos = rhs.find('[');
        
        bool lhsIsSimpleVector = (lhsRangePos != string::npos) && 
                                (lhs.find_first_of(" \t()&|^~?:", lhsRangePos) == string::npos);
        bool rhsIsSimpleVector = (rhsRangePos != string::npos) && 
                                (rhs.find_first_of(" \t()&|^~?:", rhsRangePos) == string::npos);

        if (lhsIsSimpleVector && rhsIsSimpleVector) {
            int lhsMsb, lhsLsb, rhsMsb, rhsLsb;
            parseRange(lhs, lhsMsb, lhsLsb);
            parseRange(rhs, rhsMsb, rhsLsb);

            if ((lhsMsb - lhsLsb) != (rhsMsb - rhsLsb)) {
                throw runtime_error("Vector width mismatch in assignment");
            }

            int width = lhsMsb - lhsLsb + 1;
            for (int i = 0; i < width; ++i) {
                string lhsBit = lhs.substr(0, lhsRangePos) + "[" + to_string(lhsMsb - i) + "]";
                string rhsBit = rhs.substr(0, rhsRangePos) + "[" + to_string(rhsMsb - i) + "]";
                int tempCounter = 0;
                parseExpression(rhsBit, lhsBit, circuit, tempCounter);
            }
        }
        else if (isVectorBase(lhs, circuit)) {
            vector<string> lhsBits = getVectorBits(lhs, circuit);
            
            for (const string& lhsBit : lhsBits) {
                string rhsBit = rhs;
                
                vector<string> vectorBases;
                for (const auto& in : circuit.inputs) {
                    size_t bracketPos = in.find('[');
                    if (bracketPos != string::npos) {
                        string base = in.substr(0, bracketPos);
                        if (find(vectorBases.begin(), vectorBases.end(), base) == vectorBases.end()) {
                            vectorBases.push_back(base);
                        }
                    }
                }
                
                string tempRhs = rhsBit;
                for (const string& base : vectorBases) {
                    if (isVectorBase(base, circuit)) {
                        vector<string> baseBits = getVectorBits(base, circuit);
                        size_t lhsIdx = 0;
                        for (size_t i = 0; i < lhsBits.size(); i++) {
                            if (lhsBits[i] == lhsBit) {
                                lhsIdx = i;
                                break;
                            }
                        }
                        if (lhsIdx < baseBits.size()) {
                            tempRhs = rewriteExpressionForBit(tempRhs, base, baseBits[lhsIdx], circuit);
                        }
                    }
                }
                rhsBit = tempRhs;
                
                int tempCounter = 0;
                parseExpression(rhsBit, lhsBit, circuit, tempCounter);
            }
        } else {
            int tempCounter = 0;
            string result = parseExpression(rhs, lhs, circuit, tempCounter);
        }
    }

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
            else if (line.find("reg") != string::npos) {
                parseIO(line, circuit.registers);
            }
            else if (line.find("assign") != string::npos) {
                parseAssignment(line, circuit);
            }
            else if (line.find("always") != string::npos) {
                string block = readVerilogBlock(file, line);
                parseAlwaysBlock(block, circuit);
            }
            else if (line.find("generate") != string::npos) {
                string block = readVerilogBlock(file, line);
                parseGenerateBlock(block, circuit);
            }
        }
        return circuit;
    }
};

// ---------------- MAIN ----------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./sat_cnf <verilog_file>\n";
        return 1;
    }
    string filename = argv[1];
    
    try {
        LogicCircuit circuit = VerilogParser::parse(filename);

        // Remove duplicate gates (same output = redundant)
        sort(circuit.gates.begin(), circuit.gates.end(), [](const Gate& a, const Gate& b) {
            return a.output < b.output;
        });
        auto last = unique(circuit.gates.begin(), circuit.gates.end(), 
            [](const Gate& a, const Gate& b) { 
                return a.output == b.output; 
            });
        circuit.gates.erase(last, circuit.gates.end());

        cout << "\n=== FINAL CIRCUIT STATE ===" << endl;
        cout << "Inputs (" << circuit.inputs.size() << "): ";
        vector<string> sorted_inputs(circuit.inputs.begin(), circuit.inputs.end());
        sort(sorted_inputs.begin(), sorted_inputs.end());
        for (const auto& inp : sorted_inputs) {
            cout << "\"" << inp << "\" ";
        }
        cout << endl;

        cout << "Outputs (" << circuit.outputs.size() << "): ";
        vector<string> sorted_outputs(circuit.outputs.begin(), circuit.outputs.end());
        sort(sorted_outputs.begin(), sorted_outputs.end());
        for (const auto& out : sorted_outputs) {
            cout << "\"" << out << "\" ";
        }
        cout << endl;

        if (!circuit.registers.empty()) {
            cout << "Registers (" << circuit.registers.size() << "): ";
            vector<string> sorted_regs(circuit.registers.begin(), circuit.registers.end());
            sort(sorted_regs.begin(), sorted_regs.end());
            for (const auto& reg : sorted_regs) {
                cout << "\"" << reg << "\" ";
            }
            cout << endl;
        }

        cout << "Wires (" << circuit.wires.size() << "): ";
        vector<string> sorted_wires(circuit.wires.begin(), circuit.wires.end());
        sort(sorted_wires.begin(), sorted_wires.end());
        for (const auto& wire : sorted_wires) {
            cout << "\"" << wire << "\" ";
        }
        cout << endl;

        cout << "Gates (" << circuit.gates.size() << "):" << endl;
        for (size_t i = 0; i < circuit.gates.size(); i++) {
            const Gate& g = circuit.gates[i];
            cout << "  Gate " << i << ": " << g.output << " = ";
            switch (g.type) {
                case Gate::Type::AND: cout << "AND("; break;
                case Gate::Type::OR: cout << "OR("; break;
                case Gate::Type::NOT: cout << "NOT("; break;
                case Gate::Type::XOR: cout << "XOR("; break;
                case Gate::Type::XNOR: cout << "XNOR("; break;
                case Gate::Type::NAND: cout << "NAND("; break;
                case Gate::Type::NOR: cout << "NOR("; break;
                case Gate::Type::BUF: cout << "BUF("; break;
                case Gate::Type::MUX: cout << "MUX("; break;
            }
            for (size_t j = 0; j < g.inputs.size(); j++) {
                if (j > 0) cout << ", ";
                cout << g.inputs[j];
            }
            cout << ")" << endl;
        }
        cout << "==========================\n" << endl;

        CNFConverter converter;
        auto cnf = converter.circuitToCNF(circuit);
        auto varMap = converter.getVariableMap();

        cout << "c Variable mapping (signal_name -> variable_number):\n";
        vector<pair<string, int>> sortedVars(varMap.begin(), varMap.end());
        sort(sortedVars.begin(), sortedVars.end());
        for (const auto& kv : sortedVars) {
            cout << "c " << kv.first << " -> " << kv.second << "\n";
        }

        ofstream out("circuit.cnf");
        out << "c CNF generated from Verilog combinational logic\n";
        out << "p cnf " << converter.getNumVariables() << " " << cnf.size() << "\n";
        for (const auto& clause : cnf) {
            for (int lit : clause) out << lit << " ";
            out << "0\n";
        }
        out.close();

        cout << "CNF written to circuit.cnf\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}