#include "gpio.h"
#include "pwr.h"
#include "main.h"
#include "Util/delay.h"

uint8_t bootsource;

void setup_gpio(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	//enable the clocks 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);//GPIO/AFIO clks
	setuppwr();				//configure power control
	disable_pin();				//disable WKUP pin functionality
	//Configure and read the Charger_EN/VBus pin - this has a pullup to V_USB, so if it reads 1 we booted off usb so setup USB detatch isr
	GPIO_InitStructure.GPIO_Pin = VBUS_DETECT;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init( GPIOB, &GPIO_InitStructure );/* configure pin 2 as input*/
	for(uint16_t n=1;n;n++) {		//USB insertion can be really messy, so loop to detect anything on chrg pin over a few milliseconds
		if(GET_VBUS_STATE) {		//We booted from USB
			bootsource=USB_SOURCE;	//so we know for reference later
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;//reset the pin to an open drain output
			#if BOARD<3		/*This pin is shared with charger control on early boards*/
				GPIO_Init( GPIOB, &GPIO_InitStructure );//This enables the pin to be used to shutdown the charger in suspend mode
			#else
				GPIO_InitStructure.GPIO_Pin = CHARGER_EN;
				GPIO_Init( GPIOA, &GPIO_InitStructure );// The TTDM pin on later boards				
			#endif
			CHRG_ON;		//default to charger enabled
			n=0;
			break;
		}
	}
	//Configure the io pins
	//Pull up the SD CS pin
	GPIO_InitStructure.GPIO_Pin =  SD_SEL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//pullup
	GPIO_Init( GPIOB, &GPIO_InitStructure );/* configure SDSEL pin as input pull up until the SD driver is intialised*/
	//Pull up all the SD SPI lines until the bus is intialized - SD spec says MISO and MOSI should be pulled up at poweron
	#if BOARD<3
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
		GPIO_Init( GPIOA, &GPIO_InitStructure );
	#else
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
		GPIO_Init( GPIOB, &GPIO_InitStructure );		
	#endif
	//LEDS
	GPIO_InitStructure.GPIO_Pin = RED|GREEN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOB, &GPIO_InitStructure );/* configure pins 11 and 12 as output*/
	//Power button
	GPIO_InitStructure.GPIO_Pin = WKUP;
	if(USB_SOURCE==bootsource)		//Configure for turnoff on usb removal or pwr button
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//pullup
	else
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;//pulldown
	GPIO_Init( GPIOA, &GPIO_InitStructure );/* configure WKUP pin as input pull down/up for button*/
	//Power supply enable
	GPIO_InitStructure.GPIO_Pin = PWREN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//pushpull
	GPIO_Init( GPIOB, &GPIO_InitStructure );
	GPIO_WriteBit(GPIOB,PWREN,Bit_SET);	//Make sure power enabled
	//Configure the ADC inputs
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init( GPIOA, &GPIO_InitStructure );
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init( GPIOB, &GPIO_InitStructure );
	//Configure the PWM outputs
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;//reduced slew rate to reduce interference on the board
	GPIO_PinRemapConfig( GPIO_FullRemap_TIM2, ENABLE );//to B.10
	#if BOARD<3
		GPIO_InitStructure.GPIO_Pin = PWM0|PWM1|PWM2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init( GPIOB, &GPIO_InitStructure );
	#else
		GPIO_InitStructure.GPIO_Pin = PWM0|PWM2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init( GPIOB, &GPIO_InitStructure );
		GPIO_InitStructure.GPIO_Pin = PWM1;
		GPIO_Init( GPIOA, &GPIO_InitStructure );
	#endif		
	//Configure the pump Motor PWM
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;//faster slew rate for the motor so mosfet spends more time fully on/off
	GPIO_InitStructure.GPIO_Pin = PWM_MOTOR;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );
	//Configure Solenoid pin
	GPIO_InitStructure.GPIO_Pin = SOLENOID;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	#if BOARD<3
		GPIO_Init( GPIOB, &GPIO_InitStructure );
		GPIO_WriteBit(GPIOB,SOLENOID,Bit_RESET);//Make sure solenoid off
	#else
		GPIO_Init( GPIOA, &GPIO_InitStructure );
		GPIO_WriteBit(GPIOA,SOLENOID,Bit_RESET);//Make sure solenoid off
	#endif
}

void switch_leds_on(void)
{
	if(USB_SOURCE==bootsource)
		GPIO_WriteBit(GPIOB,RED,Bit_SET);
	else
		GPIO_WriteBit(GPIOB,GREEN,Bit_SET);
}

void switch_leds_off(void)
{
	if(USB_SOURCE==bootsource)
		GPIO_WriteBit(GPIOB,RED,Bit_RESET);
	else
		GPIO_WriteBit(GPIOB,GREEN,Bit_RESET);
}

void red_flash(void)
{
	GPIO_WriteBit(GPIOB,RED,Bit_SET);
	Delay(400000);
	GPIO_WriteBit(GPIOB,RED,Bit_RESET);
}

uint8_t get_wkup()
{
	return GPIO_ReadInputDataBit(GPIOA,WKUP);
}
