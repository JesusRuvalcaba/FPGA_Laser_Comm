// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// -------------------------------------------------------------------------------

`timescale 1 ps / 1 ps

(* BLOCK_STUB = "true" *)
module cpu (
  Clk,
  Reset,
  IO_addr_strobe,
  IO_address,
  IO_byte_enable,
  IO_read_data,
  IO_read_strobe,
  IO_ready,
  IO_write_data,
  IO_write_strobe
);

  (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK.Clk CLK" *)
  (* X_INTERFACE_MODE = "slave CLK.Clk" *)
  (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME CLK.Clk, FREQ_HZ 100000000, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN bd_3914_Clk, ASSOCIATED_BUSIF , ASSOCIATED_PORT , ASSOCIATED_RESET , INSERT_VIP 0, ASSOCIATED_ASYNC_RESET Reset, BOARD.ASSOCIATED_PARAM CLK_BOARD_INTERFACE" *)
  input Clk;
  (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RST.Reset RST" *)
  (* X_INTERFACE_MODE = "slave RST.Reset" *)
  (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME RST.Reset, POLARITY ACTIVE_HIGH, INSERT_VIP 0, BOARD.ASSOCIATED_PARAM RESET_BOARD_INTERFACE" *)
  input Reset;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO ADDR_STROBE" *)
  (* X_INTERFACE_MODE = "master IO" *)
  output IO_addr_strobe;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO ADDRESS" *)
  output [31:0]IO_address;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO BYTE_ENABLE" *)
  output [3:0]IO_byte_enable;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO READ_DATA" *)
  input [31:0]IO_read_data;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO READ_STROBE" *)
  output IO_read_strobe;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO READY" *)
  input IO_ready;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO WRITE_DATA" *)
  output [31:0]IO_write_data;
  (* X_INTERFACE_INFO = "xilinx.com:interface:mcsio_bus:1.0 IO WRITE_STROBE" *)
  output IO_write_strobe;

  // stub module has no contents

endmodule
