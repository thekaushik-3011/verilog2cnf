// Test Case 1: 4-bit ALU
// Tests vector parsing, bit-slicing, and nested ternary operators (MUXes).
module test_alu(
    input [3:0] A,
    input [3:0] B,
    input [1:0] Op, // Operation select: 00=AND, 01=OR, 10=XOR, 11=NOT A
    output [3:0] Y
);

    // This ALU's logic is implemented bit-by-bit to be compatible with the parser.
    // The selection logic is equivalent to a 4-to-1 MUX for each output bit.
    // Y = Op[1] ? (Op[0] ? ~A : A^B) : (Op[0] ? A|B : A&B);

    // --- Bit 0 Logic ---
    assign and_res_0 = A[0] & B[0];
    assign or_res_0 = A[0] | B[0];
    assign xor_res_0 = A[0] ^ B[0];
    assign not_res_0 = ~A[0];
    assign mux_out_b0_lower = Op[0] ? or_res_0 : and_res_0;
    assign mux_out_b0_upper = Op[0] ? not_res_0 : xor_res_0;
    assign Y[0] = Op[1] ? mux_out_b0_upper : mux_out_b0_lower;

    // --- Bit 1 Logic ---
    assign and_res_1 = A[1] & B[1];
    assign or_res_1 = A[1] | B[1];
    assign xor_res_1 = A[1] ^ B[1];
    assign not_res_1 = ~A[1];
    assign mux_out_b1_lower = Op[0] ? or_res_1 : and_res_1;
    assign mux_out_b1_upper = Op[0] ? not_res_1 : xor_res_1;
    assign Y[1] = Op[1] ? mux_out_b1_upper : mux_out_b1_lower;

    // --- Bit 2 Logic ---
    assign and_res_2 = A[2] & B[2];
    assign or_res_2 = A[2] | B[2];
    assign xor_res_2 = A[2] ^ B[2];
    assign not_res_2 = ~A[2];
    assign mux_out_b2_lower = Op[0] ? or_res_2 : and_res_2;
    assign mux_out_b2_upper = Op[0] ? not_res_2 : xor_res_2;
    assign Y[2] = Op[1] ? mux_out_b2_upper : mux_out_b2_lower;

    // --- Bit 3 Logic ---
    assign and_res_3 = A[3] & B[3];
    assign or_res_3 = A[3] | B[3];
    assign xor_res_3 = A[3] ^ B[3];
    assign not_res_3 = ~A[3];
    assign mux_out_b3_lower = Op[0] ? or_res_3 : and_res_3;
    assign mux_out_b3_upper = Op[0] ? not_res_3 : xor_res_3;
    assign Y[3] = Op[1] ? mux_out_b3_upper : mux_out_b3_lower;

endmodule