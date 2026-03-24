	EXPORT __main
	AREA PROG, CODE, READONLY
__main
	MOV		R0, #0
	MSR 	APSR, R0	;clear APSR
	MOVS    R2, #1   	; R1 = 1
	MOVS    R3, #-1  	; R2 = 0xFFFFFFFF
	ADDS    R1,R2,R3    ; R1 = R2+R3
	NOP
L4  B   	L4 			; stay here forever
    END
