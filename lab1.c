/*-------------------------------------------------------------------
**
**  Fichero:
**    lab1.c  25/8/2016
**
**    (c) J.M. Mendias
**    Programación de Sistemas y Dispositivos
**    Facultad de Informática. Universidad Complutense de Madrid
**
**  Propósito:
**    Test del laboratorio 1
**
**  Notas de diseño:
**    Presupone que todo el SoC ha sido previamente configurado
**    por el programa residente en ROM que se ejecuta tras reset
**
**-----------------------------------------------------------------*/

#define SEGS (*(volatile unsigned char *)  0x2140000)
/*
void main(void) 
{

    unsigned char i;
    unsigned int j;

    SEGS = 0xff;
    while( 1 )
        for( i=0; i<16; i++ )
        {
            for( j=0; j<300000; j++ );
            switch( i ){
                case 0x00:
                    SEGS = 0x12; // 0b00010010;
                    break;
                case 0x01:
                    SEGS = 0x9f;
                    break;
                case 0x02:
                	SEGS = 0x31;
					break;
                case 0x03:
					SEGS = 0x15;
					break;
                case 0x04:
					SEGS = 0x9c;
					break;
                case 0x05:
					SEGS = 0x54;
					break;
                case 0x06:
					SEGS = 0x50;
					break;
                case 0x07:
					SEGS = 0x1f;
					break;
                case 0x08:
					SEGS = 0x10;
					break;
                case 0x09:
					SEGS = 0x1c;
					break;
                case 0x0A:
					SEGS = 0x18;
					break;
                case 0x0B:
					SEGS = 0xd0;
					break;
                case 0x0C:
					SEGS = 0x72;
					break;
                case 0x0D:
					SEGS = 0x91;
					break;
                case 0x0E:
					SEGS = 0x70;
					break;
                case 0x0f:
                    SEGS = 0x78;
                    break;
                default:
                    SEGS = 0xef;
                    break;
            }                    
        }

}

*/


const unsigned char hex2segs[16] = {0x12, 0x9f, 0x31,0x15,0x9c,0x54,0x50,0x1f,0x10,0x1c,0x18,0xd0,0x72,0x91,0x70,0x78};

void main(void) 
{

    unsigned char i;
    unsigned int j;

    SEGS = 0xff;
    while( 1 )
        for( i=0; i<16; i++ )
        {
            for( j=0; j<300000; j++ );
                SEGS = hex2segs[i];
        }

}


