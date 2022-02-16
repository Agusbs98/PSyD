/*-------------------------------------------------------------------
**
**  Fichero:
**    lab15.c  14/1/2021
**
**    (c) J.M. Mendias
**    Programación de Sistemas y Dispositivos
**    Facultad de Informática. Universidad Complutense de Madrid
**
**  Propósito:
**    Ejemplo de una aplicación guiada por tiempo con arquitectura
**    cola de funciones tipo FIFO
**
**  Notas de diseño:
**
**-----------------------------------------------------------------*/

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

#define TICKS_PER_SEC   (100)

/* Declaración de fifo de punteros a funciones */

#define BUFFER_LEN   (512)

typedef void (*pf_t)(void);

typedef struct fifo {
    uint16 head;
    uint16 tail;
    uint16 size;
    pf_t buffer[BUFFER_LEN];
} fifo_t;

void fifo_init( void );
void fifo_enqueue( pf_t pf );
pf_t fifo_dequeue( void );
boolean fifo_is_empty( void );
boolean fifo_is_full( void );

/* Declaración de recursos */

volatile fifo_t fifo;

uint8 scancode;

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
void isr_tick( void ) __attribute__ ((interrupt ("IRQ")));

/*******************************************************************/

void main( void )
{
    pf_t pf;
    
    sys_init();      /* Inicializa el sistema */
    timers_init();
    uart0_init();
    leds_init();
    segs_init();
    rtc_init();
    pbs_init();
    keypad_init(); 
    lcd_init();
    lcd_clear();
    lcd_on();
      
    uart0_puts( "\n\n Ejecutando una aplicación como cola de funciones\n" );
    uart0_puts( " ------------------------------------------------\n\n" ) ;

    fifo_init();             /* Inicializa cola de funciones */

    Task1();    /* Ejecuta por primera vez a las funciones para inicializarlas */
    Task2();
    Task3();
    Task4();
    Task5();
    Task6();
    Task7();
    Task8();
    Task9();

    pbs_open( isr_pb );                           /* Instala isr_pb como RTI por presión de pulsadores  */
    timer0_open_tick( isr_tick, TICKS_PER_SEC );  /* Instala isr_tick como RTI del timer0  */
        
    while( 1 )
    {
        sleep();                        /* Entra en estado IDLE, sale por interrupción */
        while( !fifo_is_empty() )
        {
            pf = fifo_dequeue();
            (*pf)();                    /* Las tareas encoladas se ejecutan en esta hebra (background) en orden de encolado */
        }
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
                fifo_enqueue( Task5 );
                fifo_enqueue( Task6 );
                fifo_enqueue( Task8 );
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
    else
    {
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
    else
    {
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
    else
    {   
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

	    else
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
    fifo_enqueue( Task7 );
    EXTINTPND = BIT_RIGHTPB | BIT_LEFTPB;
    I_ISPC = BIT_PB;
}


void isr_tick( void )
{  
    static uint16 cont5ticks    = 5;
    static uint16 cont50ticks   = 50;
    static uint16 cont100ticks  = 100;
    static uint16 cont1000ticks = 1000;
    
    if( !(--cont5ticks) )
    {
        cont5ticks = 5;
        fifo_enqueue( Task2 );        
    }
    if( !(--cont50ticks) )
    {
        cont50ticks = 50;
        fifo_enqueue( Task1 );
    }        
    if( !(--cont100ticks) )
    {
        cont100ticks = 100;
        fifo_enqueue( Task3 );
        fifo_enqueue( Task9 );
    }    
    if( !(--cont1000ticks) )
    {
        cont1000ticks = 1000;
        fifo_enqueue( Task4 );        
    }   
    
    I_ISPC = BIT_TIMER0;
};

/*******************************************************************/

void fifo_init( void )
{
    fifo.head = 0;
    fifo.tail = 0;
    fifo.size = 0;
}

void fifo_enqueue( pf_t pf )
{
    fifo.buffer[fifo.tail++] = pf;
    if( fifo.tail == BUFFER_LEN )
        fifo.tail = 0;
    INT_DISABLE;
    fifo.size++;
    INT_ENABLE;
}

pf_t fifo_dequeue( void )
{
    pf_t pf;
    
    pf = fifo.buffer[fifo.head++];
    if( fifo.head == BUFFER_LEN )
        fifo.head = 0;
    INT_DISABLE;
    fifo.size--;
    INT_ENABLE;
    return pf;
}

boolean fifo_is_empty( void )
{
    return (fifo.size == 0);
}

boolean fifo_is_full( void )
{
    return (fifo.size == BUFFER_LEN-1);
}

/*******************************************************************/
