module test(
    input [3:0] a, 
    input [3:0] b, 
    output [3:0] sum
);
    always @(*) begin
        sum = a + b;
    end
endmodule