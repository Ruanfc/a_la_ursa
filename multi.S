#include <msp430g2553.h>
#define PC R0
#define SP R1
#define SR R2
;**********************************************************	
;Algoritmo de tirar o valor absoluto 
;recebe R12
;return R12
;**********************************************************
		.global ab
		.section ".text"
ab:	.func
	TST R12
	JN neg
end:	RET
neg:	INV R12
	INC R12
	JMP end
	.endfunc

;**********************************************************	
; Algoritmo para escalonar um inteiro de 0 a 4000 em 0 a 1333
; temp32_0 será R12.R13
; temp32_1 será R14.R15
; temp32_2 será R11.R10
; entrada e saída em R12
;**********************************************************
		.global map
		.section ".text"
map:	.func
		PUSH R10
		PUSH R11
		PUSH R14
		PUSH R13
		PUSH R12
		;MOV	R15,R12;<<16;CLR R13;CLR R13;CLR R14...
		MOV	R12,R14;							
		MOV	R13,R15; parse temp32_0	to temp32_1			
		RRA	R14;								
		RRC	R15;								
		RRA	R14;								
		RRC	R15;								
		RRA	R14;								
		RRC	R15;								
		RRA	R14;								
		RRC	R15;	>>4							
		INV	R14;								
		INV	R15;								
		INC	R15;								
		JNC	carry; Se tiver carry de uma palavra pra outra
		INC	R14;	::::troca	de	sinal					
carry:	ADD	R13,	R15;							
		ADDC R12, R14; Soma temp32_0 a temp32_1
		MOV	R14, R11;							
		MOV	R15, R10; parse temp32_1 to temp32_2			
		RRA	R11;								
		RRC	R10;								
		RRA	R11;								
		RRC	R10; >> 2						
		ADD	R12, R11;							
		ADDC R13,R10; Soma temp32_0 a temp32_2							
		MOV	R11, R14;
		MOV	R10, R15; parse temp32_2 to temp32_1			
		RRA	R14;
		RRC	R11;
		RRA	R14;
		RRC	R11; >>	2
		ADD	R12, R14;
		ADDC R13, R15; Soma temp32_0 a temp32_1			
		MOV	R14,R11;
		MOV	R15,R10; parse temp32_1 to temp32_2			
		RRA	R11;
		RRC	R10;
		RRA	R11;
		RRC	R10; >>	2
		ADD	R12, R11;
		ADDC R13, R10; Soma temp32_0 a temp32_2			
		MOV	R11,R14;							
		MOV	R10,R15; parset temp32_2 to temp32_1
		RRA	R14;								
		RRC	R15;
		RRA	R14;
		RRC	R15; >>2
		ADD	R12,R14;
		ADDC R13,R15; Soma temp32_0 a temp32_1
		MOV	R14,R11;
		MOV	R15,R10; parse temp32_1 to temp32_2
		RRA	R11;
		RRC	R10;
		RRA	R11;
		RRC	R10; >>2
		ADD	R12,R11;
		ADDC R13,R10; Soma temp32_0 a temp32_2			
		RRA	R11;
		RRC	R10;
		RRA	R11;
		RRC	R10; >>2
		MOV	R11, R12;
		POP R10
		POP R11
		POP R14
		POP R13
		RET
		.endfunc
		
;**********************************************************	
; Algoritmo de multiplicação com divisão por 2^16
; extern int mult(int a, int b);
; a = R12; b = R13; a*b = res
; 'a' e 'res' precisam de uma word mais significativa
; efetuar a operação => a = R15.R12; res = R11.R14
;**********************************************************
		.global mult
		.section ".text"
mult:	.func
		PUSH R14
		PUSH R15
		PUSH R11
		CLR R14
		CLR R15
		CLR R11			; Limpa o lixo
loop:	CLRC			; Limpa carry
		RRC R13			; Testa o LSB de 'b'
		JNC unset		; Se for alto...
		ADD R12,R14		; então soma 'b' a res
		ADDC R15,R11	; Senão, deixa deixa de somar
unset:	RLA R12			; Prepara para a próxima soma
		RLC R15
		TST R13			; Se ainda tiver o que multiplicar...
		JNZ loop		; Repete o procedimento
		;MOV R14, R13
		;MOV R11, R12
		MOV R11,R12		; retorna o produto dividido por 2^16
		POP R11
		POP R15
		POP R14
		RET
		.endfunc

;**********************************************************	
; Algoritmo para realizar divisão especial
; Obs: chamei de divisão especial justamente porque ela
; multiplica o resultado por 2^16 para poder usar a parte
; fracionária como se fosse um número inteiro
; Recebe [R12] o dividendo e o divisor [R13]. Multiplica
; por 2^16 por contar o procedimento 32 vezes ao invés de 16.
; Usa [R14] como contador. Conta de 32 até 0.
; Usa [R15] como quociente da divisão e usa [R11] como
; o resto.
; Resumo:
; Recebe [R12] e o torna o dividendo;
; Recebe [R13] como divisor;
; Retorna [R12] como quociente;
; Declaração em C:
; extern int div(int dividendo, int divisor)
;**********************************************************
		.global div
		.section ".text"
div:	.func 
		PUSH R10
		PUSH R11
		PUSH R15
		PUSH R14
		MOV #0x20,R14	; Inicializa contador
		CLR R15			; Limpa quociente
		CLR R11			; Limpa resto
iter:	RLA R12			; Empurra bit para resto
		RLC R11			; Puxa bit do dividendo
		RLA R15			; Dobra quociente
		CMP R13,R11		; Verifica se é divisível
		JL	maior		; Se não, pula
		INC R15			; Set LSB do quociente
		SUB R13,R11		; Subtrai quoc. ao resto
maior:	DEC R14			; Atualiza contador
		;JC iter			; Finaliza iteração
		JNZ iter			; Finaliza iteração
		MOV R15,R12		; retorna quociente
		;MOV R11,R13	; retorna resto
		POP R14
		POP R15
		POP R11
		POP R10
		RET
		.endfunc
		
		.end
