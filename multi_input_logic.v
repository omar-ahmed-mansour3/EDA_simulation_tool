// multi_input_logic.v
// Circuit with 3-input AND gates using continuous assignments (assign)
// w1 = a & b & c
// w2 = d & e & f
// out = w1 ^ w2 (XOR combination of 3-input AND outputs)

module multi_input_logic(input a, input b, input c, input d, input e, input f, output out);
    wire w1, w2;

    // Continuous assignment for 3-input AND gate 1
    assign w1 = a & b & c;

    // Continuous assignment for 3-input AND gate 2
    assign w2 = d & e & f;

    // Output assignment combining w1 and w2
    assign out = w1 ^ w2;
endmodule
