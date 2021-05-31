#include "user_defined.h"
#include <msp430g2553.h>
//#include "multi.o"

// ===============================================================
// --- Declaração de funções definidas pelo assembler ---
int mult(int a, int b);
int ab(int x);
int div(int dividendo, int divisor);
// ===============================================================
// --- Declaração de funções definidas no source file ---
void flash_ww(int *Data_ptr, int word);
int comparar(int x, int y);


void flash_ww(int *Data_ptr, int word)
{
	FCTL3 = 0x0a500; // LOCK = 0
	FCTL1 = 0X0a540; // WRT = 1
	*Data_ptr = word; // program flash word
	FCTL1 = 0x0A500; // WRT = 0;
	FCTL3 = 0x0A510; //Lock = 1
}

int comparar(int x, int y)
{
		int boolean = LOW;
		if ((x ^ y) < 0)
		{if (x < 0) boolean = HIGH;}
		else if (x > y) boolean = HIGH;
		return boolean;
}
