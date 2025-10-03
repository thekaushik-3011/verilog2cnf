#include "ver2cnf.h"

// ---------------- LogicCircuit ----------------
void LogicCircuit::addGate(const Gate& gate) {
    gates.push_back(gate);
    wires.insert(gate.output);
    for (const auto& in : gate.inputs) {
        if (wires.find(in) == wires.end()) {
            inputs.insert(in);
        }
        wires.insert(in);
    }
}

std::vector<std::string> LogicCircuit::getOutputs() const {
    std::unordered_set<std::string> allInputs;
    for (const auto& gate : gates) {
        for (const auto& in : gate.inputs) {
            allInputs.insert(in);
        }
    }

    std::vector<std::string> result;
    for (const auto& wire : wires) {
        if (allInputs.find(wire) == allInputs.end()) {
            result.push_back(wire);
        }
    }

    for (const auto& out : outputs) {
        if (std::find(result.begin(), result.end(), out) == result.end()) {
            result.push_back(out);
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> LogicCircuit::getInputs() const {
    std::vector<std::string> result(inputs.begin(), inputs.end());
    std::sort(result.begin(), result.end());
    return result;
}

// ---------------- CNFConverter ----------------
CNFConverter::CNFConverter() : variableCounter(0) {}

int CNFConverter::getVariable(const std::string& name) {
    if (variableMap.find(name) == variableMap.end()) {
        variableCounter++;
        variableMap[name] = variableCounter;
    }
    return variableMap[name];
}

void CNFConverter::resetVariables() {
    variableCounter = 0;
    variableMap.clear();
}

std::vector<std::vector<int>> CNFConverter::gateToCNF(const Gate& gate) {
    std::vector<std::vector<int>> clauses;
    int outputVar = getVariable(gate.output);
    std::vector<int> inputVars;
    for (const auto& in : gate.inputs) {
        inputVars.push_back(getVariable(in));
    }

    // Standard CNF encodings for boolean gates: out <-> f(inputs)
    if (gate.type == Gate::Type::AND) {
        // out -> each input true: (-out v in_i)
        for (int inpVar : inputVars) {
            clauses.push_back({-outputVar, inpVar});
        }
        // all inputs true -> out: (-in1 v -in2 v ... v out)
        std::vector<int> clause = {outputVar};
        for (int inpVar : inputVars) {
            clause.push_back(-inpVar);
        }
        clauses.push_back(clause);
    }
    else if (gate.type == Gate::Type::OR) {
        // out -> at least one input true: (-out v in1 v in2 ...)
        {
            std::vector<int> clause = {-outputVar};
            clause.insert(clause.end(), inputVars.begin(), inputVars.end());
            clauses.push_back(clause);
        }
        // each input -> out: (-in_i v out)
        for (int inpVar : inputVars) {
            clauses.push_back({-inpVar, outputVar});
        }
    }
    else if (gate.type == Gate::Type::NOT) {
        int inpVar = inputVars[0];
        // out <-> ~in  => (-out v -in) & (out v in)
        clauses.push_back({-outputVar, -inpVar});
        clauses.push_back({outputVar, inpVar});
    }
    else if (gate.type == Gate::Type::XOR) {
        // two-input XOR truth table CNF (out = a xor b)
        int a = inputVars[0];
        int b = inputVars[1];
        clauses.push_back({-a, -b, -outputVar});
        clauses.push_back({a, b, -outputVar});
        clauses.push_back({a, -b, outputVar});
        clauses.push_back({-a, b, outputVar});
    }
    else if (gate.type == Gate::Type::XNOR) {
        // two-input XNOR (out = a xnor b)
        int a = inputVars[0];
        int b = inputVars[1];
        clauses.push_back({a, b, -outputVar});
        clauses.push_back({-a, -b, -outputVar});
        clauses.push_back({-a, b, outputVar});
        clauses.push_back({a, -b, outputVar});
    }
    else if (gate.type == Gate::Type::NAND) {
        // out <-> !(a & b)  <=> out <-> (¬a ∨ ¬b)
        // Encoded as:
        // (a ∧ b) -> ¬out   => (-a ∨ -b ∨ -out)
        // ¬out -> a         => (out ∨ a)
        // ¬out -> b         => (out ∨ b)
        int a = inputVars[0];
        int b = inputVars[1];
        clauses.push_back({-a, -b, -outputVar});
        clauses.push_back({outputVar, a});
        clauses.push_back({outputVar, b});
    }
    else if (gate.type == Gate::Type::NOR) {
        // out <-> !(a ∨ b)  <=> out <-> (¬a ∧ ¬b)
        // Encoded as:
        // out -> ¬a         => (-out ∨ -a)
        // out -> ¬b         => (-out ∨ -b)
        // (¬a ∧ ¬b) -> out  => (a ∨ b ∨ out)
        int a = inputVars[0];
        int b = inputVars[1];
        clauses.push_back({-outputVar, -a});
        clauses.push_back({-outputVar, -b});
        clauses.push_back({a, b, outputVar});
    }
    else if (gate.type == Gate::Type::MUX) {
        // 3-input MUX: out = sel ? b : a
        // inputs: a, b, sel (we assume this order in your parser)
        int a = inputVars[0];
        int b = inputVars[1];
        int sel = inputVars[2];
        // Implementation (common CNF encoding):
        // sel=0 -> out = a  => (-sel v -a v out) & (-sel v a v -out)
        // sel=1 -> out = b  => ( sel v -b v out) & ( sel v b v -out)
        clauses.push_back({-sel, -a, outputVar});
        clauses.push_back({-sel, a, -outputVar});
        clauses.push_back({sel, -b, outputVar});
        clauses.push_back({sel, b, -outputVar});
    } else if (gate.type == Gate::Type::BUF) {
        // out <-> in
        int in = inputVars[0];
        clauses.push_back({-outputVar, in});  // out -> in  == (-out v in)
        clauses.push_back({-in, outputVar});  // in -> out  == (-in v out)
    }

    return clauses;
}

std::vector<std::vector<int>> CNFConverter::circuitToCNF(const LogicCircuit& circuit) {
    resetVariables();
    std::vector<std::vector<int>> clauses;

    // Ensure all wires have variables assigned (inputs/outputs/temps)
    for (const auto& wire : circuit.wires) {
        getVariable(wire);
    }

    for (const auto& gate : circuit.gates) {
        auto getClauses = gateToCNF(gate);
        clauses.insert(clauses.end(), getClauses.begin(), getClauses.end());
    }

    return clauses;
}

std::unordered_map<std::string, int> CNFConverter::getVariableMap() const {
    return variableMap;
}

int CNFConverter::getNumVariables() const {
    return variableCounter;
}

// ---------------- VerilogParser ----------------
std::string VerilogParser::generateTempName(const std::string& base, int& counter) {
    return base + "_temp_" + std::to_string(counter++);
}

std::vector<std::string> VerilogParser::tokenize(const std::string& expr) {
    std::vector<std::string> tokens;
    std::string current;
    int parenCount = 0;
    
    for (size_t i = 0; i < expr.length(); i++) {
        char c = expr[i];
        if (isspace(static_cast<unsigned char>(c))) continue;
        
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
        else if ((c == '&' || c == '|' || c == '^' || c == '~' || c == '!') && parenCount == 0) {
            if (!current.empty()) {
                tokens.push_back(current);
                current = "";
            }
            // handle common multi-char operators
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
                tokens.push_back(std::string(1, c));
            }
        }
        else if (c == '~' || c == '!') {
            if (!current.empty()) {
                tokens.push_back(current);
                current = "";
            }
            tokens.push_back(std::string(1, c));
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

std::string VerilogParser::parseExpression(const std::string& expr, const std::string& target, 
                                         LogicCircuit& circuit, int& tempCounter) {
    std::string cleaned = expr;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(), ::isspace), cleaned.end());
    
    if (cleaned.empty()) return "";
    
    // Handle ternary MUX: sel ? b : a
    size_t quesPos = cleaned.find('?');
    size_t colonPos = cleaned.find(':', quesPos);
    if (quesPos != std::string::npos && colonPos != std::string::npos) {
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
            std::string sel = cleaned.substr(0, quesPos);
            std::string b = cleaned.substr(quesPos + 1, colonPos - quesPos - 1);
            std::string a = cleaned.substr(colonPos + 1);
            std::string selParsed = parseExpression(sel, target + "_sel", circuit, tempCounter);
            std::string aParsed = parseExpression(a, target + "_a", circuit, tempCounter);
            std::string bParsed = parseExpression(b, target + "_b", circuit, tempCounter);
            circuit.addGate(Gate(Gate::Type::MUX, {aParsed, bParsed, selParsed}, target));
            return target;
        }
    }
    
    // Special-case: patterns like ~(a | b)  => create NOR
    if (cleaned.size() >= 4 && cleaned[0] == '~' && cleaned[1] == '(' && cleaned.back() == ')') {
        std::string inner = cleaned.substr(2, cleaned.size() - 3);
        std::vector<std::string> innerTokens = tokenize(inner);
        for (size_t i = 0; i < innerTokens.size(); i++) {
            if (innerTokens[i] == "^") {
                size_t opPos = inner.find('^');
                if (opPos != std::string::npos) {
                    std::string left = inner.substr(0, opPos);
                    std::string right = inner.substr(opPos + 1);
                    left.erase(std::remove_if(left.begin(), left.end(), ::isspace), left.end());
                    right.erase(std::remove_if(right.begin(), right.end(), ::isspace), right.end());
                    if (!left.empty() && !right.empty()) {
                        std::string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                        std::string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                        circuit.addGate(Gate(Gate::Type::XNOR, {leftParsed, rightParsed}, target));
                        return target;
                    }
                }
            }
            else if (innerTokens[i] == "|") {
                size_t opPos = inner.find('|');
                if (opPos != std::string::npos) {
                    std::string left = inner.substr(0, opPos);
                    std::string right = inner.substr(opPos + 1);
                    left.erase(std::remove_if(left.begin(), left.end(), ::isspace), left.end());
                    right.erase(std::remove_if(right.begin(), right.end(), ::isspace), right.end());
                    if (!left.empty() && !right.empty()) {
                        std::string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                        std::string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                        // ~(left | right) == NOR(left, right)
                        circuit.addGate(Gate(Gate::Type::NOR, {leftParsed, rightParsed}, target));
                        return target;
                    }
                }
            }
            else if (innerTokens[i] == "&") {
                size_t opPos = inner.find('&');
                if (opPos != std::string::npos) {
                    std::string left = inner.substr(0, opPos);
                    std::string right = inner.substr(opPos + 1);
                    left.erase(std::remove_if(left.begin(), left.end(), ::isspace), left.end());
                    right.erase(std::remove_if(right.begin(), right.end(), ::isspace), right.end());
                    if (!left.empty() && !right.empty()) {
                        std::string leftParsed = parseExpression(left, target + "_left", circuit, tempCounter);
                        std::string rightParsed = parseExpression(right, target + "_right", circuit, tempCounter);
                        // ~(left & right) == NAND(left, right)
                        circuit.addGate(Gate(Gate::Type::NAND, {leftParsed, rightParsed}, target));
                        return target;
                    }
                }
            }
        }
    }
    
    // Unary NOT
    if (cleaned[0] == '~' || cleaned[0] == '!') {
        std::string operand = cleaned.substr(1);
        std::string operandParsed = parseExpression(operand, target + "_not", circuit, tempCounter);
        circuit.addGate(Gate(Gate::Type::NOT, {operandParsed}, target));
        return target;
    }
    
    // Parentheses only
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
    
    // Tokenize and handle XOR with no parentheses precedence issues
    std::vector<std::string> tokens = tokenize(cleaned);
    
    std::vector<std::string> xorParts;
    std::string currentPart;
    int parenCount = 0;
    for (const std::string& token : tokens) {
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
        std::string current = parseExpression(xorParts[0], target + "_xor0", circuit, tempCounter);
        for (size_t i = 1; i < xorParts.size(); i++) {
            std::string nextPart = parseExpression(xorParts[i], target + "_xor" + std::to_string(i), circuit, tempCounter);
            std::string tempName = (i == xorParts.size() - 1) ? target : generateTempName(target + "_xor", tempCounter);
            circuit.addGate(Gate(Gate::Type::XOR, {current, nextPart}, tempName));
            current = tempName;
        }
        return target;
    }
    
    // OR parts
    std::vector<std::string> orParts;
    currentPart = "";
    parenCount = 0;
    for (const std::string& token : tokens) {
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
        std::string current = parseExpression(orParts[0], target + "_or0", circuit, tempCounter);
        for (size_t i = 1; i < orParts.size(); i++) {
            std::string nextPart = parseExpression(orParts[i], target + "_or" + std::to_string(i), circuit, tempCounter);
            std::string tempName = (i == orParts.size() - 1) ? target : generateTempName(target + "_or", tempCounter);
            circuit.addGate(Gate(Gate::Type::OR, {current, nextPart}, tempName));
            current = tempName;
        }
        return target;
    }
    
    // AND parts
    std::vector<std::string> andParts;
    currentPart = "";
    parenCount = 0;
    for (const std::string& token : tokens) {
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
        std::string current = parseExpression(andParts[0], target + "_and0", circuit, tempCounter);
        for (size_t i = 1; i < andParts.size(); i++) {
            std::string nextPart = parseExpression(andParts[i], target + "_and" + std::to_string(i), circuit, tempCounter);
            std::string tempName = (i == andParts.size() - 1) ? target : generateTempName(target + "_and", tempCounter);
            circuit.addGate(Gate(Gate::Type::AND, {current, nextPart}, tempName));
            current = tempName;
        }
        return target;
    }
    
    // otherwise this is a simple signal name (or bit)
    return cleaned;
}

void VerilogParser::parseIO(const std::string& line, std::unordered_set<std::string>& container) {
    std::string cleaned = line;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
                                [](unsigned char c) { return c == ',' || c == ';'; }),
                  cleaned.end());

    std::stringstream ss(cleaned);
    std::string word;
    ss >> word;

    std::string currentRange = "";
    
    while (ss >> word) {
        if (word[0] == '[') {
            currentRange = word;
        } else {
            if (!currentRange.empty()) {
                std::string rangeClean = currentRange;
                rangeClean.erase(std::remove(rangeClean.begin(), rangeClean.end(), '['), rangeClean.end());
                rangeClean.erase(std::remove(rangeClean.begin(), rangeClean.end(), ']'), rangeClean.end());
                size_t colonPos = rangeClean.find(':');
                if (colonPos != std::string::npos) {
                    int msb = std::stoi(rangeClean.substr(0, colonPos));
                    int lsb = std::stoi(rangeClean.substr(colonPos + 1));
                    for (int i = msb; i >= lsb; --i) {
                        container.insert(word + "[" + std::to_string(i) + "]");
                    }
                } else {
                    container.insert(word + "[" + rangeClean + "]");
                }
                currentRange = "";
            } else {
                container.insert(word);
            }
        }
    }
}

void VerilogParser::parseAssignment(const std::string& line, LogicCircuit& circuit) {
    std::string cleaned = line;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());

    size_t pos = cleaned.find('=');
    if (pos == std::string::npos) return;

    std::string lhs = cleaned.substr(0, pos);
    std::string rhs = cleaned.substr(pos + 1);

    if (lhs.find("assign") != std::string::npos) {
        size_t assignPos = lhs.find("assign");
        lhs = lhs.substr(assignPos + 6);
    }

    auto trim = [](std::string& s) {
        if (s.empty()) return;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !isspace(c); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !isspace(c); }).base(), s.end());
    };
    trim(lhs);
    trim(rhs);

    if (lhs.empty() || rhs.empty()) return;

    auto lhsRangePos = lhs.find('[');
    auto rhsRangePos = rhs.find('[');
    
    bool lhsIsSimpleVector = (lhsRangePos != std::string::npos) && 
                            (lhs.find_first_of(" \t()&|^~?:", lhsRangePos) == std::string::npos);
    bool rhsIsSimpleVector = (rhsRangePos != std::string::npos) && 
                            (rhs.find_first_of(" \t()&|^~?:", rhsRangePos) == std::string::npos);

    if (lhsIsSimpleVector && rhsIsSimpleVector) {
        int lhsMsb, lhsLsb, rhsMsb, rhsLsb;
        parseRange(lhs, lhsMsb, lhsLsb);
        parseRange(rhs, rhsMsb, rhsLsb);

        if ((lhsMsb - lhsLsb) != (rhsMsb - rhsLsb)) {
            throw std::runtime_error("Vector width mismatch in assignment");
        }

        int width = lhsMsb - lhsLsb + 1;
        for (int i = 0; i < width; ++i) {
            std::string lhsBit = lhs.substr(0, lhsRangePos) + "[" + std::to_string(lhsMsb - i) + "]";
            std::string rhsBit = rhs.substr(0, rhsRangePos) + "[" + std::to_string(rhsMsb - i) + "]";
            int tempCounter = 0;
            parseExpression(rhsBit, lhsBit, circuit, tempCounter);
        }
    }
    else if (isVectorBase(lhs, circuit)) {
        std::vector<std::string> lhsBits = getVectorBits(lhs, circuit);
        
        for (const std::string& lhsBit : lhsBits) {
            std::string rhsBit = rhs;
            
            std::vector<std::string> vectorBases;
            for (const auto& in : circuit.inputs) {
                size_t bracketPos = in.find('[');
                if (bracketPos != std::string::npos) {
                    std::string base = in.substr(0, bracketPos);
                    if (std::find(vectorBases.begin(), vectorBases.end(), base) == vectorBases.end()) {
                        vectorBases.push_back(base);
                    }
                }
            }
            
            std::string tempRhs = rhsBit;
            for (const std::string& base : vectorBases) {
                if (isVectorBase(base, circuit)) {
                    std::vector<std::string> baseBits = getVectorBits(base, circuit);
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
        parseExpression(rhs, lhs, circuit, tempCounter);
    }
}

void VerilogParser::parseRange(const std::string& s, int& msb, int& lsb) {
    size_t lb = s.find('['), rb = s.find(']');
    std::string r = s.substr(lb+1, rb-lb-1);
    size_t colonPos = r.find(':');
    msb = std::stoi(r.substr(0, colonPos));
    lsb = std::stoi(r.substr(colonPos+1));
}

std::string VerilogParser::extractBaseName(const std::string& signal) {
    size_t pos = signal.find('[');
    if (pos != std::string::npos) {
        return signal.substr(0, pos);
    }
    return signal;
}

bool VerilogParser::isVectorBase(const std::string& name, const LogicCircuit& circuit) {
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

std::vector<std::string> VerilogParser::getVectorBits(const std::string& baseName, const LogicCircuit& circuit) {
    std::vector<std::string> bits;
    
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
    
    std::sort(bits.begin(), bits.end(), [](const std::string& a, const std::string& b) {
        size_t a_start = a.find('[') + 1;
        size_t a_end = a.find(']', a_start);
        size_t b_start = b.find('[') + 1;
        size_t b_end = b.find(']', b_start);
        
        if (a_start != std::string::npos && a_end != std::string::npos && 
            b_start != std::string::npos && b_end != std::string::npos) {
            int a_idx = std::stoi(a.substr(a_start, a_end - a_start));
            int b_idx = std::stoi(b.substr(b_start, b_end - b_start));
            return a_idx > b_idx;
        }
        return a < b;
    });
    
    return bits;
}

std::string VerilogParser::rewriteExpressionForBit(const std::string& expr, const std::string& baseName, 
                                                 const std::string& bitSignal, const LogicCircuit& circuit) {
    std::string result = expr;
    size_t pos = 0;
    
    while ((pos = result.find(baseName, pos)) != std::string::npos) {
        bool beforeOk = (pos == 0) || !std::isalnum(static_cast<unsigned char>(result[pos-1]));
        size_t afterPos = pos + baseName.length();
        bool afterOk = (afterPos >= result.length()) || !std::isalnum(static_cast<unsigned char>(result[afterPos]));
        
        if (beforeOk && afterOk) {
            result.replace(pos, baseName.length(), bitSignal);
            pos += bitSignal.length();
        } else {
            pos += baseName.length();
        }
    }
    
    return result;
}

int VerilogParser::getVectorWidth(const std::string& baseName, const LogicCircuit& circuit) {
    int width = 0;
    for (const auto& out : circuit.outputs) {
        if (out.find(baseName + "[") == 0) {
            width++;
        }
    }
    if (width == 0) {
        for (const auto& in : circuit.inputs) {
            if (in.find(baseName + "[") == 0) {
                width++;
            }
        }
    }
    return width;
}

LogicCircuit VerilogParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file");
    }

    LogicCircuit circuit;
    std::string line;
    while (std::getline(file, line)) {
        auto commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        if (line.find("module") != std::string::npos) continue;
        if (line.find("endmodule") != std::string::npos) continue;

        if (line.find("input") != std::string::npos) {
            parseIO(line, circuit.inputs);
        }
        else if (line.find("output") != std::string::npos) {
            parseIO(line, circuit.outputs);
        }
        else if (line.find("assign") != std::string::npos) {
            parseAssignment(line, circuit);
        }
    }
    return circuit;
}
