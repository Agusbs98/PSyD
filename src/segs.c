#include <s3c44b0x.h>
#include <s3cev40.h>
#include <segs.h>

static const uint8 hex2segs[16] = {0x12, 0x9f, 0x31,0x15,0x9c,0x54,0x50,0x1f,0x10,0x1c,0x18,0xd0,0x72,0x91,0x70,0x78};

static uint8 state;

void segs_init( void )
{
    segs_off();
}

void segs_off( void )
{
    state = SEGS_OFF;
    SEGS = state;
}

void segs_putchar( uint8 n )
{
    state = n & 0x0f;
    SEGS = hex2segs[state];
};

uint8 segs_status( void )
{
    return state;
};


