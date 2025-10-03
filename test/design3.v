// Design 3: Different functionality (OR instead of AND)
// This should be detected as NOT equivalent
module test_circuit(
    input [3:0] a,
    input [3:0] b,
    output [3:0] y
);
    // OR operation instead of AND
    assign y = a | b;
endmodule