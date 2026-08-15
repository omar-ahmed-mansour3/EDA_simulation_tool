// comb_logic.v - Combinational Logic Circuit with delays and assign statements
module comb_logic(input in1, input in2, input in3, output out1, output out2);
    wire net_and;

    // Primitive gates with optional propagation delays
    and # (2) g1 (net_and, in1, in2);
    not # (1) g2 (out1, net_and);

    // Continuous assignment expression
    assign out2 = (in1 ^ in2) | in3;
endmodule
