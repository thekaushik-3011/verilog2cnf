# SAT-CNF Generator from Verilog

This project converts a **combinational Verilog design** into a **CNF formula** (Conjunctive Normal Form) suitable for SAT solvers like [MiniSAT](http://minisat.se).
It also supports **test pattern generation** and **custom output constraints**.

---

## Features

* Parses a **subset of Verilog**:

  * `module ... endmodule`
  * `input`, `output`, `wire`
  * `assign` statements with `&`, `|`, `^`, `~`
  * Vector signals like `[3:0]`
* Converts logic into **CNF clauses** (DIMACS format).
* Allows automatic test pattern generation:

  1. All outputs = `1`
  2. All outputs = `0`
  3. Toggle each output individually
  4. Custom constraints (`signal=0/1`)
* Exports CNF to `circuit.cnf` (or multiple `.cnf` files for toggle mode).
* Works with external SAT solvers (MiniSAT, Glucose, Kissat, etc.).

---

## Prerequisites

* C++ compiler (g++/clang++)
* [MiniSAT](https://github.com/niklasso/minisat) or any DIMACS-compatible SAT solver.

Install MiniSAT on Ubuntu/Debian:

```bash
sudo apt-get install minisat
```

---

## Build

Compile the program:

```bash
g++ -std=c++17 -O2 sat_cnf.cpp -o sat_cnf
```

---

## Usage

Run the tool with a Verilog file:

```bash
./sat_cnf <verilog_file>
```

Example:

```bash
./sat_cnf and_gate.v
```

You will then be asked to choose a **test pattern type**:

```
Choose test pattern type:
1. All outputs = 1
2. All outputs = 0
3. Toggle each output individually
4. Custom constraints
Choice:
```

---

## Generated Files

* **`circuit.cnf`** → CNF file with constraints for case 1, 2, or 4.
* **`toggle_<output>.cnf`** → multiple CNFs (one per output forced to `1`) for case 3.

---

## Running SAT Solver

After generating CNF, solve it using MiniSAT:

```bash
minisat circuit.cnf result.out
```

* `SAT` → circuit is satisfiable, and `result.out` contains variable assignments.
* `UNSAT` → no input assignment satisfies constraints.

Check `result.out`:

```
SAT
1 -2 3 -4 5 ...
```

Here, positive = variable true, negative = variable false.
Refer to the variable mapping printed by the program.

---

## Example Test Cases

### Simple AND Gate

`and_gate.v`

```verilog
module and_gate(input a, input b, output y);
    assign y = a & b;
endmodule
```

Run:

```bash
./sat_cnf and_gate.v
```

Choose test type → CNF saved → run MiniSAT.

---

### Half Adder

`half_adder.v`

```verilog
module half_adder(input a, input b, output sum, output carry);
    assign sum = a ^ b;
    assign carry = a & b;
endmodule
```

---

### Full Adder

`full_adder.v`

```verilog
module full_adder(input a, input b, input cin, output sum, output cout);
    assign sum = a ^ b ^ cin;
    assign cout = (a & b) | (b & cin) | (a & cin);
endmodule
```

---

### 4-bit Ripple Adder

`ripple_adder.v`

```verilog
module ripple_adder(input [3:0] a, input [3:0] b, output [3:0] sum, output cout);
    wire c1, c2, c3;
    assign sum[0] = a[0] ^ b[0];
    assign c1 = a[0] & b[0];
    assign sum[1] = a[1] ^ b[1] ^ c1;
    assign c2 = (a[1] & b[1]) | (a[1] & c1) | (b[1] & c1);
    assign sum[2] = a[2] ^ b[2] ^ c2;
    assign c3 = (a[2] & b[2]) | (a[2] & c2) | (b[2] & c2);
    assign sum[3] = a[3] ^ b[3] ^ c3;
    assign cout = (a[3] & b[3]) | (a[3] & c3) | (b[3] & c3);
endmodule
```

---

## Notes

* Only **combinational circuits** are supported.
* **Sequential elements (flip-flops, always blocks, etc.) are not parsed.**
* Extend `gateToCNF()` if you want support for more gates (NAND, NOR, MUX).

---

## License

MIT License – free to use and modify.