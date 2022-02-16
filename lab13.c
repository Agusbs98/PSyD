/*-------------------------------------------------------------------
**
**  Fichero:
**    lab13.c  12/1/2021
**
**    (c) J.M. Mendias
**    Programación de Sistemas y Dispositivos
**    Facultad de Informática. Universidad Complutense de Madrid
**
**  Propósito:
**    Ejemplo de una aplicación bajo un kernel de planificación
**    no expropiativa de tareas cooperativas multiestado
**
**  Notas de diseño:
**
**-----------------------------------------------------------------*/

#include "kernelcoop.h"

#include <s3c44b0x.h>
#include <s3cev40.h>
#include <common_types.h>
#include <system.h>
#include <leds.h>
#include <segs.h>
#include <uart.h>
#include <pbs.h>
#include <keypad.h>
#include <timers.h>
#include <rtc.h>
#include <lcd.h>

/* Declaración de recursos */

uint8 scancode;
boolean flagTask5;
boolean flagTask6;
boolean flagTask8;

volatile boolean flagPb;

/* Declaración de tareas */

void Task1( void );
void Task2( void );
void Task3( void );
void Task4( void );
void Task5( void );
void Task6( void );
void Task7( void );
void Task8(void);
void Task9(void);

/* Declaración de RTI */

void isr_pb( void ) __attribute__ ((interrupt ("IRQ")));

/*******************************************************************/

void main( void )
{
    sys_init();      /* Inicializa el sistema */
    timers_init();
    uart0_init();
    leds_init();
    segs_init();
    rtc_init();
    pbs_init();
    keypad_init();
    lcd_init();
    lcd_on();
    lcd_clear();


    uart0_puts( "\n\n Ejecutando kernel de planificación no expropiativa\n" );
    uart0_puts( " --------------------------------------------------\n\n" ) ;

    flagTask5  = FALSE;               /* Inicializa flags */
    flagTask6  = FALSE;
    flagPb     = FALSE;
    flagTask8  = FALSE;

    scheduler_init();                 /* Inicializa el kernel */

    create_task( Task2, 5 );          /* Crea las tareas de la aplicación... */
    create_task( Task7, 5 );          /* ... el kernel asigna la prioridad según orden de creación: Task2 > Task5 > Task6 > ... */
    create_task( Task5, 10 );         /* ... las tareas más frecuentes tienen mayor prioridad (criterio Rate-Monotonic-Scheduling) */
    create_task( Task6, 10 );
    create_task(Task8, 10);
    create_task( Task1, 50 );
    create_task( Task3, 100 );
    create_task(Task9, 100);
    create_task( Task4, 1000 );

    timer0_open_tick( scheduler, TICKS_PER_SEC );  /* Instala scheduler como RTI del timer0  */
    pbs_open( isr_pb );                            /* Instala isr_pb como RTI por presión de pulsadores  */

    while( 1 )
    {
        sleep();                /* Entra en estado IDLE, sale por interrupción */
        dispacher();            /* Las tareas preparadas se ejecutan en esta hebra (background) en orden de prioridad */
    }

}

/*******************************************************************/

void Task1( void )  /* Cada 0,5 segundos (50 ticks) alterna el led que se enciende */
{
    static boolean init = TRUE;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 1: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
        led_on( LEFT_LED );
        led_off( RIGHT_LED );
    }
    else
    {
        led_toggle( LEFT_LED );
        led_toggle( RIGHT_LED );
    }
}

void Task2( void )  /* Cada 50 ms (5 ticks) muestrea el keypad y envía el scancode a otras tareas */
{
    static boolean init = TRUE;
    static enum { wait_keydown, scan, wait_keyup } state;

    if( init )
    {
        init  = FALSE;
        uart0_puts( " Task 2: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
        state = wait_keydown;
    }
    else switch( state )
    {
        case wait_keydown:
            if( keypad_pressed() )
                state = scan;
            break;
        case scan:
            scancode = keypad_scan();
            if( scancode != KEYPAD_FAILURE )
            {
                flagTask5 = TRUE;
                flagTask6 = TRUE;
                flagTask8 = TRUE;
            }
            state = wait_keyup;
            break;
        case wait_keyup:
            if( !keypad_pressed() )
                state = wait_keydown;
            break;
    }
}

void Task3( void  )  /* Cada segundo (100 ticks) muestra por la UART0 la hora del RTC */
{
    static boolean init = TRUE;
    rtc_time_t rtc_time;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 3: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
    }
    else
    {
        rtc_gettime( &rtc_time );
        uart0_puts( "  (Task 3) Hora: " );
        uart0_putint( rtc_time.hour );
        uart0_putchar( ':' );
        uart0_putint( rtc_time.min );
        uart0_putchar( ':' );
        uart0_putint( rtc_time.sec );
        uart0_puts( "\n" );
    }
}

void Task4( void )  /* Cada 10 segundos (1000 ticks) muestra por la UART0 los ticks transcurridos */
{
    static boolean init = TRUE;
    static uint32 ticks;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 4: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
        ticks = 0;
    }
    else
    {
        ticks += TICKS_PER_SEC * 10;
        uart0_puts( "  (Task 4) Ticks: " );
        uart0_putint( ticks );
        uart0_puts( "\n" );
    }
}

void Task5( void )  /* Cada vez que reciba un scancode lo muestra por la UART0 */
{
    static boolean init = TRUE;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 5: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
    }
    else if( flagTask5 )
    {
        flagTask5 = FALSE;
        uart0_puts( "  (Task 5) Tecla pulsada: " );
        uart0_puthex( scancode );
        uart0_puts( "\n" );
    }
}

void Task6( void )  /* Cada vez que reciba un scancode lo muestra por los 7 segmentos */
{
    static boolean init = TRUE;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 6: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
    }
    else if( flagTask6 )
    {
        flagTask6 = FALSE;
        segs_putchar( scancode );
    }
}

void Task7( void )  /* Cada vez que se presione un pulsador lo avisa por la UART0 */
{
    static boolean init = TRUE;

    if( init )
    {
        init = FALSE;
        uart0_puts( " Task 7: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
    }
    else if( flagPb )
    {   
        flagPb = FALSE;
        uart0_puts( "  (Task 7) Se ha pulsado algún pushbutton...\n" );
    }
}
void Task8(void){
	static boolean init = TRUE;

	if( init )
	{
		init = FALSE;
		lcd_draw_box( 10, 10, 310, 230, BLACK, 5 );
		uart0_puts( " Task 8: iniciada.\n" );  /* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
		lcd_puts( 20,32,BLACK," Task 8: iniciada.\n");
	}

	else if(flagTask8)
	{

		//lcd_clear();
		//lcd_draw_hline(0,LCD_WIDTH-1,32,0,16);
		lcd_draw_box( 10, 10, 310, 230, BLACK, 5 );
		lcd_puts( 20,32,BLACK,"  (Task 8) Tecla pulsada: " );
		//uart0_puthex( scancode );
		lcd_puthex(27*8,32,BLACK,scancode);
		//uart0_puts( "\n" );
	}
}
void Task9(void){
	static boolean init = TRUE;
	rtc_time_t rtc_time;
	uint8 pos = 20;
	if( init )
	{
		init = FALSE;

		lcd_draw_box( 10, 10, 310, 230, BLACK, 5 );
		uart0_puts( " Task 9: iniciada.\n" );/* Muestra un mensaje inicial en la UART0 (no es necesario semáforo) */
		lcd_puts(20,16,BLACK," Task 9: iniciada.\n");
	}
	else
	{
		rtc_gettime( &rtc_time );
		//uart0_puts( "  (Task 9) Hora: " );
		//uart0_putint( rtc_time.hour );
		//uart0_putchar( ':' );
		//uart0_putint( rtc_time.min );
		//uart0_putchar( ':' );
		//uart0_putint( rtc_time.sec );
		//uart0_puts( "\n" );
		//lcd_clear();
		//lcd_draw_hline(0,LCD_WIDTH-1,16,0,16);
		lcd_draw_box( 10, 10, 310, 230, BLACK, 5 );
		lcd_puts(pos,16,BLACK,"  (Task 9) Hora: " );
		pos = pos + 16*8;
		lcd_putint(pos,16,BLACK,rtc_time.hour);
		if(rtc_time.hour>9)pos = pos +1*8;
		pos = pos +1*8;
		lcd_puts(pos,16,BLACK,":");
		pos = pos +1*8;
		lcd_putint(pos,16,BLACK,rtc_time.min);
		if(rtc_time.min>9)pos = pos +1*8;
		pos = pos + 1*8;
		lcd_puts(pos,16,BLACK,":");
		pos = pos+1*8;
		lcd_putint(pos,16,BLACK,rtc_time.sec);
		if(rtc_time.sec>9)pos = pos +1*8;
		pos = pos +1*8;
		lcd_puts(pos,16,BLACK,"   ");
		//lcd_puts()

	}
}
/*******************************************************************/


void isr_pb( void )
{
    flagPb = TRUE;
    EXTINTPND = BIT_RIGHTPB | BIT_LEFTPB;
    I_ISPC = BIT_PB;
}

/*******************************************************************/
