; -----------------------------------------------------------------------------
; merged_control.asm — top-level control-code tree (single uC: uC0 = column 0).
;
;   Job 0 : SHIM + MEM stream-switch routing, all 24 cols (.include aie_asm_init.asm)
;   Job 1 : 24-col SHIM MM2S + SHIM S2MM + 4-col PLIO data flow + data
;           (.include aie_control.asm)
;
; All DMA control runs on uC0 (column 0) poking all 24 cols via UC_DMA_BD.
; -----------------------------------------------------------------------------
.attach_to_group 0

; ── Job 0: SHIM + MEM stream-switch routing for all 24 cols ───────────────────
START_JOB 0
        .include aie_asm_init.asm
END_JOB

.eop

; ── Job 1 (+ EOF + data): full-duplex SHIM DMA + PLIO data flow ──────────────
.include aie_control.asm
