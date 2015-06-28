//copyright Burhan wani
//
#include <stdint.h>
#include <stdbool.h>
#include<string.h>
#include "inc\tm4c123gh6pm.h"
#define MOTOR1_L  (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 1*4))) // pb1
#define MOTOR1_H  (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 2*4))) //pb2
#define MOTOR2_L  (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 3*4)))//pb3
#define MOTOR2_H  (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 4*4)))//pb4


char value;
int task;
void (*fn)(void);
void forward();
void backward(void);
void left(void);
void right(void);
void idle(void);
char prev;
int flag=0;
long int loadval=900;

void initHw()
{
	// Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
	SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S)| SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_2;

	// Set GPIO ports to use APB (not needed since default configuration -- for clarity)
	// Note UART on port A must use APB
	SYSCTL_GPIOHBCTL_R = 0;

	// Enable GPIO port A and F and C peripherals
	SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOE |SYSCTL_RCGC2_GPIOB|SYSCTL_RCGC2_GPIOD;


	 GPIO_PORTE_DIR_R |= 0x30;   // make bits 4 and 5 outputs
	    GPIO_PORTE_DR2R_R |= 0x30;  // set drive strength to 2mA
	    GPIO_PORTE_DEN_R |= 0x30;   // enable bits 4 and 5 for digital
	    GPIO_PORTE_AFSEL_R |= 0x30; // select auxilary function for bits 4 and 5
	    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE4_M0PWM4 | GPIO_PCTL_PE5_M0PWM5; // enable PWM on bits 4 and 5


	    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;             // turn-on PWM0 module
	        __asm(" NOP");                                   // wait 3 clocks
	        __asm(" NOP");
	        __asm(" NOP");
	        SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
	        SYSCTL_SRPWM_R = 0;                              // leave reset state
	       // PWM0_1_CTL_R = 0;                                // turn-off PWM0 generator 1
	        PWM0_2_CTL_R = 0;                                // turn-off PWM0 generator 2
	      //  PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
	                                                         // output 3 on PWM0, gen 1b, cmpb
	        PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
	                                                         // output 4 on PWM0, gen 2a, cmpa
	        PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
	                                                         // output 5 on PWM0, gen 2b, cmpb
	       // PWM0_1_LOAD_R = 1024;                            // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
	        PWM0_2_LOAD_R = 1024;
	        PWM0_INVERT_R =  PWM_INVERT_PWM4INV | PWM_INVERT_PWM5INV;
	                                                         // invert outputs for duty cycle increases with increasing compare values
	       // PWM0_1_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
	        PWM0_2_CMPB_R = 0;                               // green off
	        PWM0_2_CMPA_R = 0;                               // blue off

	       // PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
	        PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
	        PWM0_ENABLE_R =  PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;

	GPIO_PORTB_DEN_R = 0x1E;
	GPIO_PORTB_DIR_R = 0x1E;


		SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R2;         // turn-on UART2, leave other uarts in same status
		GPIO_PORTD_DEN_R |= 0xC0;                           // default, added for clarity
		GPIO_PORTD_AFSEL_R |= 0xC0;                         // default, added for clarity
		GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD6_U2RX | GPIO_PCTL_PD7_U2TX;// see pg 1344

		// Configure UART2 to 9600 baud, 8N1 format (must be 3 clocks from clock enable and config writes)
		UART2_CTL_R = 0;                                 // turn-off UART0 to allow safe programming
		UART2_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
		UART2_IBRD_R = 260;                               // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
		UART2_FBRD_R = 27;                               // round(fract(r)*64)=45
		UART2_LCRH_R |= UART_LCRH_WLEN_8 ; // configure for 8N1 w/ 16-level FIFO
		UART2_CTL_R |= UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; // enable TX, RX, and module

		UART2_IM_R = UART_IM_RXIM;                       // turn-on RX interrupt
		NVIC_EN1_R |= 1<<1;



}


void waitMicrosecond(uint32_t us)
{
	// Approx clocks per us
	__asm("WMS_LOOP0:   MOV  R1, #6");          // 1
	__asm("WMS_LOOP1:   SUB  R1, #1");          // 6
	__asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
	__asm("             NOP");                  // 5
	__asm("             B    WMS_LOOP1");       // 5*3
	__asm("WMS_DONE1:   SUB  R0, #1");          // 1
	__asm("             CBZ  R0, WMS_DONE0");   // 1
	__asm("             B    WMS_LOOP0");       // 1*3
	__asm("WMS_DONE0:");                        // ---
	// 40 clocks/us + error
}
void Uart2ISR()
{

	value = UART2_DR_R;


}


void Scheduler()
{

	if(value=='f')
	{

		prev=value;
		MOTOR1_L = 0;
		MOTOR1_H = 1;
		MOTOR2_L = 0;
		MOTOR2_H = 1;

		if(flag)
		{
			loadval=800;
			flag=0;
		}

		PWM0_2_CMPA_R = loadval ;
		PWM0_2_CMPB_R = loadval;
	
	}

	else if (value== 'b')
	{
		prev=value;
		MOTOR1_L = 1;
		MOTOR1_H = 0;
		MOTOR2_L = 1;
		MOTOR2_H = 0;

		if(flag)
				{
					loadval=800;
					flag=0;
				}
		PWM0_2_CMPA_R = loadval ;
		PWM0_2_CMPB_R = loadval;

	}
	else if(value== 'l')
	{
		//;
		prev=value;
		MOTOR1_L = 1;
		MOTOR1_H = 0;
		MOTOR2_L = 0;
		MOTOR2_H = 1;
		if(flag)
				{
					loadval=800;
					flag=0;
				}
		PWM0_2_CMPA_R = 1000 ;
		PWM0_2_CMPB_R = 900;
		waitMicrosecond(3000000);
		value='f';
		

	}
	else if(value== 'r')
	{
		
		prev=value;
		MOTOR1_L = 0;
		MOTOR1_H = 1;
		MOTOR2_L = 1;
		MOTOR2_H = 0;
		if(flag)
				{
					loadval=800;
					flag=0;
				}
	
		PWM0_2_CMPA_R = 1000 ;
	
		PWM0_2_CMPB_R = 900;
		waitMicrosecond(3000000);
		value='f';
	

	}

	else if(value== 'm')
	{
		
		value=prev;
		loadval-=50;
		if (loadval<0)
		loadval=0;
	
	}
	else if(value== 'p')
	{
		value=prev;
		loadval+=50;
		if(loadval>1023)
		loadval=1023;
		
	}
	else if(value== 's')
	{
		PWM0_2_CMPA_R = 0 ;
		PWM0_2_CMPB_R = 0;
		flag=1;
	}


}





int main()
{
	initHw();

	while(1)
	{

		Scheduler();

	}

}
