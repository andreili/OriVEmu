`timescale 1ps/1ps

module i8255
(
    input   wire        i_clk,
    input   wire[1:0]   i_addr,
    input   wire[7:0]   i_data,
    output  wire[7:0]   o_data,
    input   wire        i_rd_n,
    input   wire        i_wr_n,
    input   wire        i_cs_n,
    input   wire        i_reset,
    input   wire[7:0]   i_PA,
    output  wire[7:0]   o_PA,
    input   wire[7:0]   i_PB,
    output  wire[7:0]   o_PB,
    input   wire[7:0]   i_PC,
    output  wire[7:0]   o_PC
);

    logic[7:0] reg_A;
    logic[7:0] reg_B;
    logic[7:0] reg_C;
    logic[7:0] control_reg;

    logic[7:0] Ain;
    logic[7:0] Bin;
    logic[7:0] Cin;
    logic[3:0] Cinl;
    logic[3:0] Cinh;

    always_ff @(posedge i_clk)
    begin
        if (i_reset)
        begin
            reg_A <= '0;
            reg_B <= '0;
            reg_C <= '0;
            control_reg <= 8'h9B;
        end
        else if ((!i_cs_n) & (!i_wr_n))
        begin
            case (i_addr)
                2'b00: reg_A <= i_data;
                2'b01: reg_B <= i_data;
                2'b10: reg_C <= i_data;
                2'b11:
                    begin
                        if (i_data[7] == 1'b1)	// immediate value
                            control_reg <= i_data;
                        else	// set/reset bit in port C
                        begin
                            if (i_data[1] == '1) // set bit
                                reg_C[i_data[3:1]] <= '1;
                            else // reset bit
                                reg_C[i_data[3:1]] <= '0;
                        end
                    end
            endcase
        end
    end

    logic[7:0] reg_o_A, reg_o_B, reg_o_C;
    always_ff @(posedge i_clk)
    begin
        if (control_reg[4] == '0)
            reg_o_A <= reg_A;

        if (control_reg[1] == '0)
            reg_o_B <= reg_B;

        if (control_reg[0] == '0)
            reg_o_C[3:0] <= reg_C[3:0];

        if (control_reg[3] == '0)
            reg_o_C[7:4] <= reg_C[7:4];
    end;

    assign o_PA = reg_o_A;
    assign o_PB = reg_o_B;
    assign o_PC = reg_o_C;
    assign Cinl	= control_reg[0] ? i_PC[3:0] : reg_C[3:0];
    assign Bin	= control_reg[1] ? i_PB      : reg_B;
    assign Cinh	= control_reg[3] ? i_PC[7:4] : reg_C[7:4];
    assign Ain	= control_reg[4] ? i_PA : reg_A;
    assign Cin	= { Cinh, Cinl };

    logic[7:0] r_data;
    always_ff @(posedge i_clk)
    begin
        if ((!i_cs_n) & (!i_rd_n))
        begin
            case (i_addr)
            2'b00: r_data <= Ain;
            2'b01: r_data <= Bin;
            2'b10: r_data <= Cin;
            2'b11: r_data <= control_reg;
            endcase;
        end
    end;
    assign o_data = r_data;

endmodule
