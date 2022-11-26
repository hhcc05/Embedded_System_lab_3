//#include "st_basic.h"
#include <stm32l4xx.h>

const char keypad[16] = { '1', '2', '3', 'A',
	                        '4', '5', '6', 'B',
	                        '7', '8', '9', 'C',
	                        '*', '0', '#', 'D' };
const char password[4] = { '1', '2', '3', '4' }; // password


unsigned char isPushed[16] = { };
unsigned int sequence = 0;
typedef enum GPIO_Mode { GPIO_INPUT, GPIO_OUTPUT, GPIO_ALTERNATIVE, GPIO_ANALOG, GPIO_INPUT_PULLUP, GPIO_INPUT_PULLDOWN = 0x8} GPIO_Mode; // enum for GPIO_mode

void PasswordInput(char character);
void ClockInit(void);
void Delay(unsigned int duration);
void GPIO_Init(GPIO_TypeDef *port, unsigned int pin, GPIO_Mode mode);
unsigned int sysMillis = 0;

int main(void)
{
	ClockInit();
	for (int i = 0; i <= 3; i++) GPIO_Init(GPIOA, i, GPIO_INPUT_PULLDOWN); // set PA0~3 to input with pulldown
	GPIO_Init(GPIOB, 2, GPIO_OUTPUT); // set PB2 to output
	GPIO_Init(GPIOE, 8, GPIO_OUTPUT); // set PE8 to output
	
	GPIOB->BSRR = GPIO_BSRR_BS2; // Turn on Red LED
	
	while (1)
	{
		for (int i = 0; i < 4; i++)
		{
			GPIOE->BSRR |= (1 << ((i - 1 + 4) % 4 + 12 + 16)); // output '0' to previous GPIO
			Delay(1);
			GPIO_Init(GPIOE, (i - 1 + 4) % 4 + 12, GPIO_ANALOG); // convert the previous pin to the default mode.
			
			GPIO_Init(GPIOE, i + 12, GPIO_OUTPUT); // set current pin to output
			GPIOE->BSRR |= (1 << (i + 12)); // output '1' to current pin.
			Delay(1);
			
			for (int j = 0; j < 4; j++)
			{
				if (GPIOA->IDR & (1 << j)) // read PA0~3 to find which keypad is pressed
				{
					if (!isPushed[4 * i + j]) // check array to block consecutive input
					{
						PasswordInput(keypad[4 * i + j]); // compare with password
						isPushed[4 * i + j] = 1; // set '1' value to array to block consecutive input
					}
				}
				
				else isPushed[4 * i + j] = 0; // clear matching values in an array
			}
		}
	}
}

void PasswordInput(char character)
{
	if (password[sequence] == character) // compare whether the entered value is the same as the current digit of the password.
	{
		if (sequence == 3) // if password is correct
		{
			GPIOB->BSRR = GPIO_BSRR_BR2; // turn off red led
			GPIOE->BSRR = GPIO_BSRR_BS8; // turn on green led
			Delay(3000);
			GPIOB->BSRR = GPIO_BSRR_BS2; // turn on red led
			GPIOE->BSRR = GPIO_BSRR_BR8; // turn off green led
			
			sequence = 0; // reset sequence value
		}
		else sequence++; // increase sequence to compare with next digit of the password
	}
	else sequence = 0; // if entered value is wrong, reset sequeunce to compare with first digit of the password 
}

void ClockInit(void)
{
	//Increase the delay by 4 wait states(5 clock cycles) to read the flash
	FLASH->ACR |= FLASH_ACR_LATENCY_4WS;
	
	//Enable PLLR that can be used as the system clock
	//Divide the 16MHz input clock by 2(to 8MHz), multiply by 20(to 160MHz),
	//divide by 2(to 80MHz)
	//Set PLL input source to HSI
	RCC->PLLCFGR = RCC_PLLCFGR_PLLREN | (20 << RCC_PLLCFGR_PLLN_Pos)
							 | RCC_PLLCFGR_PLLM_0 | RCC_PLLCFGR_PLLSRC_HSI;
	
	//Turn on HSI oscillator and PLL circuit.
	RCC->CR |= RCC_CR_PLLON | RCC_CR_HSION;
	
	//Be sure that the wait state of the flash changed,
	//PLL circuit is locked, and HSI is stabilized
	while (!((FLASH->ACR & FLASH_ACR_LATENCY_4WS)
				&& (RCC->CR & RCC_CR_PLLRDY)
				&& (RCC->CR & RCC_CR_HSIRDY)));
	
	//Set the system clock source from PLL
	RCC->CFGR = RCC_CFGR_SW_PLL;
	
	//Turn off MSI to reduce power consumption
	RCC->CR &= ~RCC_CR_MSION;
}

void GPIO_Init(GPIO_TypeDef *port, unsigned int pin, GPIO_Mode mode)
{
	//Get only the the last 2 digits of 'modeln32Bit'. To do that, AND it with 3(0b11). Then shift 'modeln32Bit' left to where the bits we want are.
	unsigned int modeIn32Bit = ((mode & 3) << (2 * pin));
	//get bit2, bit3 value and assign them to PUPDR to configure the appropriate pull-up/pull-down settings.
	unsigned int pullUpDown = ((mode >> 2) << (2 * pin));
	
	RCC->AHB2ENR |= (1 << (((unsigned int)port - GPIOA_BASE) >> 10)); // Turn on the clock of GPIO
	
	//Turn on the bit that 'modeln32Bit' has '1' in that position, by Bitwise OR
	port->MODER |= modeIn32Bit;
	//Fill '1' in every bit in 'modeln32Bit' except the two bits we are interested in.
	//Then turn off the bit that 'modeln32Bit' has '0' in that position, by Bitwise AND.
	port->MODER &= (modeIn32Bit | ~(3 << (2 * pin)));

	//Turn on the bit that 'pullUpDown' has '1' in that position, by Bitwise OR
	port->PUPDR |= pullUpDown;
	//Fill '1' in every bit in 'pullUpDown' except the two bits we are interested in.
	//Then turn off the bit that 'pullUpDown' has '0' in that position, by Bitwise AND.
	port->PUPDR &= (pullUpDown | ~(3 << (2 * pin)));
}

void Delay(unsigned int duration)
{
	//Temporarily store 'sysMillis' and compare the difference between
	//the value and 'duration' until the value be equal to or bigger than 'duration'
	//This method is okay with overflow
	unsigned int prevMillis = sysMillis;
	while (sysMillis - prevMillis < duration);
}
