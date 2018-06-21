;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer

　
;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------

	MOV.B #0FFh, &P1IE	;interrupt enable
	MOV.B #0FFh, &P1IES ;zbocze opadajace, te przyciski sa normalnie zamkniete

	MOV.B #0FFh, &P2IE	;interrupt enable
	MOV.B #0FFh, &P2IES ;zbocze opadajace

	MOV.B #0FFh, &P3DIR ;port 3 jako output na ekran

	MOV.W #00000h,R4	;licznik nkb
	MOV.W #00000h,R5	;gray
	MOV.W #00000h,R6	;debouncing counter
	MOV.W #00000h,R7	;max wartosc zliczania
	
	MOV.B P4IN, R7		;ladowanie maksymalnej wartosci
	INC.B R7

	MOV.B R5, &P3OUT	;jednorazowy display,zeby 00 sie ustawilo

SLEEP:
	BIS #GIE+CPUOFF,SR	;lpm
MAIN:
	CMP.B R7,R4			;sprawdzenie czy już max
	JNZ DISPLAY			
	MOV.B #000h,R4		;jesli tak to zerowanie licznika nkb
DISPLAY:
	MOV.B R4,R5			;konwersja na gray
	;RRA.B R5
	;XOR R4,R5
	MOV.B R5, &P3OUT	;display

	DINT
	CMP.B #000h,R6		;sprawdzenie czy cos drga, jak 0 to nie i sleep
	JZ SLEEP
	CMP.B #001h,R6		;sprawdzenie czy cos skonczylo drgac, jak nie to decrement countera
	JNZ DECREMENT

	CMP.B #000h,P2IES	;sprawdzenie czy przycisk jest wcisniety, jesli nie to decrement
	JNZ DECREMENT		;jesli jest puszczony to decrement
	INC.B R4			;jesli jest wcisniety to increment nkb counter

DECREMENT:
	DEC.B R6			;jesli jest inny counter od 0 to dekrementuj
	EINT
	JMP MAIN


P1_INTERRUPT:			;CLR asynchroniczny
	INV.B P1IES			;zmiana zbocza wyzwalania na przeciwne
	MOV.B #000h, &P1IFG	;clear flagi przerwania
	
	MOV.B #000h, R4		;zerowanie nkb counter
	MOV.B #000h, R5		;zerowanie gray counter
	MOV.B #000h, R6		;zerowanie debouncing counter
	
	BIC #CPUOFF,0(SP)	;edycja SR na stacku

	MOV.B P4IN, R7		;aktualizacja max wartosci zliczania
	INC.B R7
	
	INV.B P2IE			;wylaczenie/wlaczenie przerwan od clk
	MOV.B #000h, &P2IFG	;clear flagi P2 zeby sie nie inkrementowal po puszczeniu zerowania
	RETI

　
P2_INTERRUPT:			;CLK
	INV.B P2IES 		;odwrocenie zbocza
	MOV.B #000h, &P2IFG	;clear flagi
	MOV.B #0FFh, R6 	;zaladowanie do debouncu
	BIC #CPUOFF,0(SP)	;edycja SR na stacku
	RETI
                                            

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            

            .sect ".int04"
            .short P1_INTERRUPT

            .sect ".int01"
            .short P2_INTERRUPT

　