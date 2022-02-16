
#include <s3c44b0x.h>
#include <leds.h>

void leds_init( void )
{
	PCONB &= ~((1<<10)|(1<<9));  // PB[10] = out, PF[9] = out
	led_off(LEFT_LED);
	led_off(RIGHT_LED);
}

void led_on( uint8 led )
{

    PDATB &= ~(led << 9);
}

void led_off( uint8 led )
{

    PDATB |= (led << 9);
}

void led_toggle( uint8 led )
{

    PDATB ^=(led<<9);
}

uint8 led_status( uint8 led )
{
	if(PDATB& led<<9){
		return 0;
	}
	else return 1;

	//return 0;
}
