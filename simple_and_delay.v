// simple_and_delay.v - Simple AND gate with #(2) propagation delay
module simple_and_delay(input a, input b, output out1);
    and # (5) o1 (out1, a, b);
endmodule
