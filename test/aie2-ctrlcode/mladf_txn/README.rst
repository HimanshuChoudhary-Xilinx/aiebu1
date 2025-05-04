..
    comment:: SPDX-License-Identifier: MIT
    comment:: Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

===================
MLADF ctrlcode test
===================

Input
=====

1. ml_txn.bin -- TXN ctrlcode binary. Checked in as *base64* encoded ml_txn.b64 ascii file which is decoded into binary before use with the help of **b64.cmake**

Output
======

ml_txn.elf -- ELF file for loading by XRT and execution by NPU FW.

Command
=======

``ctest`` in this directory runs the following commands::

   cmake -P b64.cmake -d ml_txn.b64 ml_txn.bin
   aiebu-asm -r -t aie2txn -c ml_txn.bin -o ml_txn.elf
