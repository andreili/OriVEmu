`timescale 1ps/1ps

module emulate
(
    input   wire                        i_clk,
    input   wire                        i_reset_n
);

    // interfaces to emulator
    logic[31:0] rom1_addr       /* verilator public */;
    logic[31:0] rom2_addr       /* verilator public */;
    logic[31:0] rom_disk_addr   /* verilator public */;
    logic[31:0] rom1_rdata      /* verilator public */;
    logic[31:0] rom2_rdata      /* verilator public */;
    logic[31:0] rom_disk_rdata  /* verilator public */;
    logic[31:0] cfg_sw          /* verilator public */;

    orion_pro_top
    #(
        .TURBO_CLK_10                   (1'b1)
    )
    u_orion_core
    (
        .i_clk                          (i_clk),
        .i_reset_n                      (i_reset_n),
        .i_cfg_sw                       (cfg_sw[7:0]),
        .o_rom1_addr                    (rom1_addr[12:0]),
        .o_rom2_addr                    (rom2_addr[19:0]),
        .o_rom_dsk_addr                 (rom_disk_addr[19:0]),
        .i_rom1_rdata                   (rom1_rdata[7:0]),
        .i_rom2_rdata                   (rom2_rdata[7:0]),
        .i_rom_dsk_rdata                (rom_disk_rdata[7:0])
    );

initial
begin
    rom1_addr = '0;
    rom2_addr = '0;
    rom_disk_addr = '0;
end

endmodule
