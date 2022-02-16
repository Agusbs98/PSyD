#include <s3c44b0x.h>
#include <s3cev40.h>
#include <common_types.h>
#include <system.h>
#include <timers.h>
#include <lcd.h>
#include <pbs.h>
#include <keypad.h>

#define TICKS_PER_SEC (100)

/* Declaración de graficos */

#define LANDSCAPE  ((uint8 *)0x0c250000)
#define FIREMEN    ((uint8 *)0x0c260000)
#define CRASH      ((uint8 *)0x0c260800)
#define DUMMY_0    ((uint8 *)0x0c270000)
#define DUMMY_90   ((uint8 *)0x0c270400)
#define DUMMY_180  ((uint8 *)0x0c270800)
#define DUMMY_270  ((uint8 *)0x0c270C00)
#define LIFE       ((uint8 *)0x0c271000)

typedef struct plots {
    uint16 x;               // Posición x en donde se pinta el gráfico
    uint16 y;               // Posición y en donde se pinta el gráfico
    uint8 *plot;            // Puntero al BMP que contiene el gráfico
} plots_t;

typedef struct sprite {
    uint16 width;           // Anchura del gráfico en pixeles
    uint16 height;          // Altura del gráfico en pixeles
    uint16 num_plots;       // Número de posiciones diferentes en donde pintar el gráfico
    plots_t plots[];        // Array de posiciones en donde pintar el gráfico
} sprite_t;

const sprite_t firemen = 
{
    64, 32, 3,                      // Los bomberos de tamaño 64x32 se pintan en 3 posiciones distintas
    {
        {  32, 176, FIREMEN },
        { 128, 176, FIREMEN },
        { 224, 176, FIREMEN }
    }
};

const sprite_t dummy = 
{
    32, 32, 19,                    // Los dummies de tamaño 32x32 se pintan en 19 posiciones distintas con 4 formas diferentes que se alternan
    {
        {   0,  64, DUMMY_0   },
        {  16,  96, DUMMY_90  },
        {  32, 128, DUMMY_180 },
        {  48, 160, DUMMY_270 },
        {  64, 128, DUMMY_0   },
        {  80,  96, DUMMY_90  },
        {  96,  64, DUMMY_180 },
        { 112,  96, DUMMY_270 },
        { 128, 128, DUMMY_0   },
        { 144, 160, DUMMY_90  },
        { 160, 128, DUMMY_180 },
        { 176,  96, DUMMY_270 },
        { 192,  64, DUMMY_0   },
        { 208,  96, DUMMY_90  },
        { 224, 128, DUMMY_180 },
        { 240, 160, DUMMY_270 },
        { 256, 128, DUMMY_0   },
        { 272, 96,  DUMMY_90  },
        { 288, 64,  DUMMY_180 }
    }
};

const sprite_t crash = 
{
    64, 32, 3,                     // Los dummies estrellados de tamaño 64x32 se pintan en 3 posiciones distintas
    {
        {   32, 208, CRASH },
        {  128, 208, CRASH },
        {  224, 208, CRASH }
    }
};

const sprite_t life =
{
    16, 16, 3,                    // Los corazones estrellados de tamaño 16x16 se pintan en 3 posiciones distintas
    {
        {   8, 8, LIFE },
        {  24, 8, LIFE },
        {  40, 8, LIFE }
    }
};

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

volatile fifo_t fifo;       // Cola de tareas
boolean gameOver;           // Flag de señalización del fin del juego

/* Declaración de variables */

uint8 dummyPos;     // Posición del dummy 
uint16 count;       // Número de dummies salvados
uint8 firemenPos;	// Posicion del firemen
uint8 countLife;	// Contador de vidas
uint8 mode;			// Modo de juego
uint8 pause;		// Indicador de modo en pause o no
enum { wait_keydown, scan, wait_keyup } state_firemen,state_mode;

/* Declaración de funciones */

void dummy_init( void );                                    // Inicializa la posición del dummy y lo dibuja
void count_init( void );                                    // Inicializa el contador de dummies salvados y lo dibuja
void firemen_init(void); 									// Inicializa la posicion del firemen y lo dibuja
void mode_init( void );										// Inicializa el modo de juego
void sprite_plot( sprite_t const *sprite, uint16 pos );     // Dibuja el gráfico en la posición indicada
void sprite_clear( sprite_t const *sprite, uint16 pos );    // Borra el gráfico pintado en la posición indicada

/* Declaración de tareas */

void dummy_move( void );    // Mueve el dummy
void dummyCrash(void);		// El dummy choca con el suelo
void count_inc( void );     // Incrementa el contador de dummies salvados
void mode_change( void );	// Cambia el modo del juego
void new_mode( void );		// Establece el nuevo modo de juego y su configuracion
void firemen_move(void);	// Mueve el firemen
/* Declaración de RTI */

void isr_tick( void ) __attribute__ ((interrupt ("IRQ")));

/*******************************************************************/

void main( void )
{
    uint8 i;
    pf_t pf;
    
    sys_init();
    timers_init();
    lcd_init();
    pbs_init();
    
    lcd_on();
    lcd_clear();
	mode_init();								// Inicializa el modo

	lcd_putWallpaper( LANDSCAPE );              // Dibuja el fondo de la pantalla

	for( i=0; i<life.num_plots; i++ )           // Dibuja los corazones en todas sus posiciones posibles
		sprite_plot( &life, i );
	countLife = 3;
	lcd_puts_x2(150,0,BLACK, "Mode: ");		// Dibuja modo
	lcd_putint_x2( 230, 0, BLACK, mode );

	dummy_init();                               // Inicializa las tareas
	count_init();
	firemen_init();


	fifo_init();                                  // Inicializa cola de funciones
    timer0_open_tick( isr_tick, TICKS_PER_SEC );  // Instala isr_tick como RTI del timer0
           
    while( !gameOver )
    {
//      sleep();                        // Entra en estado IDLE, sale por interrupción
        while( !fifo_is_empty() )
        {
            pf = fifo_dequeue();
            (*pf)();                    // Las tareas encoladas se ejecutan en esta hebra (background) en orden de encolado
        }
        if(gameOver){
        	lcd_clear();
        	mode_init();
        	new_mode();
        }
    }
    lcd_puts_x2(88,120,BLACK,"GAME OVER ");
    
    timer0_close();
    while(1);
}

/*******************************************************************/

void dummy_init( void )
{
    dummyPos = 0;                           // Inicializa la posición del dummy...
    sprite_plot( &dummy, 0 );               // ... y lo dibuja
}

void dummy_move( void )
{
    sprite_clear( &dummy, dummyPos );       // Borra el dummy de su posición actual
    if( dummyPos == dummy.num_plots-1 )     // Si el dummy ha alcanzado la última posición...
    {
        dummyPos = 0;                       // ... lo coloca en la posición de salida
        fifo_enqueue( count_inc );          // ... incremeta el contador de dummies rescatados 
    } else{
        dummyPos++;                         // En caso contrario, avanza su posición
    	if(dummyPos == 4 && firemenPos !=0){
    		fifo_enqueue(dummyCrash);
    	}
    	if(dummyPos == 10 && firemenPos !=1){
    		fifo_enqueue(dummyCrash);
		}
    	if(dummyPos == 16 && firemenPos !=2){
    		fifo_enqueue(dummyCrash);
		}
    }
    sprite_plot( &dummy, dummyPos );        // Dibuja el dummy en la nueva posición   
}
void dummyCrash(void){
	uint8 pos = (dummyPos>=10)+(dummyPos>=16);
	countLife--;
	sprite_clear( &dummy, dummyPos );
	dummyPos = 0;
	sprite_plot(&crash, pos);
	if(!countLife){
		gameOver = TRUE;
	}
	sprite_plot( &dummy, dummyPos );
	sprite_clear(&life, countLife);

}

/*******************************************************************/

void firemen_init(void){
	firemenPos = 0;
	sprite_plot(&firemen,firemenPos);
	state_firemen = wait_keydown;
}
void firemen_move(void){
	uint8 scancode;
	switch(state_firemen){
	case wait_keydown:
		if( pb_pressed() )
			state_firemen = scan;
		break;
	case scan:
		sprite_clear(&firemen,firemenPos);
		scancode = pb_scan();
		if(scancode == PB_RIGHT && firemenPos < firemen.num_plots-1){
			firemenPos++;
		}
		else if(scancode == PB_LEFT && firemenPos > 0){
			firemenPos--;
		}
		state_firemen = wait_keyup;
		sprite_plot(&firemen, firemenPos);
		break;
	case wait_keyup:
		if( !pb_pressed() )
			state_firemen = wait_keydown;
		break;
	}
}

/*******************************************************************/

void mode_init( void ){
	lcd_puts_x2(60,0,BLACK, "NINTENDO GAME");
	lcd_puts_x2(10,40,BLACK, "Mode 1: Classic ");
	lcd_puts_x2(10,70,BLACK, "Mode 2: Advanced ");
	lcd_puts_x2(10,100,BLACK, "Mode 3: Infinity ");
	lcd_puts_x2(10,130,BLACK, "EXIT");
	uint8 scancode;
	while((scancode= keypad_scan())==KEYPAD_FAILURE);
	gameOver = FALSE;
	mode= 1;
	if(scancode == KEYPAD_KEY0){
		mode = 1;
	}
	else if(scancode == KEYPAD_KEY1){
		mode = 2;
	}
	else if(scancode == KEYPAD_KEY2){
		mode = 3;
	}
	else{
		gameOver = TRUE;
	}
	lcd_clear();
	state_mode = wait_keydown;
	pause =0;
}

void new_mode( void ){
    lcd_clear();
    lcd_putWallpaper( LANDSCAPE );              // Dibuja el fondo de la pantalla

    uint8 i;
    for( i=0; i<life.num_plots; i++ )           // Dibuja los corazones en todas sus posiciones posibles
        sprite_plot( &life, i );
    countLife = 3;

    dummy_init();                               // Inicializa las tareas
    count_init();
    firemen_init();
    lcd_puts_x2(150,0,BLACK, "Mode: ");			// Dibuja modo
	lcd_putint_x2( 230, 0, BLACK, mode );
    fifo_init();                                // Inicializa cola de funciones
}

void mode_change( void ){
	uint8 scancode;
	switch(state_mode){
	case wait_keydown:
		if( keypad_pressed() )
			state_mode = scan;
		break;
	case scan:
		scancode = keypad_scan();
		if(scancode == KEYPAD_KEY0 && mode != 1){
			mode = 1;
			fifo_enqueue(new_mode);
		}
		else if(scancode == KEYPAD_KEY1 && mode != 2){
			mode = 2;
			fifo_enqueue(new_mode);
		}
		else if(scancode == KEYPAD_KEY2 && mode != 3){
			mode = 3;
			fifo_enqueue(new_mode);
		}

		if(scancode == KEYPAD_KEY3){
			if(pause){
				pause = 0;
				lcd_puts_x2(150,0,BLACK, "Mode: ");		// Dibuja modo
				lcd_putint_x2( 230, 0, BLACK, mode );
			}
			else{
				pause = 1;
				lcd_puts_x2(150,0,BLACK,"PAUSE ");
			}
		}
		state_mode = wait_keyup;
		break;
	case wait_keyup:
		if( !keypad_pressed() )
			state_mode = wait_keydown;
		break;
	}
}

/*******************************************************************/

void count_init( void )
{
    count = 0;                              // Inicializa el contador de dummies salvados...
    lcd_putint_x2( 287, 0, BLACK, count );  // ... y lo dibuja
}

void count_inc( void )
{
    count++;                                // Incrementa el contador de dummies salvados
    lcd_putint_x2( 287, 0, BLACK, count );
    if( count == 9 && mode != 3)            // Si se han salvado 9 dummies...
        gameOver = TRUE;                    // ... señaliza fin del juego
}

/*******************************************************************/

void isr_tick( void )
{   
	static uint16 cont50ticks = 50;
	static uint16 cont5ticks = 5;
	if(!pause){
		if(!(--cont5ticks))
		{
			cont5ticks = 5;
			fifo_enqueue(mode_change);
			fifo_enqueue(firemen_move);
		}
		if(mode == 1){
			if( !(--cont50ticks) )
			{
				cont50ticks = 50;
				fifo_enqueue( dummy_move );
			}
		}
		else {
			if( (--cont50ticks)-(2*count) <=0 )
			{
				cont50ticks = 50;
				fifo_enqueue( dummy_move );
			}
		}
	}
	else{
		if(!(--cont5ticks))
		{
			cont5ticks = 5;
			fifo_enqueue(mode_change);
		}
	}
    I_ISPC = BIT_TIMER0;
};

/*******************************************************************/

extern uint8 lcd_buffer[];

void lcd_putBmp( uint8 *bmp, uint16 x, uint16 y, uint16 xsize, uint16 ysize );
void lcd_clearWindow( uint16 x, uint16 y, uint16 xsize, uint16 ysize );

void sprite_plot( sprite_t const *sprite, uint16 num )
{
    lcd_putBmp( sprite->plots[num].plot, sprite->plots[num].x, sprite->plots[num].y, sprite->width, sprite->height );
}

void sprite_clear( sprite_t const *sprite, uint16 num )
{
    lcd_clearWindow( sprite->plots[num].x, sprite->plots[num].y, sprite->width, sprite->height );
}

/*
** Muestra un BMP de tamaño (xsize, ysize) píxeles en la posición (x,y)
** Esta función es una generalización de lcd_putWallpaper
*/
void lcd_putBmp( uint8 *bmp, uint16 x, uint16 y, uint16 xsize, uint16 ysize )
{
	uint32 headerSize;

	uint16 xSrc, ySrc, yDst;
	uint16 offsetSrc, offsetDst;

	headerSize = bmp[10] + (bmp[11] << 8) + (bmp[12] << 16) + (bmp[13] << 24);

	bmp = bmp + headerSize; 

	for( ySrc=0, yDst=ysize-1; ySrc<ysize; ySrc++, yDst-- )
	{
		offsetDst = (yDst+y)*LCD_WIDTH/2+x/2;
		offsetSrc = ySrc*xsize/2;
		for( xSrc=0; xSrc<xsize/2; xSrc++ )
			lcd_buffer[offsetDst+xSrc] = ~bmp[offsetSrc+xSrc];
	}
}

/*
** Borra una porción de la pantalla de tamaño (xsize, ysize) píxeles desde la posición (x,y)
** Esta función es una generalización de lcd_clear
*/
void lcd_clearWindow( uint16 x, uint16 y, uint16 xsize, uint16 ysize )
{
	uint16 xi, yi;

	for( yi=y; yi<y+ysize; yi++ )
		for( xi=x; xi<x+xsize; xi++ )
			lcd_putpixel( xi, yi, WHITE );
}

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
