module vector_example(input [3:0] a, input [3:0] b, output [3:0] y);
    assign y = a & b; // AND of two 4-bit vectors
endmodule
