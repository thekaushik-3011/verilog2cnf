// Design 1: Simple 4-bit AND operation
module test_circuit(
    input [3:0] a,
    input [3:0] b,
    output [3:0] y
);
    // Direct AND operation
    assign y = a & b;
endmodule