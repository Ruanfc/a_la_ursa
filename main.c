/**********************************************************
 * MAPEAMENTO MSP430G2553
 *
 * P1.0 - FLIP
 * P1.1 - Canal avanço-recuo
 * P1.2 - Canal direita-esquerda
 * P1.4 - Indicação de bateria (Led vermelho)
 * P1.5 - Funcionamento normal (led verde)
 * P1.6 - Leitura de bateria inferior
 * P1.7 - Leitura de bateria superior
 * P2.0 - Recepção de sinal
 * P2.1 - Motor esquerdo recuo
 * P2.2 - Motor esquerdo avanço
 * P2.3 - Botão de calibração
 * P2.4 - Motor direito recuo
 * P2.5 - Motor direito avanço
 *
 *                           MSP430G2553
 *                         ---------------
 *                    VDD--| 1        20 |--GND
 *            P1.0  flip --| 2        19 |--        P2.6
 *            P1.1  IN_AR--| 3        18 |--        P2.7
 *            P1.2  IN_DE--| 4        17 |--TEST
 *            P1.3  	 --| 5        16 |--RST
 *            P1.4  LED_R--| 6        15 |--NBATS	P1.7
 *            P1.5  LED_G--| 7        14 |--NBATI   P1.6
 *            P2.0  LED_B--| 8        13 |--MDR     P2.5
 *            P2.1    MEA--| 9        12 |--MDA     P2.4
 *            P2.2    MER--| 10       11 |-- 	  	P2.3
 *                         ---------------
 **********************************************************/

#include <msp430g2553.h>
#include "user_defined.h"
// =================================================================
// --- Mapeamento de hardware ---
#define FLIP BIT0 		//P1.0
#define IN_AR BIT1
#define IN_DE BIT2
#define LED_R BIT4		//P1.4 -> Bateria
#define LED_G BIT5		//P1.5 -> Com sinal & calibrado & bateria ok 
#define NBATI BIT6 		//P1.6
#define NBATS BIT7 		//P1.7
#define LED_B BIT0 		//P2.0 -> Sem sinal
//#define CAL BIT3

//Não sei o porquê, mas essa tem que ser a ordem para o programa dar certo.
unsigned int MER = BIT2;
unsigned int MEA = BIT1;
unsigned int MDR = BIT5;
unsigned int MDA = BIT4;

#define INPUT_PINS (FLIP | IN_AR | IN_DE | NBATI | NBATS)
#define OUTPUT_PINS_1 ( LED_R | LED_G)						//port1
#define OUTPUT_PINS_2 ( LED_B | MER | MEA | MDA | MDR) 		//port2
#define MOTOR_PINS (MER | MEA | MDA | MDR)					//port1
#define PPM_PINS (IN_AR | IN_DE)

// =================================================================
// --- Configuração do watchdog timer ---
//Watchdog mode
#define watchdog_setup (WDTPW|WDTSSEL)  // 32768(prescale)/(freq. ACLK)
#define watchdog_hold (WDTPW | WDTHOLD)
#define watchdog_reset (WDTPW | WDTCNTCL)
//Interval mode; ACLK sourced by LFXT1CLK(LF) = 32768Hz; /8192 => ~250ms
//#define watchdog_hold (WDTPW | WDTHOLD)
//#define watchdog_setup (WDTPW | WDTTMSEL | WDTCNTCL | WDTSSEL | 3)//WDTIS1)

// =================================================================
// --- Configuração dos blocos de captura (TA0CCTLx) ---
#define CAPTURE_CONTROL_CONFIG CM_1 | SCS | CAP | CCIE 
/*
 * CM_1 // Captura em borda de subida 
 * SCS // Captura síncrona
 * CAP // Modo de captura ativado
 * CCIE; // Interrupção CCRx habilitada
 * CCIS_0; //Entrada de captura/comparação CCIxA
 */

// =================================================================
// --- Variáveis para pwm e ppm ---
#define totalTicks 1333 //ticks= freq_clock/freq_pwm= 8M/6k = 1333,3
int y=0, x=0;
volatile int yfinal=0, xfinal=0, yinit=0, xinit=0, interrupt_flags=0, signalCheck=3, signalLost = LOW;
int interrupt_aux = 0, iter=0;

int deltaTx = MEAN_PPM_VAL, deltaTy = MEAN_PPM_VAL;
int deltaTxmin = MIN_PPM_VAL, deltaTxmax = MAX_PPM_VAL;
int deltaTymin = MIN_PPM_VAL, deltaTymax = MAX_PPM_VAL;
int newDeltaTxmax=12000, newDeltaTymax=12000 ,newDeltaTymin=12000 ,newDeltaTxmin=12000, newDeltaTx=12000, newDeltaTy=12000;
int deltaTxmean = MEAN_PPM_VAL;
int deltaTymean = MEAN_PPM_VAL;

// =================================================================
// --- Variáveis para alerta de bateria ---
int blinks, blinkOff, batOK; //Variáveis para o blink
volatile int blinkCounter=0;
volatile unsigned int ADC_acc[] = {0,0}; //Acumula valor das 2 bat.
volatile unsigned int INCH_var = INCH_7;

// =================================================================
// --- Variáveis para o flip
#define flipThreshold 300 //400//649 //50 (tensão_de_limiar/(ref+))*1024
volatile int flipCounter=0;
int flipState = 0;
unsigned int* pwmLptr = (unsigned int*)&TA1CCR1;
unsigned int* pwmRptr = (unsigned int*)&TA1CCR2;

// =================================================================
// --- Calibração ---
int callibrated = LOW;
int* SEGD = (int*) 0x01000; //SEGD = Segment 63
int escala = 21840; //≃ (1333/4000)*2^16
//int escala = 10920; //≃ (1333/8000)*2^16
int callibrateCounter = 0; //Conta tempo para pode sair do proced.
        // Sem calibração provisoriamente
        int escalaX = 10920;
        int escalaY = 10920;
					
// ================================================================
// --- Range padrão de valores de entradas da bateria ---
//p = 11/(130+11) = 0,07801 //div. tensao
//Vcc_min/2 = 11,6
//adc ref+ = 2,5
//Nadc_min = p*(Vcc_min/2)*1023/2,5 = 320,29
#define MIN_CHARGE 320 // ≃ Nadc_min
//READY := 0,95*Nadc_max = 0,95*804,426 = 764,2047
#define READY 764 //2*12,6*0.95*(p)*(1023/2,5)

int arrayCounter = 0;
#define ARRAYMAX 8
int Xarray[ARRAYMAX];
int Yarray[ARRAYMAX];
void valoresToArray(int x, int y)
{
	Xarray[arrayCounter] = x;
	Yarray[arrayCounter] = y;
	if(++arrayCounter >= ARRAYMAX) arrayCounter = 0;
}
//  ===============================================================
// --- Captura do canal ED e atualização de pwm ---
void handler2(void)
{
	register int _y = ab(y);
	register int _x = ab(x);
	register int modulo = _y - _x;
	register int _y_x = ab(modulo);
	if (_y_x < (200)) _x = _y;
	if (x < 0) x = -_x;
	else x = _x;
	
	//Polarização de motores
	//Motor esquerdo
	if (y>=-x)	//Motor esquerdo para frente
	{ 
		P2SEL &= ~MER;
		P2OUT &= ~MER;
		P2SEL |= MEA;
	} 
	else		//Motor esquerdo para trás
	{
		P2SEL &= ~MEA;
		P2OUT &= ~MEA;
		P2SEL |= MER;
	}
	//Motor direito
	if (y>=x) //Motor direito para frente
	{
		P2SEL &= ~MDR;
		P2OUT &= ~MDR;
		P2SEL |= MDA;
	}
	else		//Motor direito para trás
	{
		P2SEL &= ~MDA;
		P2OUT &= ~MDA;
		P2SEL |= MDR;
	}
	
	//Processamento das variaveis
	register int valorL = y+x;
	register int valorR = y-x;
	if ((x^y) < 0) valorR += (modulo < 0) ? -y : x;
	else valorL -= (modulo < 0) ? y : x;
	
	// Escalona valores e envia para motores.
	      valorL = mult(escala,ab(valorL));
	*pwmLptr = valorL;
	      valorR = mult(escala,ab(valorR));
	*pwmRptr = valorR;
}//Fim handler2

// ================================================================
// --- Indicação de bateria ---
void handler4 (void)
{
	//blinks conta o número de vezes que o led deve piscar
	if (blinks)
	{
		if (P1OUT & LED_R) //Led vermelho aceso?
		{
			blinks--;
			P1OUT &= ~(LED_R);
			//Acende leds se for necessário.
			//Acredito que as duas próximas linhas são desnecessárias...
			//... bem como a variável signalLost. Isto se deve à
			//atualização frequente durante o estouro de timer
			P1OUT |= (!signalLost && callibrated)? LED_G :0;
			P2OUT |= signalLost ? LED_B : 0;
		}
		else //Se o led vermelho estiver apagado
		{	//Momento de blink
			P1OUT |= LED_R;
			//Talvez também sejam desnecessárias as próximas 2 linhas
			P1OUT &= ~LED_G;
			P2OUT &= ~LED_B;
		}
	}
	//blinfOff conta o tempo discretizado em que LED_R estará em OFF.
	else if (blinkOff--)
	{
		//Pede pro ADC ler bateria 
		ADC10CTL0 |= (ENC | ADC10SC);
	}
	else
	{
		register int bat_reg1 = ADC_acc[0]; //Valor da bateria 1
		register int bat_reg2 = ADC_acc[1];// - bat_reg1;
		blinkOff = 4;
		if (bat_reg1 <= MIN_CHARGE)
		{
			blinks ++; //BatI descarregada: 1 blink
		}
		if (bat_reg2 <= MIN_CHARGE)
		{
			blinks +=2;//BatS descarregada: 2 blinks
		}
		batOK = blinks ? LOW : HIGH;
		//Ambas as baterias descarregas: 3 blinks
		//Se ambas estiverem ok, então 0 blinks		
	}
} //Fim handler4

// ===============================================================
// --- FLIP ---
// Ainda precisa de ajustes. Pelo visto apenas trocar esta variáveis não
// dá certo.
void handler8()
{
	if (flipState)
	{
		pwmLptr = (unsigned int*)&TA1CCR2;
		pwmRptr = (unsigned int*)&TA1CCR1;
		MER = BIT2;
		MEA = BIT1;
		MDR = BIT5;
		MDA = BIT4;
	}
	else
	{
		pwmLptr = (unsigned int*)&TA1CCR1;
		pwmRptr = (unsigned int*)&TA1CCR2;
		MER = BIT4;
		MEA = BIT5;
		MDR = BIT1;
		MDA = BIT2;
	}
}
// ===============================================================
// --- Procedimento de calibração
void callibrate(int state)
{
	//State indica se o robô deve sobrer calibração
	if (state)
	{
		//Limpa segmento
		WDTCTL = watchdog_hold; //Disable watchdog
		FCTL2  = FWKEY | FSSEL1 | 17; //f_FTG= MCLK/17 < 476kHz
		FCTL3 = FWKEY; //LOCK = 0
		FCTL1 = FWKEY | ERASE; // enable erase
		*SEGD = 0;
		//BUSY e ERASE são automaticamente limpos ao fim do ciclo
		FCTL3 = FWKEY; //?????????????????
		
		//Joga valores antigos para trás;
		flash_ww(SEGD+2,*SEGD); 
		flash_ww(SEGD+3,*(SEGD+1));
		//Insere novos valores
		flash_ww(SEGD, newDeltaTx);
		flash_ww(SEGD+1, newDeltaTy);
	}

	//Organiza emordem crescente e determina os valores máximos e mínimos
	if (*SEGD > *(SEGD+2))
	{
		deltaTxmax = *SEGD;
		deltaTxmin = *(SEGD+2);
		
	}else
	{
		deltaTxmin = *SEGD;
		deltaTxmax = *(SEGD+2);
	}
	// repete para os valores no eixo y	
	if (*(SEGD+1) > *(SEGD+3))
	{
		deltaTxmax = *(SEGD+1);
		deltaTxmin = *(SEGD+3);
		
	}else
	{
		deltaTxmin = *(SEGD+3);
		deltaTxmax = *(SEGD+1);
	}
	
        //Determina escala de para os eixos x e y
	escalaX = div(totalTicks, deltaTxmax - deltaTxmin);
	escalaY = div(totalTicks, deltaTymax - deltaTymin);
	WDTCTL = watchdog_setup; //Reabilita wdt

	// Determina valores médios
	deltaTymean = ((deltaTymax + deltaTymin) >> 1)&(0x7fff);
	deltaTxmean = ((deltaTxmax + deltaTxmin) >> 1)&(0x7fff);
       
	
} //Fim callibrate
// ================================================================
// --- Código principal ---
int main(void)
{
	//Inicialização de variáveis
	deltaTymean = ((deltaTymax + deltaTymin) >> 1)&(0x7fff);
	deltaTxmean = ((deltaTxmax + deltaTxmin) >> 1)&(0x7fff);
	/*** Watchdog timer and clock Set-Up ***/
	WDTCTL = watchdog_setup;		// Configura cachorro
	//WDTCTL = watchdog_hold;
	//IE1 |= WDTIE; //Habilita interrupção do wdt

	// Leitura de calibração
	//callibrate(0);
	
	DCOCTL = 0;                     // Select lowest DCOx and MODx
	//BCSCTL1 = CALBC1_8MHZ;          // Set range
	//DCOCTL = CALDCO_8MHZ;           // Set DCO step + modulation
        BCSCTL1 = 13; //RSELx = 13; XT2OFF = 0
        DCOCTL = (4 << 5) | 10; // DCOx = 5; MODx = 0;
		// No controle pistola usar TRIM : CH1= N00 e CH2= B23; D/R: CH2 = 90%
	
	//Manipulação de ports entrada saída
	P1DIR &= ~INPUT_PINS;
	P1DIR |= OUTPUT_PINS_1; //LED_R e LED_G como OUTPUT
	P2DIR |= OUTPUT_PINS_2;
	
	
	//Configuração de port1: Pinos 1 e 2 configurados para captura
	P1REN |= PPM_PINS;	//Habilita resistores internos
	P1OUT &= ~PPM_PINS;	//Configura como pull down
	P1SEL |= PPM_PINS;	//Timer0_A3.CCI0A e Timer0_A3.CCI1A
	//P1OUT |= LED_G; 	//LED_G = HIGH e LED_B = LOW no início
	P1OUT &= ~LED_R;
	
	//Garante que os motores vão estar desabilitados
	P2SEL &= ~MOTOR_PINS;
	P2OUT &= ~(MOTOR_PINS | OUTPUT_PINS_2);
	
	/*
	//Teste e Interrupção do pinos 2.3
	P2DIR	&= ~CAL; 
	P2REN	|= CAL;
	P2OUT	&= ~CAL;
	P2IES	|= CAL;	//Borda de descida
	P2IE	|= CAL; 
	*/
	
	//Configuração do timer 0 - para port 1
	TA0CCTL0 |= CAPTURE_CONTROL_CONFIG;
	TA0CCTL1 |= CAPTURE_CONTROL_CONFIG;
	TA0CTL |= TASSEL_2 | MC_2 | TAIE; // SMCLK, Countinuous mode, TAIFG Enable

	//Configuração do Timer 1 - para port 2
	TA1CCTL1 = OUTMOD_7; // Saída em reset/set
	TA1CCTL2 = OUTMOD_7; // Saída em reset/set
	TA1CCR0 = totalTicks; //Registrador para comparação
	TA1CTL |= TASSEL_2 | MC_1; // SMCLK (Submain clock 8MHz) UPMODE 
	
	//Configuração do ADC10
	ADC10CTL0 =
		ADC10SHT_3 |	//Sample and Hold Time = 64*ADC10CLKs
		ADC10SR |		//Suports ~50ksps (SR = Slew Rate)
		ADC10ON |		//Mantêm o ADC10 em fucnionamento
		ADC10IE ;		//Interrupt enable
		
	ADC10CTL1 =
		INCH_var |		//Começa com pino P1.7 para ler
		ADC10DIV_7 |	//Divide ADC10CLK by 7
		ADC10SSEL_2 |	//Source SELect: MCLK
		CONSEQ_0 ;		//Single-chanel-single-conversion
		
	ADC10AE0 = FLIP | NBATI | NBATS; //Habilita os pinos para leitura
	
	__bis_SR_register(GIE);		// General Interrupt Enable;	
	
// ===============================================================
// --- loop infinito ---
	while(1)
	{
		WDTCTL = watchdog_reset;
		//Coloca i_f em um registrador para ganhar em processamento
		register int menu = interrupt_flags;
		if (menu & 1)
		{
			//Gerenciamento para canal AR //handler1();
			deltaTy = (yfinal - yinit);
			y = deltaTy - deltaTymean;
			interrupt_flags &= ~1; //Protege contra repetição indevida
		}
		if (menu & 2)
		{
			deltaTx = (xfinal - xinit);
			x = deltaTx - deltaTxmean;
			//Salva x e y em um array de ARRAYMAX el. para depois
			//tirar a média e então executar handler2
			valoresToArray(x,y);

			//Falta decidir se os valores ainda devem ser filtrados aos motores
			
			//Gerenciamento para tratamento de ppm + canal ED
			handler2();
			interrupt_flags &= ~2;
		}
		if (menu & 4)
		{
			//Gerenciamento para led de bateria
			handler4();
			interrupt_flags &= ~4;
		}
		if (menu & 8)
		{
			//Gerenciamento para quando FLIP é acionado
			//Troca os lados dos motores
			handler8();					
			interrupt_flags &= ~8;
		}
		  if (menu & 16)
		  {
			// Obtem a média móvel dos deltaTx e deltaTy das últimas ARRAYMAX=8 amostras
			int xmean = 0;
			int ymean = 0;
			for (int i = 0; i < ARRAYMAX; i++)
			{
				xmean += Xarray[i];
				ymean += Yarray[i];
			}
			xmean = xmean >> 3;
			ymean = ymean >> 3;
			
			newDeltaTx = xmean + deltaTxmean; 
			newDeltaTy = ymean + deltaTymean; 

			
			callibrate(-1);
			callibrated = HIGH;
			interrupt_flags &= ~16;
		  }
	}// Fim loop
	return 0;
} //Fim main

// ================================================================
// --- ISR do IN_AR ----
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) input1(void)
{
	//Se for borda de descida
	if (TA0CCTL0 & CM_2)
	{
		yfinal = TA0CCR0;
		interrupt_flags |= 1;
	}
	else //Se for borda de subida
	{	
		yinit = TA0CCR0;
		//Ainda não está na hora de calcular y
		interrupt_flags &= ~1;
	}
	//Altera a borda de interrupção
	TA0CCTL0 ^= CM_3;
	
	//Reseta bit para indicar que está recebendo dados
	signalCheck &= ~1;
	//Recebendo e usando dados
	
}// Fim interrupt

// ================================================================
// --- ISR do IN_DE ---
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) input2(void)
{
	switch (TA0IV)
	{
		case TA0IV_TACCR1:
			if (TA0CCTL1 & CM_2) //Se for borda de descida
			{
				xfinal = TA0CCR1;
				interrupt_flags |= 2;
			}
			else //Se for borda de subida
			{
				xinit = TA0CCR1;
				//Cancela ISR com atraso de um ciclo de ppm
				interrupt_flags &= ~2;
			}
			TA0CCTL1 ^= CM_3; //Troca a borda de interrupção
			signalCheck &= ~2; //Indica que recebeu sinal
			break;
			//Se for estouro do timer_A0:
		case 10: //TAIFG: Estouro de timer
			//Blink de bateria: 90 ≃ 750ms /(2^16/8e6)
			if ((++blinkCounter) >= 90)
			{
				interrupt_flags |= 4;
				blinkCounter = 0;
			}
			//Polling para flip: 6 ≃ 50ms /(2^16/8e6)
			if ((++flipCounter) >= 6)
			{
				ADC10CTL1 &= ~0xf000;
				ADC10CTL0 |= (ENC | ADC10SC);
				flipCounter = 0;
			}

			//Tira média dos valores e manda aos motores
			if (arrayCounter >= ARRAYMAX - 1)
			{
				long int sum[] = {0,0};
				int i=0;
				for (i=0; i< ARRAYMAX; i++)
				{
					sum[0] += Xarray[i];
					sum[1] += Yarray[i];
				}
				//Preciso fazer de 3 uma constante
				sum[0] = sum[0] >> 3;
				//Só precisamos da word inferior, então
				//forçamos o compilador a apenas tomá-la.
				x = (int)sum[0];
				
				//Fazemos o mesmo para o lado direito
				sum[1] = sum[1] >> 3;
				y = (int)sum[1];
				//Aplicada média móvel, podemos usar o handler2
				handler2();
			}
			
			//Acende led indicando perda de sinal
			if (signalCheck)
			{
				signalLost = HIGH;
				P2OUT |= LED_B;
				P1OUT &= ~LED_G;
				//fail-secure;
	//			*pwmLptr = 0;
	//			*pwmRptr = 0;
			}
			else //Acende led verde se não houver problemas
			{
				signalLost = LOW;
				P2OUT &= ~LED_B;
				P1OUT |= (batOK && callibrated ? LED_G : 0);
			}
			signalCheck = 3; //Reinicia flag de sinal perdido
		break;

	}//Fim do switch
}// fim interrupt

// ================================================================
// --- ISR do ADC10 ---
void __attribute__ ((interrupt(ADC10_VECTOR))) adc_interrupt(void)
{
	ADC10CTL0 &= ~(ADC10ON + ENC); //Apenas ~ENC já dava
	switch (ADC10CTL1 & 0xf000)
	{
		case INCH_6: //BATI
			ADC_acc[0] = ADC10MEM;
			INCH_var ^= (INCH_6 ^ INCH_7);			
			break;
		case INCH_7: //BATS
			ADC_acc[1] = ADC10MEM;
			INCH_var ^= (INCH_6 ^ INCH_7);
			break;
		case INCH_0: //flip
			flipState = (ADC10MEM > flipThreshold)? HIGH : LOW;
			interrupt_flags |= 8;
			//If not callibrated, callibrate it
			if (!callibrated)
			{
				//Para os motores
//				*pwmLptr = 0;
//				*pwmRptr = 0;
				//Led fica amarelo
				P1OUT |= OUTPUT_PINS_1;
				P2OUT &= ~LED_B;
				//Verfica se o controle está no centro
				//if ((x<1600)&&(x>-1600)&&
				//	(y<1600)&&(y>-1600)) callibrateCounter++;
				//else
				//{
				if (flipState)
				{
					callibrateCounter++;
					if (deltaTx > newDeltaTxmax)
						newDeltaTxmax = deltaTx;
					if (deltaTx < newDeltaTxmin)
						newDeltaTxmin = deltaTx;
					if (deltaTy > newDeltaTymax)
						newDeltaTymax = deltaTy;
					if (deltaTy < newDeltaTymin)
						newDeltaTymin = deltaTy;
				//	callibrateCounter = 0;
				}
				//Se estiver no centro por 3 segundos, salva valores
				//if (callibrateCounter >= 610) //3seg
				else
				{
//					callibrate(callibrateCounter>10);
					callibrated  = HIGH;
				}
			}
			break;
	}
	ADC10CTL1|= INCH_var; //Restabelece para os pinos de bateria
	ADC10CTL0 |= ADC10ON;
}

/*
//Interrupção causada pelo pino P2.3
void __attribute__ ((interrupt(PORT2_VECTOR))) calib_interrupt(void)
{
interrupt_flags |= 16;	
}
*/
