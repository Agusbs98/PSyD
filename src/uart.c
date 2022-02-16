#include <s3c44b0x.h>
#include <uart.h>

void uart0_init( void )
{
    UFCON0 = 0x1;
    UMCON0 = 0x0;
    ULCON0 = 0x3;
    UBRDIV0 = 0x22;
    UCON0 = 0x5;
}

void uart0_putchar( char ch )
{
    while( UFSTAT0 & (1<<9) );
    UTXH0 = ch;
}        

char uart0_getchar( void )
{
    while( (UFSTAT0 & 0xF) == 0);
    return URXH0;
}

void uart0_puts( char *s )
{
    while(*s != '\0'){
    	uart0_putchar(*s);
    	s++;
    }
}

void uart0_putint( int32 i )
{
	char buf[11 + 1];
	char *p = buf + 11;
	uint8 c;
	boolean neg = (i<0);
	*p = '\0';

	if(neg) i *= -1;
	if(i == 0)*--p = '0';
	while(i> 0){
		c = i % 10;
		i = i / 10;
		*--p = '0' + c;
	}

	if(neg) *--p = '-';
	/*
	do {
		c = i & v;
		*--p = '0' + c;
		v=v<<2;
	} while( i );
*/
	uart0_puts( p );
}

void uart0_puthex( uint32 i )
{
    char buf[8 + 1];
    char *p = buf + 8;
    uint8 c;

    *p = '\0';

    do {
        c = i & 0xf;
        if( c < 10 )
            *--p = '0' + c;
        else
            *--p = 'a' + c - 10;
        i = i >> 4;
    } while( i );

    uart0_puts( p );
}

void uart0_gets( char *s )
{
	char str;
	while((str= uart0_getchar()) != '\n'){
		*s = str;
		s++;
	}
	*s = '\0';
}

int32 uart0_getint( void )
{
    int32 salida = 0; //si no me guarda un valor anterior
    int32 sig;
    char car = uart0_getchar();
    boolean neg= FALSE;
    if(car == '-'){
    	neg = TRUE;
    	car = uart0_getchar();
    }
    while(car != '\n'){
    	sig = car - '0';
    	salida = salida*10;
    	if(neg) salida-= sig;
    	else salida += sig;
    	car = uart0_getchar();
    }
    return salida;
}

uint32 uart0_gethex( void )
{
	int32 num = 0;
	char caracter;

	while((caracter = uart0_getchar()) != '\n'){
		if (caracter - '0' < 10){
			num = num*16 + (caracter - '0');
		}
		else if(caracter<='Z'){
			num = num*16 + (caracter - 'A' + 10);
		}
		else{
			num = num*16 + (caracter - 'a' + 10);
		}

	}
	return num;
}
