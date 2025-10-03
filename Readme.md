# Verilog-to-CNF Converter

This project parses **combinational Verilog code** and converts it into a **CNF (Conjunctive Normal Form)** representation suitable for SAT solvers.

It supports standard logic gates (`AND`, `OR`, `NOT`, `XOR`, `XNOR`, `NAND`, `NOR`, `BUF`, `MUX`) and handles both scalar and vector signals in Verilog.

---

## Features

* **Verilog Parser**

  * Supports `input`, `output`, `assign`, and bit-vector declarations (`[msb:lsb]`).
  * Handles scalar and vector assignments, including bit-level mappings.
  * Supports logic expressions with parentheses, chained operators, and ternary operators (`?:`).

* **Logic Circuit Representation**

  * Internally represents the parsed circuit as a directed graph of gates and wires.

* **CNF Conversion**

  * Converts each gate into its CNF equivalent using Tseitin encoding.
  * Ensures CNF is compact and ready for SAT solving.
  * Produces variable-to-signal mapping for debugging.

* **Output**

  * DIMACS CNF file (`circuit.cnf`)
  * Human-readable mapping from signal names to CNF variables.

---

## Requirements

* C++17 or later
* Standard build tools (`g++` or `clang++`)

---

## Build

```bash
g++ -std=c++17 -O2 -o sat_cnf sat_cnf.cpp
````

Alternatively, you can use the provided **Makefile**:

```makefile
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall

ver2cnf: ver2cnf.cpp
	$(CXX) $(CXXFLAGS) -o ver2cnf ver2cnf.cpp

equiv_checker: equiv_checker.cpp ver2cnf.cpp
	$(CXX) $(CXXFLAGS) -o equiv_checker equiv_checker.cpp ver2cnf.cpp

clean:
	rm -f ver2cnf equiv_checker equivalence.cnf circuit.cnf

.PHONY: clean
```

---

## Usage

```bash
./sat_cnf <verilog_file>
```

Example:

```bash
./sat_cnf ripple_adder.v
```

---

## Output

1. **Console Output**
   Shows mapping from Verilog signals to CNF variables:

   ```
   c Variable mapping (signal_name -> variable_number):
   c a[0] -> 12
   c b[0] -> 9
   c sum[0] -> 17
   ...
   ```

2. **CNF File (`circuit.cnf`)**
   Standard DIMACS format:

   ```
   c CNF generated from Verilog combinational logic
   p cnf 31 76
   -12 -9 -17 0
   12 9 -17 0
   ...
   ```

   * First line describes the number of variables and clauses.
   * Each clause ends with `0`.
   * Variables are mapped in the console output.

---

## Example Verilog Input

```verilog
module half_adder(input a, input b, output sum, output cout);
  assign sum = a ^ b;
  assign cout = a & b;
endmodule
```

### Corresponding CNF Output (excerpt)

```
c CNF generated from Verilog combinational logic
p cnf 4 6
-1 -2 3 0
1 2 3 0
1 -2 -3 0
-1 2 -3 0
-4 1 0
-4 2 0
```

Here:

* `a -> 1`, `b -> 2`, `sum -> 3`, `cout -> 4`

---

## File Structure

```
.
├── main.cpp          # Core implementation
├── sat_cnf.cpp       # Core code for Verilog-to-CNF conversion
├── ver2cnf.cpp       # Used for CNF conversion (supporting code)
├── ver2cnf.h         # Header for conversion utilities
├── equiv_checker.cpp # Checks equivalence of circuits using MiniSat
├── Makefile          # Build automation
├── circuit.cnf       # Generated CNF output (after running)
└── README.md         # Documentation
```

---

## Notes

* `sat_cnf.cpp` is the **core code** responsible for parsing and converting Verilog into CNF.
* The other `.cpp` and `.h` files (`ver2cnf.cpp`, `ver2cnf.h`, `equiv_checker.cpp`) are supporting modules for conversion and equivalence checking with **MiniSat**.
* Only **combinational logic** is supported (no sequential elements like flip-flops).
* CNF is directly usable in SAT solvers like **MiniSat**, **Glucose**, etc.
* Signal names are mapped to CNF variables to aid debugging.

---
