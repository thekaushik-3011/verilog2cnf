// Design 2: Equivalent 4-bit AND using De Morgan's Law
// a & b = ~(~a | ~b)
module test_circuit(
    input [3:0] a,
    input [3:0] b,
    output [3:0] y
);
    // Implement AND using NOT and OR (De Morgan's Law)
    assign y = ~(~a | ~b);
endmodule