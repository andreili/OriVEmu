`timescale 1ps/1ps

module clk_sel
(
    input   wire                        i_clk1,
    input   wire                        i_clk2,
    input   wire                        i_sel,
    output  wire                        o_clk
);

    logic and11, and12, and21, and22;
    logic[1:0] sync1, sync2;

    assign and11 = (!i_sel) & (!sync2[1]);
    assign and12 = (!sync1[1]) & i_sel;

    always_ff @(posedge i_clk1)
        sync1 <= { sync1[0], and11 };
    always_ff @(posedge i_clk2)
        sync2 <= { sync2[0], and12 };
    
    assign and21 = sync1[1] & i_clk1;
    assign and22 = sync2[1] & i_clk2;

    assign o_clk = and21 | and22;

endmodule
