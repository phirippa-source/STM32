 EXPORT __main
    AREA PROG, CODE, READONLY
__main
        MOV R1, #0x12     ; R1 = 0x25
        MOV R2, #0x34     ; R2 = 0x34
        ADD R0, R1, R2    ; R3 = R2 + R1
L1     	NOP
        MOV R5, #0x12AB
        MOV R4, #0x20000000
        STR R5, [R4]
L2     	NOP
        LDR R6, [R4]
L3     	NOP
HERE    B HERE             ; stay here forever
        END
