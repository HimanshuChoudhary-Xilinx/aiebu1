..
    comment:: SPDX-License-Identifier: MIT
    comment:: Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

===================
DMA compiler ctrlcode + ctrkpkt + pmload test
===================

Input
=====

1. dma_txn.bin -- TXN ctrlcode binary. Checked in as *base64* encoded ml_txn.b64 ascii file which is decoded into binary before use with the help of **b64.cmake**
2. dma_ctrl_pkt.bin -- Control Packet. Checked in as *base64* encoded ctrl_pkt0.b64 which is decoded into binary before use with the help of **b64.cmake**
3. external_buffer_id.json -- Buffer metadata with address patch information

Output
======

dma_txn_ctrlpkt_pmload.elf -- ELF file for loading by XRT and execution by NPU FW.

Command
=======

``ctest`` in this directory runs the following commands::

   cmake -P b64.cmake -d ml_txn.b64 dma_txn.bin
   cmake -P b64.cmake -d ctrl_pkt0.b64 dma_ctrl_pkt.bin
   aiebu-asm -r -t aie2txn -c dma_txn.bin -p dma_ctrl_pkt.bin -j external_buffer_id.json -o dma_txn_ctrlpkt_pmload.elf