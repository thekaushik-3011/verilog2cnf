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
            clauses.push_back({-sel, a, outputVar});
            clauses.push_back({sel, b, outputVar});
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

    // Tokenize expression while respecting parentheses
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
                // Handle potential multi-character operators
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

    // Parse expression with proper handling of chained operators
    static string parseExpression(const string& expr, const string& target, 
                                LogicCircuit& circuit, int& tempCounter) {
        string cleaned = expr;
        cleaned.erase(remove_if(cleaned.begin(), cleaned.end(), ::isspace), cleaned.end());
        
        if (cleaned.empty()) return "";
        
        // Handle ternary operator first
        size_t quesPos = cleaned.find('?');
        size_t colonPos = cleaned.find(':', quesPos);
        if (quesPos != string::npos && colonPos != string::npos) {
            // Check if ? and : are at top level (parenCount = 0)
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
        
        // Handle XNOR/NOR/NAND with parentheses: ~(a ^ b)
        if (cleaned.size() >= 4 && cleaned[0] == '~' && cleaned[1] == '(' && cleaned.back() == ')') {
            string inner = cleaned.substr(2, cleaned.size() - 3);
            // Tokenize inner expression to find operators
            vector<string> innerTokens = tokenize(inner);
            // Look for binary operators in inner tokens
            for (size_t i = 0; i < innerTokens.size(); i++) {
                if (innerTokens[i] == "^") {
                    // Find the operator position in original string
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
        
        // Handle unary NOT
        if (cleaned[0] == '~' || cleaned[0] == '!') {
            string operand = cleaned.substr(1);
            string operandParsed = parseExpression(operand, target + "_not", circuit, tempCounter);
            circuit.addGate(Gate(Gate::Type::NOT, {operandParsed}, target));
            return target;
        }
        
        // Remove outer parentheses if they exist and are balanced
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
        
        // Now handle binary and chained operators
        vector<string> tokens = tokenize(cleaned);
        
        // First, try to split by XOR (lowest precedence in our handling)
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
            // Handle chained XOR: a ^ b ^ c ^ d
            string current = parseExpression(xorParts[0], target + "_xor0", circuit, tempCounter);
            for (size_t i = 1; i < xorParts.size(); i++) {
                string nextPart = parseExpression(xorParts[i], target + "_xor" + to_string(i), circuit, tempCounter);
                string tempName = (i == xorParts.size() - 1) ? target : generateTempName(target + "_xor", tempCounter);
                circuit.addGate(Gate(Gate::Type::XOR, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        // Next, try to split by OR
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
            // Handle chained OR: a | b | c | d
            string current = parseExpression(orParts[0], target + "_or0", circuit, tempCounter);
            for (size_t i = 1; i < orParts.size(); i++) {
                string nextPart = parseExpression(orParts[i], target + "_or" + to_string(i), circuit, tempCounter);
                string tempName = (i == orParts.size() - 1) ? target : generateTempName(target + "_or", tempCounter);
                circuit.addGate(Gate(Gate::Type::OR, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        // Finally, try to split by AND
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
            // Handle chained AND: a & b & c & d
            string current = parseExpression(andParts[0], target + "_and0", circuit, tempCounter);
            for (size_t i = 1; i < andParts.size(); i++) {
                string nextPart = parseExpression(andParts[i], target + "_and" + to_string(i), circuit, tempCounter);
                string tempName = (i == andParts.size() - 1) ? target : generateTempName(target + "_and", tempCounter);
                circuit.addGate(Gate(Gate::Type::AND, {current, nextPart}, tempName));
                current = tempName;
            }
            return target;
        }
        
        // Base case: it's a single signal name
        return cleaned;
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

        // Check for vector [msb:lsb]
        string range = "";
        ss >> word;
        if (!word.empty() && word[0] == '[') {
            range = word;
            ss >> word; // move to first signal after vector
        }

        int msb = 0, lsb = 0;
        if (!range.empty()) {
            range.erase(remove(range.begin(), range.end(), '['), range.end());
            range.erase(remove(range.begin(), range.end(), ']'), range.end());
            size_t colonPos = range.find(':');
            if (colonPos != string::npos) {
                msb = stoi(range.substr(0, colonPos));
                lsb = stoi(range.substr(colonPos + 1));
            }
        }

        do {
            if (range.empty()) {
                container.insert(word);
            } else {
                for (int i = msb; i >= lsb; --i) {
                    container.insert(word + "[" + to_string(i) + "]");
                }
            }
        } while (ss >> word);
    }

    static void parseAssignment(const string& line, LogicCircuit& circuit) {
        string cleaned = line;
        // Remove semicolon
        cleaned.erase(remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());

        size_t pos = cleaned.find('=');
        if (pos == string::npos) return;

        string lhs = cleaned.substr(0, pos);
        string rhs = cleaned.substr(pos + 1);

        // Remove "assign" from lhs if present (e.g., "assign out = ...")
        if (lhs.find("assign") != string::npos) {
            lhs = lhs.substr(lhs.find("assign") + 6); // "assign" is 6 letters
        }

        // Trim whitespace
        auto trim = [](string& s) {
            s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char c) { return !isspace(c); }));
            s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !isspace(c); }).base(), s.end());
        };
        trim(lhs);
        trim(rhs);

        if (lhs.empty() || rhs.empty()) return;

        // Check for vector assignments: both sides have [msb:lsb]
        auto lhsRangePos = lhs.find('[');
        auto rhsRangePos = rhs.find('[');

        if (lhsRangePos != string::npos && rhsRangePos != string::npos) {
            // Parse ranges
            int lhsMsb, lhsLsb, rhsMsb, rhsLsb;
            parseRange(lhs, lhsMsb, lhsLsb);
            parseRange(rhs, rhsMsb, rhsLsb);

            // Ensure same width
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
        } else {
            // Scalar assignment
            int tempCounter = 0;
            parseExpression(rhs, lhs, circuit, tempCounter);
        }
    }

    // helper function
    static void parseRange(const string& s, int& msb, int& lsb) {
        size_t lb = s.find('['), rb = s.find(']');
        string r = s.substr(lb+1, rb-lb-1);
        size_t colonPos = r.find(':');
        msb = stoi(r.substr(0, colonPos));
        lsb = stoi(r.substr(colonPos+1));
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
    auto varMap = converter.getVariableMap();

    // Output variable mapping for debugging
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
    return 0;
}
