; -----------------------------------------------------------------------------
; aie_asm_init.asm — COMBINED full-duplex: 24-col SHIM MM2S ch0 + SHIM S2MM ch0
;                    + 4-col PLIO (7/11/19/23).
;
; Per column N (SHIM base = N<<25, MEM row1 = SHIM|0x100000):
;
;   WRITE path / DDR read (all 24 cols) — SHIM MM2S ch0 -> MEM S2MM ch0:
;     SHIM NoC DMA MM2S ch0 -> Mux South3 -> SHIM slave South_3 (idx5)
;       -> SHIM master North0 -> MEM slave South_0 (idx7) -> MEM master DMA0
;       -> MEM S2MM ch0.   (South_3 is used, NOT South_0 which is reserved for PLIO.)
;
;   READ path / DDR write (all 24 cols) — MEM MM2S ch0 -> SHIM S2MM ch0:
;     MEM MM2S ch0 -> MEM slave DMA_0 (idx0) -> MEM master South0
;       -> SHIM slave North_0 (idx14) -> SHIM master South1
;       -> Demux (South1 -> NoC-module DMA) -> SHIM S2MM ch0 -> DDR.
;
;   PLIO path (cols 7/11/19/23) — PL IP -> MEM S2MM ch1:
;     PL IP -> downsizer (South0+South1 128-bit combine) -> SHIM slave South_0
;       (idx2) -> SHIM master North1 -> MEM slave South_1 (idx8)
;       -> MEM master DMA1 -> MEM S2MM ch1.
;
; Verified offsets: Mux 0x2104, Demux 0x2108, SHIM slave South_3 0x3F114 /
;   South_0 0x3F108 / North_0 0x3F138, SHIM master North0 0x3F030 /
;   North1 0x3F034 / South1 0x3F00C, MEM slave South_0 0x1B011C /
;   South_1 0x1B0120 / DMA_0 0x1B0100, MEM master DMA0 0x1B0000 /
;   DMA1 0x1B0004 / South0 0x1B001C.
; -----------------------------------------------------------------------------

; ===== col 0 (base 0x00000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x00002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0003F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0003F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x001B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x001B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x001B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x001B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0003F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0003F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x00002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 1 (base 0x02000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x02002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0203F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0203F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x021B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x021B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x021B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x021B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0203F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0203F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x02002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 2 (base 0x04000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x04002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0403F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0403F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x041B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x041B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x041B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x041B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0403F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0403F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x04002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 3 (base 0x06000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x06002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0603F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0603F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x061B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x061B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x061B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x061B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0603F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0603F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x06002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 4 (base 0x08000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x08002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0803F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0803F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x081B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x081B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x081B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x081B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0803F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0803F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x08002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 5 (base 0x0A000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x0A002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0A03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0A03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x0A1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x0A1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x0A1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x0A1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0A03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0A03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x0A002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 6 (base 0x0C000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x0C002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0C03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0C03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x0C1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x0C1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x0C1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x0C1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0C03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0C03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x0C002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 7 (base 0x0E000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x0E002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x0E03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x0E03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x0E1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x0E1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x0E1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x0E1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x0E03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x0E03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x0E002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM
; --- col 7 also PLIO -> MEM S2MM ch1 ---
MASK_WRITE_32    0x0E03F108, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_0 enable (PL in)
MASK_WRITE_32    0x0E03F034, 0xFFFFFFFF, 0x80000002   ; SHIM Master_Config_North1 <- South_0 (idx2)
MASK_WRITE_32    0x0E03000C, 0x00000001, 0x00000000   ; Downsizer_Bypass South0 = 0
MASK_WRITE_32    0x0E03000C, 0x00000002, 0x00000000   ; Downsizer_Bypass South1 = 0
MASK_WRITE_32    0x0E030004, 0x00000004, 0x00000004   ; South0_South1_128_combine = 1
MASK_WRITE_32    0x0E030008, 0x00000003, 0x00000003   ; Downsizer_Enable South0+South1
MASK_WRITE_32    0x0E1B0120, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_1 enable
MASK_WRITE_32    0x0E1B0004, 0xFFFFFFFF, 0x80000008   ; MEM Master_Config_DMA1 <- South_1 (idx8)

; ===== col 8 (base 0x10000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x10002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1003F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1003F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x101B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x101B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x101B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x101B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1003F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1003F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x10002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 9 (base 0x12000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x12002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1203F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1203F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x121B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x121B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x121B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x121B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1203F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1203F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x12002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 10 (base 0x14000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x14002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1403F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1403F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x141B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x141B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x141B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x141B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1403F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1403F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x14002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 11 (base 0x16000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x16002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1603F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1603F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x161B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x161B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x161B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x161B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1603F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1603F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x16002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM
; --- col 11 also PLIO -> MEM S2MM ch1 ---
MASK_WRITE_32    0x1603F108, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_0 enable (PL in)
MASK_WRITE_32    0x1603F034, 0xFFFFFFFF, 0x80000002   ; SHIM Master_Config_North1 <- South_0 (idx2)
MASK_WRITE_32    0x1603000C, 0x00000001, 0x00000000   ; Downsizer_Bypass South0 = 0
MASK_WRITE_32    0x1603000C, 0x00000002, 0x00000000   ; Downsizer_Bypass South1 = 0
MASK_WRITE_32    0x16030004, 0x00000004, 0x00000004   ; South0_South1_128_combine = 1
MASK_WRITE_32    0x16030008, 0x00000003, 0x00000003   ; Downsizer_Enable South0+South1
MASK_WRITE_32    0x161B0120, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_1 enable
MASK_WRITE_32    0x161B0004, 0xFFFFFFFF, 0x80000008   ; MEM Master_Config_DMA1 <- South_1 (idx8)

; ===== col 12 (base 0x18000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x18002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1803F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1803F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x181B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x181B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x181B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x181B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1803F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1803F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x18002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 13 (base 0x1A000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x1A002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1A03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1A03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x1A1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x1A1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x1A1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x1A1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1A03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1A03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x1A002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 14 (base 0x1C000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x1C002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1C03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1C03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x1C1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x1C1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x1C1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x1C1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1C03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1C03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x1C002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 15 (base 0x1E000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x1E002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x1E03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x1E03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x1E1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x1E1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x1E1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x1E1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x1E03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x1E03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x1E002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 16 (base 0x20000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x20002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2003F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2003F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x201B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x201B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x201B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x201B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2003F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2003F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x20002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 17 (base 0x22000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x22002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2203F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2203F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x221B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x221B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x221B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x221B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2203F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2203F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x22002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 18 (base 0x24000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x24002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2403F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2403F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x241B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x241B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x241B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x241B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2403F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2403F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x24002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 19 (base 0x26000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x26002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2603F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2603F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x261B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x261B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x261B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x261B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2603F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2603F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x26002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM
; --- col 19 also PLIO -> MEM S2MM ch1 ---
MASK_WRITE_32    0x2603F108, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_0 enable (PL in)
MASK_WRITE_32    0x2603F034, 0xFFFFFFFF, 0x80000002   ; SHIM Master_Config_North1 <- South_0 (idx2)
MASK_WRITE_32    0x2603000C, 0x00000001, 0x00000000   ; Downsizer_Bypass South0 = 0
MASK_WRITE_32    0x2603000C, 0x00000002, 0x00000000   ; Downsizer_Bypass South1 = 0
MASK_WRITE_32    0x26030004, 0x00000004, 0x00000004   ; South0_South1_128_combine = 1
MASK_WRITE_32    0x26030008, 0x00000003, 0x00000003   ; Downsizer_Enable South0+South1
MASK_WRITE_32    0x261B0120, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_1 enable
MASK_WRITE_32    0x261B0004, 0xFFFFFFFF, 0x80000008   ; MEM Master_Config_DMA1 <- South_1 (idx8)

; ===== col 20 (base 0x28000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x28002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2803F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2803F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x281B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x281B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x281B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x281B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2803F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2803F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x28002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 21 (base 0x2A000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x2A002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2A03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2A03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x2A1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x2A1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x2A1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x2A1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2A03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2A03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x2A002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 22 (base 0x2C000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x2C002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2C03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2C03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x2C1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x2C1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x2C1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x2C1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2C03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2C03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x2C002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM

; ===== col 23 (base 0x2E000000) =====
; --- DDR read: SHIM MM2S ch0 -> MEM S2MM ch0 ---
MASK_WRITE_32    0x2E002104, 0x00000C00, 0x00000400   ; Mux: South3 in = NoC DMA MM2S
MASK_WRITE_32    0x2E03F114, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_3 enable
MASK_WRITE_32    0x2E03F030, 0xFFFFFFFF, 0x80000005   ; SHIM Master_Config_North0 <- South_3 (idx5)
MASK_WRITE_32    0x2E1B011C, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_0 enable
MASK_WRITE_32    0x2E1B0000, 0xFFFFFFFF, 0x80000007   ; MEM Master_Config_DMA0 <- South_0 (idx7)
; --- DDR write: MEM MM2S ch0 -> SHIM S2MM ch0 ---
MASK_WRITE_32    0x2E1B0100, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_DMA_0 enable
MASK_WRITE_32    0x2E1B001C, 0xFFFFFFFF, 0x80000000   ; MEM Master_Config_South0 <- DMA_0 (idx0)
MASK_WRITE_32    0x2E03F138, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_North_0 enable
MASK_WRITE_32    0x2E03F00C, 0xFFFFFFFF, 0x8000000E   ; SHIM Master_Config_South1 <- North_0 (idx14)
MASK_WRITE_32    0x2E002108, 0x0000000C, 0x00000004   ; Demux: South1 out = NoC DMA S2MM
; --- col 23 also PLIO -> MEM S2MM ch1 ---
MASK_WRITE_32    0x2E03F108, 0xFFFFFFFF, 0x80000000   ; SHIM Slave_Config_South_0 enable (PL in)
MASK_WRITE_32    0x2E03F034, 0xFFFFFFFF, 0x80000002   ; SHIM Master_Config_North1 <- South_0 (idx2)
MASK_WRITE_32    0x2E03000C, 0x00000001, 0x00000000   ; Downsizer_Bypass South0 = 0
MASK_WRITE_32    0x2E03000C, 0x00000002, 0x00000000   ; Downsizer_Bypass South1 = 0
MASK_WRITE_32    0x2E030004, 0x00000004, 0x00000004   ; South0_South1_128_combine = 1
MASK_WRITE_32    0x2E030008, 0x00000003, 0x00000003   ; Downsizer_Enable South0+South1
MASK_WRITE_32    0x2E1B0120, 0xFFFFFFFF, 0x80000000   ; MEM Slave_Config_South_1 enable
MASK_WRITE_32    0x2E1B0004, 0xFFFFFFFF, 0x80000008   ; MEM Master_Config_DMA1 <- South_1 (idx8)
