#include <s3c44b0x.h>
#include <s3cev40.h>
#include <timers.h>

extern void isr_TIMER0_dummy( void );

static uint32 loop_ms = 0;
static uint32 loop_s = 0;

static void sw_delay_init( void );

void timers_init( void )
{
    TCFG0 = 0;//(TCFG0 & ~(0xff<<8))|(63 <<8);
    TCFG1 = 0;//(TCFG1 & ~(0xf))|(32<<12);

    TCNTB0 = 0x0;
    TCMPB0 = 0x0;
    TCNTB1 = 0x0;
    TCMPB1 = 0x0;
    TCNTB2 = 0x0;
    TCMPB2 = 0x0;
    TCNTB3 = 0x0;
    TCMPB3 = 0x0;//31250;
    TCNTB4 = 0x0;
    TCMPB4 = 0x0;
    TCNTB5 = 0x0;

    TCON = (1<25)|(1<<21)|(1<<17)|(1<<13)|(1<<9)|(1<<1); //(TCON & (0)) |(1<25)|(1<<21)|(1<<17)|(1<<13)|(1<<9)|(1<<1);
	TCON = ((0<25)|(0<<21)|(0<<17)|(0<<13)|(0<<9)|(0<<1));//(TCON & (0)) & (~((1<25)|(1<<21)|(1<<17)|(1<<13)|(1<<9)|(1<<1)));

    sw_delay_init();
}

static void sw_delay_init( void )
{
    uint32 i;
    
    timer3_start();
    for( i=1000000; i; i--);
    loop_s = ((uint64)1000000*10000)/timer3_stop();
    loop_ms = loop_s / 1000;
};

void timer3_delay_ms( uint16 n )
{
    for( ; n; n-- )
    {
    	TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);
		TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);

		TCNTB3 = 10;
		TCON = (TCON & ~(0xf << 16)) | (1 << 17);
		TCON = (TCON & ~(0xf << 16)) | (1 << 16);
		while( !TCNTO3 );
		while(TCNTO3);
    }
}

void sw_delay_ms( uint16 n )
{
    uint32 i;
    
    for( i=loop_ms*n; i; i-- );
}

void timer3_delay_s( uint16 n )
{
	for( ; n; n-- )
	{
		TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);
		TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);

		TCNTB3 = 10000;
		TCON = (TCON & ~(0xf << 16)) | (1 << 17);
		TCON = (TCON & ~(0xf << 16)) | (1 << 16);
		while( !TCNTO3 );
		while(TCNTO3);
	}

}

void sw_delay_s( uint16 n )
{
    uint32 i;
    
    for( i=loop_s*n; i; i-- );
}

void timer3_start( void ) 
{
    TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);    
    TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);
    
    TCNTB3 = 0xffff; 
    TCON = (TCON & ~(0xf << 16)) | (1 << 17);
    TCON = (TCON & ~(0xf << 16)) | (1 << 16);
    while( !TCNTO3 );
}

uint16 timer3_stop( void )
{
    TCON &= ~(1 << 16);
    return 0xffff - TCNTO3;
}

void timer3_start_timeout( uint16 n ) 
{
    TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);          
    TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);
    
    TCNTB3 = n; 
    TCON = (TCON & ~(0xf << 16)) | (1 << 17);
    TCON = (TCON & ~(0xf << 16)) | (1 << 16);
    while( !TCNTO3 );
}

uint16 timer3_timeout( )
{
    return !TCNTO3;
}    

void timer0_open_tick( void (*isr)(void), uint16 tps )
{
	pISR_TIMER0 = isr;
	I_ISPC      = I_ISPC |(BIT_TIMER0); //cambiado
	INTMSK     &= ~((BIT_TIMER0)|(BIT_GLOBAL));

    if( tps > 0 && tps <= 10 ) {
    	TCFG0  = (TCFG0 & ~(0xff)) | (49);//N=49
		TCFG1  = (TCFG1 & ~(0xf)) | (4);//D=32
        TCNTB0 = (40000U / tps);
    } else if( tps > 10 && tps <= 100 ) {
    	TCFG0  = (TCFG0 & ~(0xff)) | (9);//N=9
		TCFG1  = (TCFG1 & ~(0xf)) | (3);//D=16
        TCNTB0 = (400000U / (uint32) tps);
    } else if( tps > 100 && tps <= 1000 ) {
    	TCFG0  = (TCFG0 & ~(0xff)) | (1);//N=1
		TCFG1  = (TCFG1 & ~(0xf)) | (2);//D=8
        TCNTB0 = (4000000U / (uint32) tps);
    } else if ( tps > 1000 ) {
    	TCFG0  = (TCFG0 & ~(0xff));//N=0
		TCFG1  = (TCFG1 & ~(0xf));//D=2
        TCNTB0 = (32000000U / (uint32) tps);
    }

    TCON = (TCON & ~(0x3)) | (1<<1)|(1<<3);
	TCON = (TCON & ~(0x3)) | (1<<0)|(1<<3);
}

void timer0_open_ms( void (*isr)(void), uint16 ms, uint8 mode )
{
    pISR_TIMER0 = isr;
    I_ISPC      = I_ISPC |(BIT_TIMER0); //cambiado &~
    INTMSK     &= ~((BIT_TIMER0)|(BIT_GLOBAL));

    TCFG0  = (TCFG0 & ~(0xff)) | (199);//N=199+1
	TCFG1  = (TCFG1 & ~(0xf)) | (4);//D=32
    TCNTB0 = 10*ms;

    TCON = (TCON & ~(0xf)) | (1<<1)| mode<<3;
    TCON = (TCON & ~(0xf)) | (1<<0)| mode<<3;
}

void timer0_close( void )
{
    TCNTB0 = 0x0;
    TCMPB0 = 0x0;

    TCON = (TCON & ~(0x11)) | (1<<1);
    TCON = (TCON & ~(0x11)) | (1<<0);
    
    INTMSK     |= BIT_TIMER0;
    pISR_TIMER0 = isr_TIMER0_dummy;
}
