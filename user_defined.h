#ifndef _USER_DEFINED_H_
#define _USER_DEFINED_H_
	#define LOW 0
	#define HIGH -1
	// --- Range padrão de valores de entradas do ppm ---
#define QUARTER_PPM_RANGE 2000
//ton mínimo do ppm: (1ms * freq. do clock)
#define MIN_PPM_VAL 8000
//ton máximo do ppm: (2ms * freq. do clock)
#define MAX_PPM_VAL 16000
// Valor médio do cana
#define MEAN_PPM_VAL 12000
//Fazer canal ppm com valor simétrico: (-12000)
#define HALF_PPM_RANGE 4000

// ===============================================================
// --- Declaração de funções definidas pelo assembler

/*
 * div() em mult() são usadas em conjunto para realizar operações
 * concatenadas, quando  a = dividendo/divisor < 1.
 */

/*
 * Multiplica os valores 'a' e 'b' e retona a word mais
 * significativa, isto é, divide por 2^16.
 */
int mult(int a, int b);

/*
 * Realiza divisão inteira e retona a parte decimal na word
 * menos significativa, isto é, multiplica por 2^16.
 */
int div(int dividendo, int divisor);

/*
 * Retorna o valor absoluto da entrada. Foi tentado usar o abs()
 * da biblioteca math.h, mas não deu muito certo. Não se sabe o
 * motivo.
 */
int ab(int x);

// ===============================================================
// --- Declaração de funções definidas no source file ---

/*
 * flash_write_word. Escreve uma word (16 bits) no endereço apontado
 * por Data_ptr.
 */
void flash_ww(int *Data_ptr, int word);

/*
 * Retorna HIGH se x > y, senão retorna LOW. x e y são vistos como
 * valores sem sinal. Criar este procedimento foi necessário, pois,
 * para o microcontrolador, não importa se o valor é unsigned ou não…    
 * se tiver o bit mais significativo (MSB) em nível alto, então o
 * valor é considerado negativo. Daí esta função é uma forma de
 * contornar este problema.
 */
int comparar(int x, int y);
	

	
#endif
