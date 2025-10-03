#include "ver2cnf.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

class EquivalenceChecker {
private:
    // ---- Rename circuit wires/outputs with suffix to avoid clashes, but keep primary inputs shared ----
    static LogicCircuit renameCircuit(const LogicCircuit& original, const std::string& suffix, 
                                     const std::unordered_set<std::string>& primaryInputs) {
        LogicCircuit renamed;
        renamed.name = original.name + suffix;

        std::unordered_map<std::string, std::string> signalMap;

        // Handle all wires: primary inputs stay the same, others get suffix
        for (const auto& wire : original.wires) {
            if (primaryInputs.find(wire) != primaryInputs.end()) {
                // Primary input - keep original name (shared)
                signalMap[wire] = wire;
                renamed.wires.insert(wire);
            } else {
                // Internal signal or output - rename with suffix
                std::string newName = wire + suffix;
                signalMap[wire] = newName;
                renamed.wires.insert(newName);
            }
        }

        // Primary inputs are shared (not renamed)
        renamed.inputs = primaryInputs;

        // Rename outputs (outputs are not primary inputs)
        std::unordered_set<std::string> newOutputs;
        for (const auto& out : original.outputs) {
            newOutputs.insert(signalMap[out]);
        }
        renamed.outputs = newOutputs;

        // Rename gates
        for (const auto& gate : original.gates) {
            std::vector<std::string> newInputs;
            for (const auto& in : gate.inputs) {
                newInputs.push_back(signalMap[in]);
            }
            std::string newOutput = signalMap[gate.output];
            renamed.addGate(Gate(gate.type, newInputs, newOutput));
        }

        return renamed;
    }

    // ---- Add XOR gate (used for output comparison) ----
    static void addXORGate(LogicCircuit& circuit, const std::string& a, const std::string& b, const std::string& output) {
        circuit.addGate(Gate(Gate::Type::XOR, {a, b}, output));
    }

    // ---- Add OR gate that handles >2 inputs by chaining ----
    static void addORGate(LogicCircuit& circuit, const std::vector<std::string>& inputs, const std::string& output) {
        if (inputs.empty()) return;
        if (inputs.size() == 1) {
            circuit.addGate(Gate(Gate::Type::BUF, {inputs[0]}, output));
            return;
        }

        std::string current = inputs[0];
        for (size_t i = 1; i < inputs.size(); i++) {
            std::string tempName = (i == inputs.size() - 1) ? output : "equiv_or_temp_" + std::to_string(i);
            circuit.addGate(Gate(Gate::Type::OR, {current, inputs[i]}, tempName));
            current = tempName;
        }
    }

public:
    static bool checkEquivalence(const std::string& file1, const std::string& file2) {
        try {
            // ---- Parse both circuits ----
            LogicCircuit circuit1 = VerilogParser::parse(file1);
            LogicCircuit circuit2 = VerilogParser::parse(file2);

            std::cout << "Parsed Circuit 1: " << circuit1.inputs.size() << " inputs, "
                      << circuit1.outputs.size() << " outputs" << std::endl;
            std::cout << "Parsed Circuit 2: " << circuit2.inputs.size() << " inputs, "
                      << circuit2.outputs.size() << " outputs" << std::endl;

            // ---- Check input/output compatibility ----
            std::vector<std::string> inputs1(circuit1.inputs.begin(), circuit1.inputs.end());
            std::vector<std::string> inputs2(circuit2.inputs.begin(), circuit2.inputs.end());
            std::vector<std::string> outputs1(circuit1.outputs.begin(), circuit1.outputs.end());
            std::vector<std::string> outputs2(circuit2.outputs.begin(), circuit2.outputs.end());

            std::sort(inputs1.begin(), inputs1.end());
            std::sort(inputs2.begin(), inputs2.end());
            std::sort(outputs1.begin(), outputs1.end());
            std::sort(outputs2.begin(), outputs2.end());

            if (inputs1 != inputs2) {
                std::cerr << "Error: Circuits have different inputs!" << std::endl;
                return false;
            }
            if (outputs1 != outputs2) {
                std::cerr << "Error: Circuits have different outputs!" << std::endl;
                return false;
            }

            if (inputs1.empty() || outputs1.empty()) {
                std::cerr << "Error: Circuits must have inputs and outputs!" << std::endl;
                return false;
            }

            // ---- Combine circuits with shared inputs but renamed internal signals ----
            LogicCircuit combined;
            std::unordered_set<std::string> primaryInputs = circuit1.inputs;
            combined.inputs = primaryInputs;

            LogicCircuit c1_renamed = renameCircuit(circuit1, "_c1", primaryInputs);
            LogicCircuit c2_renamed = renameCircuit(circuit2, "_c2", primaryInputs);

            for (const auto& gate : c1_renamed.gates) combined.addGate(gate);
            for (const auto& gate : c2_renamed.gates) combined.addGate(gate);

            combined.wires.insert(c1_renamed.wires.begin(), c1_renamed.wires.end());
            combined.wires.insert(c2_renamed.wires.begin(), c2_renamed.wires.end());

            // ---- Add XORs to detect differences in outputs ----
            std::vector<std::string> diffSignals;
            for (const auto& out : outputs1) {
                std::string out1 = out + "_c1";
                std::string out2 = out + "_c2";
                std::string diff = "diff_" + out;
                addXORGate(combined, out1, out2, diff);
                diffSignals.push_back(diff);
                combined.wires.insert(diff);
            }

            // ---- OR all diff signals into anyDiff ----
            std::string anyDiff = "any_diff";
            addORGate(combined, diffSignals, anyDiff);
            combined.wires.insert(anyDiff);

            // ---- Convert to CNF ----
            CNFConverter converter;
            auto cnf = converter.circuitToCNF(combined);
            auto varMap = converter.getVariableMap();

            // Force SAT query: any_diff = 1
            int anyDiffVar = varMap[anyDiff];
            cnf.push_back({anyDiffVar});

            // ---- Write CNF to file ----
            std::ofstream out("equivalence.cnf");
            out << "c Equivalence checking CNF\n";
            out << "c SAT = circuits differ, UNSAT = circuits equivalent\n";
            out << "p cnf " << converter.getNumVariables() << " " << cnf.size() << "\n";
            for (const auto& clause : cnf) {
                for (int lit : clause) out << lit << " ";
                out << "0\n";
            }
            out.close();

            std::cout << "Equivalence CNF written to equivalence.cnf" << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }
};

// ---- Driver ----
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./equiv_checker <verilog_file1> <verilog_file2>" << std::endl;
        std::cerr << "Generates equivalence.cnf for SAT-based equivalence checking." << std::endl;
        return 1;
    }

    if (EquivalenceChecker::checkEquivalence(argv[1], argv[2])) {
        std::cout << "\n✅ Run SAT solver on equivalence.cnf:" << std::endl;
        std::cout << "   UNSATISFIABLE → circuits are equivalent" << std::endl;
        std::cout << "   SATISFIABLE → circuits are different" << std::endl;
    } else {
        return 1;
    }

    return 0;
}