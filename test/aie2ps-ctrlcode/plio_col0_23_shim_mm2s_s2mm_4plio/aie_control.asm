.aie_row_topology	 1-2-4-0
.partition	 24column
;text
;
; FULL-DUPLEX: 24-col SHIM MM2S ch0 (4MB each) + 24-col SHIM S2MM ch0 (4MB each)
;              + 4-col PLIO (7/11/19/23, 24MB each).
;
;   SHIM MM2S total : 24 x 4 MB   = 96 MB  (DDR -> AIE)
;   SHIM S2MM total : 24 x 4 MB   = 96 MB  (AIE -> DDR)
;   PLIO      total :  4 x 24 MB  = 96 MB  (DDR -> PL -> AIE)
;
; Per column N (identical on ALL 24 cols; PLIO cols add a 3rd flow):
;   DDR read : SHIM MM2S ch0 BD1 (single 4 MB BD)
;              -> MEM S2MM ch0 BD0  (256 KB @ word 0x20000, Repeat=15 -> 16x = 4 MB)
;              BD0 releases MEM lock0 +1 per pass -> poll lock0 == 16.
;   DDR write: MEM MM2S ch0 BD2  (128 KB @ word 0x30000, Repeat=31 -> 32x = 4 MB)
;              -> SHIM S2MM ch0 BD0 (single 4 MB BD) -> DDR.
;              Completion = SHIM S2MM ch0 status idle.
;   PLIO     : PL IP -> MEM S2MM ch1 BD24 (128 KB @ word 0x38000, Repeat=191
;              -> 192x = 24 MB), no lock; poll DMA_S2MM_Status_1 IDLE.
;
; MEM-tile (col N, row1) 512 KB local window map (word addresses; local base is
; 0x20000 — 0x0 is the WEST neighbour, which col0 does not have):
;   0x20000..0x2FFFF (256 KB)  S2MM ch0 sink  (SHIM MM2S data)
;   0x30000..0x37FFF (128 KB)  MM2S ch0 source (SHIM S2MM data)
;   0x38000..0x3FFFF (128 KB)  S2MM ch1 sink  (PLIO data, 4 cols only)
; The three regions are disjoint so the three MEM DMA channels never collide,
; which keeps the MM2S(read) and S2MM(write) DDR paths independent — the point
; of the test.
;
; NOTE: this is a pure bandwidth test — nothing is data-compared, so the MEM
; MM2S ch0 source region (0x30000) is never written by this test; it streams
; whatever the tile holds after partition configuration. That assumes the AIE
; partition config zero-initialises MEM-tile memory (so the ECC checkbits are
; valid). If a board run reports MEM-tile DM_ECC_Error_2bit, point the MM2S ch0
; BD at 0x20000 instead (the S2MM ch0 sink region) — bandwidth is unaffected,
; only the read/write bank separation is lost.
;
; Buffer/arg map (external_buffer_id.json):
;   arg 0..23  mm2s_src_col0..col23   4 MB each   (SHIM MM2S reads)
;   arg 24..47 s2mm_dst_col0..col23   4 MB each   (SHIM S2MM writes)
;   arg 48..51 plio_src_col7/11/19/23 24 MB each  (PL IP reads, bank_id 1)
;
; Record timers: id0 start; id1 all launched (BW window start);
;                id2 all SHIM DMA done (BW window end); id3 all MEM done / end.
; -----------------------------------------------------------------------------

START_JOB 1
        SAVE_TIMESTAMPS    0

        ; 1. Configure 4 PL IPs (ap_start deferred)
        APPLY_OFFSET_PL    @wts_params_c7, 48
        UC_DMA_WRITE_DES_SYNC    @wts_pl_cfg_chain_c7
        APPLY_OFFSET_PL    @wts_params_c11, 49
        UC_DMA_WRITE_DES_SYNC    @wts_pl_cfg_chain_c11
        APPLY_OFFSET_PL    @wts_params_c19, 50
        UC_DMA_WRITE_DES_SYNC    @wts_pl_cfg_chain_c19
        APPLY_OFFSET_PL    @wts_params_c23, 51
        UC_DMA_WRITE_DES_SYNC    @wts_pl_cfg_chain_c23

        ; 2. Patch DDR source addresses into the 24 SHIM MM2S BDs
        APPLY_OFFSET_57    @shim_mm2s_bd_c0, 1, 0
        APPLY_OFFSET_57    @shim_mm2s_bd_c1, 1, 1
        APPLY_OFFSET_57    @shim_mm2s_bd_c2, 1, 2
        APPLY_OFFSET_57    @shim_mm2s_bd_c3, 1, 3
        APPLY_OFFSET_57    @shim_mm2s_bd_c4, 1, 4
        APPLY_OFFSET_57    @shim_mm2s_bd_c5, 1, 5
        APPLY_OFFSET_57    @shim_mm2s_bd_c6, 1, 6
        APPLY_OFFSET_57    @shim_mm2s_bd_c7, 1, 7
        APPLY_OFFSET_57    @shim_mm2s_bd_c8, 1, 8
        APPLY_OFFSET_57    @shim_mm2s_bd_c9, 1, 9
        APPLY_OFFSET_57    @shim_mm2s_bd_c10, 1, 10
        APPLY_OFFSET_57    @shim_mm2s_bd_c11, 1, 11
        APPLY_OFFSET_57    @shim_mm2s_bd_c12, 1, 12
        APPLY_OFFSET_57    @shim_mm2s_bd_c13, 1, 13
        APPLY_OFFSET_57    @shim_mm2s_bd_c14, 1, 14
        APPLY_OFFSET_57    @shim_mm2s_bd_c15, 1, 15
        APPLY_OFFSET_57    @shim_mm2s_bd_c16, 1, 16
        APPLY_OFFSET_57    @shim_mm2s_bd_c17, 1, 17
        APPLY_OFFSET_57    @shim_mm2s_bd_c18, 1, 18
        APPLY_OFFSET_57    @shim_mm2s_bd_c19, 1, 19
        APPLY_OFFSET_57    @shim_mm2s_bd_c20, 1, 20
        APPLY_OFFSET_57    @shim_mm2s_bd_c21, 1, 21
        APPLY_OFFSET_57    @shim_mm2s_bd_c22, 1, 22
        APPLY_OFFSET_57    @shim_mm2s_bd_c23, 1, 23

        ; 3. Patch DDR destination addresses into the 24 SHIM S2MM BDs
        APPLY_OFFSET_57    @shim_s2mm_bd_c0, 1, 24
        APPLY_OFFSET_57    @shim_s2mm_bd_c1, 1, 25
        APPLY_OFFSET_57    @shim_s2mm_bd_c2, 1, 26
        APPLY_OFFSET_57    @shim_s2mm_bd_c3, 1, 27
        APPLY_OFFSET_57    @shim_s2mm_bd_c4, 1, 28
        APPLY_OFFSET_57    @shim_s2mm_bd_c5, 1, 29
        APPLY_OFFSET_57    @shim_s2mm_bd_c6, 1, 30
        APPLY_OFFSET_57    @shim_s2mm_bd_c7, 1, 31
        APPLY_OFFSET_57    @shim_s2mm_bd_c8, 1, 32
        APPLY_OFFSET_57    @shim_s2mm_bd_c9, 1, 33
        APPLY_OFFSET_57    @shim_s2mm_bd_c10, 1, 34
        APPLY_OFFSET_57    @shim_s2mm_bd_c11, 1, 35
        APPLY_OFFSET_57    @shim_s2mm_bd_c12, 1, 36
        APPLY_OFFSET_57    @shim_s2mm_bd_c13, 1, 37
        APPLY_OFFSET_57    @shim_s2mm_bd_c14, 1, 38
        APPLY_OFFSET_57    @shim_s2mm_bd_c15, 1, 39
        APPLY_OFFSET_57    @shim_s2mm_bd_c16, 1, 40
        APPLY_OFFSET_57    @shim_s2mm_bd_c17, 1, 41
        APPLY_OFFSET_57    @shim_s2mm_bd_c18, 1, 42
        APPLY_OFFSET_57    @shim_s2mm_bd_c19, 1, 43
        APPLY_OFFSET_57    @shim_s2mm_bd_c20, 1, 44
        APPLY_OFFSET_57    @shim_s2mm_bd_c21, 1, 45
        APPLY_OFFSET_57    @shim_s2mm_bd_c22, 1, 46
        APPLY_OFFSET_57    @shim_s2mm_bd_c23, 1, 47

        ; 4. Program everything (MEM locks + MEM BDs + SHIM BDs), no starts yet
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c0
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c1
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c2
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c3
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c4
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c5
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c6
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c7
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c8
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c9
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c10
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c11
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c12
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c13
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c14
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c15
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c16
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c17
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c18
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c19
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c20
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c21
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c22
        UC_DMA_WRITE_DES_SYNC    @cfg_chain_c23

        ; 5. Start ALL sinks: MEM S2MM ch0 (+ ch1 on PLIO cols) + SHIM S2MM ch0
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c0
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c1
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c2
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c3
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c4
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c5
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c6
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c7
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c8
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c9
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c10
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c11
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c12
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c13
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c14
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c15
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c16
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c17
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c18
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c19
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c20
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c21
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c22
        UC_DMA_WRITE_DES_SYNC    @sink_start_chain_c23

        ; 6. Start ALL sources: SHIM MM2S ch0 + MEM MM2S ch0
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c0
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c1
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c2
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c3
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c4
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c5
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c6
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c7
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c8
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c9
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c10
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c11
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c12
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c13
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c14
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c15
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c16
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c17
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c18
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c19
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c20
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c21
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c22
        UC_DMA_WRITE_DES_SYNC    @src_start_chain_c23

        ; 7. Fire the 4 PLIO ap_start together (all sinks are ready)
        UC_DMA_WRITE_DES_SYNC    @wts_pl_start_chain_c7
        UC_DMA_WRITE_DES_SYNC    @wts_pl_start_chain_c11
        UC_DMA_WRITE_DES_SYNC    @wts_pl_start_chain_c19
        UC_DMA_WRITE_DES_SYNC    @wts_pl_start_chain_c23

        SAVE_TIMESTAMPS    1

        ; 8. Wait all 4 PLIO ap_ready (bit 3): PL finished reading its input.
        ; This single AP_CTRL read is the ONLY reader of each kernel's control reg. It
        ; (a) re-arms the ap_ctrl_chain kernel for the next context-alive run, and
        ; (b) marks completion. Do NOT add a separate ap_done (bit1) poll after this:
        ; these are passthrough kernels that raise ap_ready and ap_done together and
        ; ap_done is clear-on-read, so this read already consumed it. True completion
        ; is gated by the MEM S2MM ch1 status poll below.
        UC_DMA_MASK_POLL_EXT     0x00000800, 0x00030000, 0x00000008, 0x00000008   ; col7
        UC_DMA_MASK_POLL_EXT     0x00000800, 0x00000000, 0x00000008, 0x00000008   ; col11
        UC_DMA_MASK_POLL_EXT     0x00000800, 0x00010000, 0x00000008, 0x00000008   ; col19
        UC_DMA_MASK_POLL_EXT     0x00000800, 0x00020000, 0x00000008, 0x00000008   ; col23

        ; 9. Wait all 24 SHIM MM2S ch0 done (DDR read side)
        MASK_POLL_32     0x00009328, 0x78003c, 0x0   ; col0 MM2S
        MASK_POLL_32     0x02009328, 0x78003c, 0x0   ; col1 MM2S
        MASK_POLL_32     0x04009328, 0x78003c, 0x0   ; col2 MM2S
        MASK_POLL_32     0x06009328, 0x78003c, 0x0   ; col3 MM2S
        MASK_POLL_32     0x08009328, 0x78003c, 0x0   ; col4 MM2S
        MASK_POLL_32     0x0A009328, 0x78003c, 0x0   ; col5 MM2S
        MASK_POLL_32     0x0C009328, 0x78003c, 0x0   ; col6 MM2S
        MASK_POLL_32     0x0E009328, 0x78003c, 0x0   ; col7 MM2S
        MASK_POLL_32     0x10009328, 0x78003c, 0x0   ; col8 MM2S
        MASK_POLL_32     0x12009328, 0x78003c, 0x0   ; col9 MM2S
        MASK_POLL_32     0x14009328, 0x78003c, 0x0   ; col10 MM2S
        MASK_POLL_32     0x16009328, 0x78003c, 0x0   ; col11 MM2S
        MASK_POLL_32     0x18009328, 0x78003c, 0x0   ; col12 MM2S
        MASK_POLL_32     0x1A009328, 0x78003c, 0x0   ; col13 MM2S
        MASK_POLL_32     0x1C009328, 0x78003c, 0x0   ; col14 MM2S
        MASK_POLL_32     0x1E009328, 0x78003c, 0x0   ; col15 MM2S
        MASK_POLL_32     0x20009328, 0x78003c, 0x0   ; col16 MM2S
        MASK_POLL_32     0x22009328, 0x78003c, 0x0   ; col17 MM2S
        MASK_POLL_32     0x24009328, 0x78003c, 0x0   ; col18 MM2S
        MASK_POLL_32     0x26009328, 0x78003c, 0x0   ; col19 MM2S
        MASK_POLL_32     0x28009328, 0x78003c, 0x0   ; col20 MM2S
        MASK_POLL_32     0x2A009328, 0x78003c, 0x0   ; col21 MM2S
        MASK_POLL_32     0x2C009328, 0x78003c, 0x0   ; col22 MM2S
        MASK_POLL_32     0x2E009328, 0x78003c, 0x0   ; col23 MM2S

        ; 10. Wait all 24 SHIM S2MM ch0 done (DDR write side)
        MASK_POLL_32     0x00009320, 0x78003c, 0x0   ; col0 S2MM
        MASK_POLL_32     0x02009320, 0x78003c, 0x0   ; col1 S2MM
        MASK_POLL_32     0x04009320, 0x78003c, 0x0   ; col2 S2MM
        MASK_POLL_32     0x06009320, 0x78003c, 0x0   ; col3 S2MM
        MASK_POLL_32     0x08009320, 0x78003c, 0x0   ; col4 S2MM
        MASK_POLL_32     0x0A009320, 0x78003c, 0x0   ; col5 S2MM
        MASK_POLL_32     0x0C009320, 0x78003c, 0x0   ; col6 S2MM
        MASK_POLL_32     0x0E009320, 0x78003c, 0x0   ; col7 S2MM
        MASK_POLL_32     0x10009320, 0x78003c, 0x0   ; col8 S2MM
        MASK_POLL_32     0x12009320, 0x78003c, 0x0   ; col9 S2MM
        MASK_POLL_32     0x14009320, 0x78003c, 0x0   ; col10 S2MM
        MASK_POLL_32     0x16009320, 0x78003c, 0x0   ; col11 S2MM
        MASK_POLL_32     0x18009320, 0x78003c, 0x0   ; col12 S2MM
        MASK_POLL_32     0x1A009320, 0x78003c, 0x0   ; col13 S2MM
        MASK_POLL_32     0x1C009320, 0x78003c, 0x0   ; col14 S2MM
        MASK_POLL_32     0x1E009320, 0x78003c, 0x0   ; col15 S2MM
        MASK_POLL_32     0x20009320, 0x78003c, 0x0   ; col16 S2MM
        MASK_POLL_32     0x22009320, 0x78003c, 0x0   ; col17 S2MM
        MASK_POLL_32     0x24009320, 0x78003c, 0x0   ; col18 S2MM
        MASK_POLL_32     0x26009320, 0x78003c, 0x0   ; col19 S2MM
        MASK_POLL_32     0x28009320, 0x78003c, 0x0   ; col20 S2MM
        MASK_POLL_32     0x2A009320, 0x78003c, 0x0   ; col21 S2MM
        MASK_POLL_32     0x2C009320, 0x78003c, 0x0   ; col22 S2MM
        MASK_POLL_32     0x2E009320, 0x78003c, 0x0   ; col23 S2MM

        SAVE_TIMESTAMPS    2

        ; 11. Wait all 24 MEM S2MM ch0 locks (lock0 == 16 -> 16 x 256KB = 4MB)
        MASK_POLL_32     0x001C0000, 0x0000003F, 0x00000010   ; col0 lock0
        MASK_POLL_32     0x021C0000, 0x0000003F, 0x00000010   ; col1 lock0
        MASK_POLL_32     0x041C0000, 0x0000003F, 0x00000010   ; col2 lock0
        MASK_POLL_32     0x061C0000, 0x0000003F, 0x00000010   ; col3 lock0
        MASK_POLL_32     0x081C0000, 0x0000003F, 0x00000010   ; col4 lock0
        MASK_POLL_32     0x0A1C0000, 0x0000003F, 0x00000010   ; col5 lock0
        MASK_POLL_32     0x0C1C0000, 0x0000003F, 0x00000010   ; col6 lock0
        MASK_POLL_32     0x0E1C0000, 0x0000003F, 0x00000010   ; col7 lock0
        MASK_POLL_32     0x101C0000, 0x0000003F, 0x00000010   ; col8 lock0
        MASK_POLL_32     0x121C0000, 0x0000003F, 0x00000010   ; col9 lock0
        MASK_POLL_32     0x141C0000, 0x0000003F, 0x00000010   ; col10 lock0
        MASK_POLL_32     0x161C0000, 0x0000003F, 0x00000010   ; col11 lock0
        MASK_POLL_32     0x181C0000, 0x0000003F, 0x00000010   ; col12 lock0
        MASK_POLL_32     0x1A1C0000, 0x0000003F, 0x00000010   ; col13 lock0
        MASK_POLL_32     0x1C1C0000, 0x0000003F, 0x00000010   ; col14 lock0
        MASK_POLL_32     0x1E1C0000, 0x0000003F, 0x00000010   ; col15 lock0
        MASK_POLL_32     0x201C0000, 0x0000003F, 0x00000010   ; col16 lock0
        MASK_POLL_32     0x221C0000, 0x0000003F, 0x00000010   ; col17 lock0
        MASK_POLL_32     0x241C0000, 0x0000003F, 0x00000010   ; col18 lock0
        MASK_POLL_32     0x261C0000, 0x0000003F, 0x00000010   ; col19 lock0
        MASK_POLL_32     0x281C0000, 0x0000003F, 0x00000010   ; col20 lock0
        MASK_POLL_32     0x2A1C0000, 0x0000003F, 0x00000010   ; col21 lock0
        MASK_POLL_32     0x2C1C0000, 0x0000003F, 0x00000010   ; col22 lock0
        MASK_POLL_32     0x2E1C0000, 0x0000003F, 0x00000010   ; col23 lock0
        ; 12. Wait the 4 PLIO MEM S2MM ch1 done (DMA_S2MM_Status_1 Status[1:0]=00 IDLE)
        MASK_POLL_32     0x0E1A0664, 0x00000003, 0x00000000   ; col7 S2MM ch1 idle
        MASK_POLL_32     0x161A0664, 0x00000003, 0x00000000   ; col11 S2MM ch1 idle
        MASK_POLL_32     0x261A0664, 0x00000003, 0x00000000   ; col19 S2MM ch1 idle
        MASK_POLL_32     0x2E1A0664, 0x00000003, 0x00000000   ; col23 S2MM ch1 idle

        SAVE_TIMESTAMPS    3

END_JOB

.eop
EOF

;data

.align    4
wts_params_c7:
.long    0x00000000   ; offset = 0
.long    0x00000000
.long    0x00600000   ; length = 6291456 words (24 MB)
.long    0x00000000
.long    0x00000000   ; stride = 0
.long    0x00000000
.long    0x00000001   ; iterations = 1
.long    0x00000000
.long    0x00000000   ; src addr[31:0]  — patched by APPLY_OFFSET_PL
.long    0x00000000   ; src addr[63:32] — patched by APPLY_OFFSET_PL
.align    4
wts_params_c11:
.long    0x00000000   ; offset = 0
.long    0x00000000
.long    0x00600000   ; length = 6291456 words (24 MB)
.long    0x00000000
.long    0x00000000   ; stride = 0
.long    0x00000000
.long    0x00000001   ; iterations = 1
.long    0x00000000
.long    0x00000000   ; src addr[31:0]  — patched by APPLY_OFFSET_PL
.long    0x00000000   ; src addr[63:32] — patched by APPLY_OFFSET_PL
.align    4
wts_params_c19:
.long    0x00000000   ; offset = 0
.long    0x00000000
.long    0x00600000   ; length = 6291456 words (24 MB)
.long    0x00000000
.long    0x00000000   ; stride = 0
.long    0x00000000
.long    0x00000001   ; iterations = 1
.long    0x00000000
.long    0x00000000   ; src addr[31:0]  — patched by APPLY_OFFSET_PL
.long    0x00000000   ; src addr[63:32] — patched by APPLY_OFFSET_PL
.align    4
wts_params_c23:
.long    0x00000000   ; offset = 0
.long    0x00000000
.long    0x00600000   ; length = 6291456 words (24 MB)
.long    0x00000000
.long    0x00000000   ; stride = 0
.long    0x00000000
.long    0x00000001   ; iterations = 1
.long    0x00000000
.long    0x00000000   ; src addr[31:0]  — patched by APPLY_OFFSET_PL
.long    0x00000000   ; src addr[63:32] — patched by APPLY_OFFSET_PL

val_ap_start:
.long    0x00000001

.align    16
wts_pl_cfg_chain_c7:
UC_DMA_BD    0x800,   0x00030010,   @wts_params_c7,   10,  1,  0
.align    16
wts_pl_cfg_chain_c11:
UC_DMA_BD    0x800,   0x00000010,   @wts_params_c11,   10,  1,  0
.align    16
wts_pl_cfg_chain_c19:
UC_DMA_BD    0x800,   0x00010010,   @wts_params_c19,   10,  1,  0
.align    16
wts_pl_cfg_chain_c23:
UC_DMA_BD    0x800,   0x00020010,   @wts_params_c23,   10,  1,  0
.align    16
wts_pl_start_chain_c7:
UC_DMA_BD    0x800,   0x00030000,   @val_ap_start,   1,  1,  0
.align    16
wts_pl_start_chain_c11:
UC_DMA_BD    0x800,   0x00000000,   @val_ap_start,   1,  1,  0
.align    16
wts_pl_start_chain_c19:
UC_DMA_BD    0x800,   0x00010000,   @val_ap_start,   1,  1,  0
.align    16
wts_pl_start_chain_c23:
UC_DMA_BD    0x800,   0x00020000,   @val_ap_start,   1,  1,  0

; ── SHIM NoC DMA BDs — MM2S uses BD1, S2MM uses BD0 (no overlap) ─────────────
; W0 Buffer_Length is a 32-bit WORD count: 4 MB / 4 = 0x00100000.
.align    4
shim_mm2s_bd_c0:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 0)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c1:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 1)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c2:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 2)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c3:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 3)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c4:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 4)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c5:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 5)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c6:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 6)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c7:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 7)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c8:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 8)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c9:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 9)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c10:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 10)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c11:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 11)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c12:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 12)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c13:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 13)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c14:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 14)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c15:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 15)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c16:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 16)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c17:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 17)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c18:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 18)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c19:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 19)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c20:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 20)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c21:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 21)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c22:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 22)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_mm2s_bd_c23:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 src addr lo — patched (arg 23)
.long    0x00000000   ; W2 src addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c0:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 24)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c1:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 25)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c2:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 26)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c3:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 27)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c4:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 28)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c5:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 29)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c6:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 30)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c7:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 31)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c8:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 32)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c9:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 33)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c10:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 34)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c11:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 35)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c12:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 36)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c13:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 37)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c14:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 38)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c15:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 39)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c16:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 40)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c17:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 41)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c18:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 42)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c19:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 43)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c20:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 44)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c21:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 45)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c22:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 46)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8
.align    4
shim_s2mm_bd_c23:
.long    0x00100000   ; W0 Buffer_Length = 4 MB
.long    0x00000000   ; W1 dst addr lo — patched (arg 47)
.long    0x00000000   ; W2 dst addr hi
.long    0x00000000   ; W3
.long    0xC0000000   ; W4 Burst_Length=512B
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x02000000   ; W7 Valid_BD
.long    0x00000000   ; W8

shim_mm2s_start_val:
.long    0x00000001   ; MM2S ch0 Start_BD_ID = 1
shim_s2mm_start_val:
.long    0x00000000   ; S2MM ch0 Start_BD_ID = 0

.align    4
mem_lock0_init:
.long    0x00000000

; MEM S2MM ch0 BD0 — 256 KB @ word 0x20000, releases lock0 +1 each pass
.align    4
mem_s2mm0_bd:
.long    0x00010000   ; W0 65536 words (256 KB)
.long    0x00020000   ; W1 Use_Next_BD=0, Base=0x20000 (LOCAL window)
.long    0x00000000   ; W2
.long    0x00000000   ; W3
.long    0x00000000   ; W4
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x81400000   ; W7 Valid | Lock_Rel +1 | Lock_Rel_ID=64 (local lock0)

; MEM MM2S ch0 BD2 — 128 KB @ word 0x30000, no lock (SHIM S2MM status gates it)
.align    4
mem_mm2s0_bd:
.long    0x00008000   ; W0 32768 words (128 KB)
.long    0x00030000   ; W1 Use_Next_BD=0, Base=0x30000
.long    0x00000000   ; W2
.long    0x00000000   ; W3
.long    0x00000000   ; W4
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x80000000   ; W7 Valid_BD only

; MEM S2MM ch1 BD24 — 128 KB @ word 0x38000, no lock (PLIO cols only)
.align    4
mem_s2mm1_bd:
.long    0x00008000   ; W0 32768 words (128 KB)
.long    0x00038000   ; W1 Use_Next_BD=0, Base=0x38000
.long    0x00000000   ; W2
.long    0x00000000   ; W3
.long    0x00000000   ; W4
.long    0x00000000   ; W5
.long    0x00000000   ; W6
.long    0x80000000   ; W7 Valid_BD only

mem_s2mm0_start16:
.long    0x000F0000   ; Start_BD=0,  Repeat=15  (16 x 256KB = 4 MB)
mem_mm2s0_start32:
.long    0x001F0002   ; Start_BD=2,  Repeat=31  (32 x 128KB = 4 MB)
mem_s2mm1_start192:
.long    0x00BF0018   ; Start_BD=24, Repeat=191 (192 x 128KB = 24 MB)

.align    16
cfg_chain_c0:
UC_DMA_BD    0,   0x001C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x001A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x001A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x00009000,   @shim_s2mm_bd_c0,   9,  0,  1
UC_DMA_BD    0,   0x00009030,   @shim_mm2s_bd_c0,   9,  0,  0
.align    16
sink_start_chain_c0:
UC_DMA_BD    0,   0x001A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x00009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c0:
UC_DMA_BD    0,   0x00009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x001A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c1:
UC_DMA_BD    0,   0x021C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x021A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x021A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x02009000,   @shim_s2mm_bd_c1,   9,  0,  1
UC_DMA_BD    0,   0x02009030,   @shim_mm2s_bd_c1,   9,  0,  0
.align    16
sink_start_chain_c1:
UC_DMA_BD    0,   0x021A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x02009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c1:
UC_DMA_BD    0,   0x02009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x021A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c2:
UC_DMA_BD    0,   0x041C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x041A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x041A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x04009000,   @shim_s2mm_bd_c2,   9,  0,  1
UC_DMA_BD    0,   0x04009030,   @shim_mm2s_bd_c2,   9,  0,  0
.align    16
sink_start_chain_c2:
UC_DMA_BD    0,   0x041A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x04009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c2:
UC_DMA_BD    0,   0x04009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x041A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c3:
UC_DMA_BD    0,   0x061C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x061A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x061A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x06009000,   @shim_s2mm_bd_c3,   9,  0,  1
UC_DMA_BD    0,   0x06009030,   @shim_mm2s_bd_c3,   9,  0,  0
.align    16
sink_start_chain_c3:
UC_DMA_BD    0,   0x061A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x06009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c3:
UC_DMA_BD    0,   0x06009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x061A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c4:
UC_DMA_BD    0,   0x081C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x081A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x081A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x08009000,   @shim_s2mm_bd_c4,   9,  0,  1
UC_DMA_BD    0,   0x08009030,   @shim_mm2s_bd_c4,   9,  0,  0
.align    16
sink_start_chain_c4:
UC_DMA_BD    0,   0x081A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x08009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c4:
UC_DMA_BD    0,   0x08009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x081A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c5:
UC_DMA_BD    0,   0x0A1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x0A1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0A1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0A009000,   @shim_s2mm_bd_c5,   9,  0,  1
UC_DMA_BD    0,   0x0A009030,   @shim_mm2s_bd_c5,   9,  0,  0
.align    16
sink_start_chain_c5:
UC_DMA_BD    0,   0x0A1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x0A009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c5:
UC_DMA_BD    0,   0x0A009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x0A1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c6:
UC_DMA_BD    0,   0x0C1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x0C1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0C1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0C009000,   @shim_s2mm_bd_c6,   9,  0,  1
UC_DMA_BD    0,   0x0C009030,   @shim_mm2s_bd_c6,   9,  0,  0
.align    16
sink_start_chain_c6:
UC_DMA_BD    0,   0x0C1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x0C009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c6:
UC_DMA_BD    0,   0x0C009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x0C1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c7:
UC_DMA_BD    0,   0x0E1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x0E1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0E1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x0E1A0300,   @mem_s2mm1_bd,   8,  0,  1
UC_DMA_BD    0,   0x0E009000,   @shim_s2mm_bd_c7,   9,  0,  1
UC_DMA_BD    0,   0x0E009030,   @shim_mm2s_bd_c7,   9,  0,  0
.align    16
sink_start_chain_c7:
UC_DMA_BD    0,   0x0E1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x0E1A060C,   @mem_s2mm1_start192,   1,  0,  1
UC_DMA_BD    0,   0x0E009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c7:
UC_DMA_BD    0,   0x0E009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x0E1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c8:
UC_DMA_BD    0,   0x101C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x101A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x101A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x10009000,   @shim_s2mm_bd_c8,   9,  0,  1
UC_DMA_BD    0,   0x10009030,   @shim_mm2s_bd_c8,   9,  0,  0
.align    16
sink_start_chain_c8:
UC_DMA_BD    0,   0x101A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x10009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c8:
UC_DMA_BD    0,   0x10009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x101A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c9:
UC_DMA_BD    0,   0x121C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x121A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x121A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x12009000,   @shim_s2mm_bd_c9,   9,  0,  1
UC_DMA_BD    0,   0x12009030,   @shim_mm2s_bd_c9,   9,  0,  0
.align    16
sink_start_chain_c9:
UC_DMA_BD    0,   0x121A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x12009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c9:
UC_DMA_BD    0,   0x12009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x121A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c10:
UC_DMA_BD    0,   0x141C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x141A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x141A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x14009000,   @shim_s2mm_bd_c10,   9,  0,  1
UC_DMA_BD    0,   0x14009030,   @shim_mm2s_bd_c10,   9,  0,  0
.align    16
sink_start_chain_c10:
UC_DMA_BD    0,   0x141A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x14009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c10:
UC_DMA_BD    0,   0x14009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x141A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c11:
UC_DMA_BD    0,   0x161C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x161A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x161A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x161A0300,   @mem_s2mm1_bd,   8,  0,  1
UC_DMA_BD    0,   0x16009000,   @shim_s2mm_bd_c11,   9,  0,  1
UC_DMA_BD    0,   0x16009030,   @shim_mm2s_bd_c11,   9,  0,  0
.align    16
sink_start_chain_c11:
UC_DMA_BD    0,   0x161A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x161A060C,   @mem_s2mm1_start192,   1,  0,  1
UC_DMA_BD    0,   0x16009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c11:
UC_DMA_BD    0,   0x16009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x161A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c12:
UC_DMA_BD    0,   0x181C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x181A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x181A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x18009000,   @shim_s2mm_bd_c12,   9,  0,  1
UC_DMA_BD    0,   0x18009030,   @shim_mm2s_bd_c12,   9,  0,  0
.align    16
sink_start_chain_c12:
UC_DMA_BD    0,   0x181A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x18009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c12:
UC_DMA_BD    0,   0x18009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x181A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c13:
UC_DMA_BD    0,   0x1A1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x1A1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1A1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1A009000,   @shim_s2mm_bd_c13,   9,  0,  1
UC_DMA_BD    0,   0x1A009030,   @shim_mm2s_bd_c13,   9,  0,  0
.align    16
sink_start_chain_c13:
UC_DMA_BD    0,   0x1A1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x1A009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c13:
UC_DMA_BD    0,   0x1A009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x1A1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c14:
UC_DMA_BD    0,   0x1C1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x1C1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1C1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1C009000,   @shim_s2mm_bd_c14,   9,  0,  1
UC_DMA_BD    0,   0x1C009030,   @shim_mm2s_bd_c14,   9,  0,  0
.align    16
sink_start_chain_c14:
UC_DMA_BD    0,   0x1C1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x1C009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c14:
UC_DMA_BD    0,   0x1C009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x1C1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c15:
UC_DMA_BD    0,   0x1E1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x1E1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1E1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x1E009000,   @shim_s2mm_bd_c15,   9,  0,  1
UC_DMA_BD    0,   0x1E009030,   @shim_mm2s_bd_c15,   9,  0,  0
.align    16
sink_start_chain_c15:
UC_DMA_BD    0,   0x1E1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x1E009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c15:
UC_DMA_BD    0,   0x1E009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x1E1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c16:
UC_DMA_BD    0,   0x201C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x201A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x201A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x20009000,   @shim_s2mm_bd_c16,   9,  0,  1
UC_DMA_BD    0,   0x20009030,   @shim_mm2s_bd_c16,   9,  0,  0
.align    16
sink_start_chain_c16:
UC_DMA_BD    0,   0x201A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x20009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c16:
UC_DMA_BD    0,   0x20009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x201A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c17:
UC_DMA_BD    0,   0x221C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x221A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x221A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x22009000,   @shim_s2mm_bd_c17,   9,  0,  1
UC_DMA_BD    0,   0x22009030,   @shim_mm2s_bd_c17,   9,  0,  0
.align    16
sink_start_chain_c17:
UC_DMA_BD    0,   0x221A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x22009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c17:
UC_DMA_BD    0,   0x22009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x221A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c18:
UC_DMA_BD    0,   0x241C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x241A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x241A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x24009000,   @shim_s2mm_bd_c18,   9,  0,  1
UC_DMA_BD    0,   0x24009030,   @shim_mm2s_bd_c18,   9,  0,  0
.align    16
sink_start_chain_c18:
UC_DMA_BD    0,   0x241A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x24009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c18:
UC_DMA_BD    0,   0x24009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x241A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c19:
UC_DMA_BD    0,   0x261C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x261A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x261A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x261A0300,   @mem_s2mm1_bd,   8,  0,  1
UC_DMA_BD    0,   0x26009000,   @shim_s2mm_bd_c19,   9,  0,  1
UC_DMA_BD    0,   0x26009030,   @shim_mm2s_bd_c19,   9,  0,  0
.align    16
sink_start_chain_c19:
UC_DMA_BD    0,   0x261A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x261A060C,   @mem_s2mm1_start192,   1,  0,  1
UC_DMA_BD    0,   0x26009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c19:
UC_DMA_BD    0,   0x26009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x261A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c20:
UC_DMA_BD    0,   0x281C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x281A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x281A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x28009000,   @shim_s2mm_bd_c20,   9,  0,  1
UC_DMA_BD    0,   0x28009030,   @shim_mm2s_bd_c20,   9,  0,  0
.align    16
sink_start_chain_c20:
UC_DMA_BD    0,   0x281A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x28009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c20:
UC_DMA_BD    0,   0x28009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x281A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c21:
UC_DMA_BD    0,   0x2A1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x2A1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2A1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2A009000,   @shim_s2mm_bd_c21,   9,  0,  1
UC_DMA_BD    0,   0x2A009030,   @shim_mm2s_bd_c21,   9,  0,  0
.align    16
sink_start_chain_c21:
UC_DMA_BD    0,   0x2A1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x2A009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c21:
UC_DMA_BD    0,   0x2A009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x2A1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c22:
UC_DMA_BD    0,   0x2C1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x2C1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2C1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2C009000,   @shim_s2mm_bd_c22,   9,  0,  1
UC_DMA_BD    0,   0x2C009030,   @shim_mm2s_bd_c22,   9,  0,  0
.align    16
sink_start_chain_c22:
UC_DMA_BD    0,   0x2C1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x2C009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c22:
UC_DMA_BD    0,   0x2C009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x2C1A0634,   @mem_mm2s0_start32,   1,  0,  0
.align    16
cfg_chain_c23:
UC_DMA_BD    0,   0x2E1C0000,   @mem_lock0_init,   1,  0,  1
UC_DMA_BD    0,   0x2E1A0000,   @mem_s2mm0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2E1A0040,   @mem_mm2s0_bd,   8,  0,  1
UC_DMA_BD    0,   0x2E1A0300,   @mem_s2mm1_bd,   8,  0,  1
UC_DMA_BD    0,   0x2E009000,   @shim_s2mm_bd_c23,   9,  0,  1
UC_DMA_BD    0,   0x2E009030,   @shim_mm2s_bd_c23,   9,  0,  0
.align    16
sink_start_chain_c23:
UC_DMA_BD    0,   0x2E1A0604,   @mem_s2mm0_start16,   1,  0,  1
UC_DMA_BD    0,   0x2E1A060C,   @mem_s2mm1_start192,   1,  0,  1
UC_DMA_BD    0,   0x2E009304,   @shim_s2mm_start_val,   1,  0,  0
.align    16
src_start_chain_c23:
UC_DMA_BD    0,   0x2E009314,   @shim_mm2s_start_val,   1,  0,  1
UC_DMA_BD    0,   0x2E1A0634,   @mem_mm2s0_start32,   1,  0,  0
