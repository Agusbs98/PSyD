/*-------------------------------------------------------------------
**
**  Fichero:
**    system.h  25/2/2015
**
**    (c) J.M. Mendias
**    Programación de Sistemas y Dispositivos
**    Facultad de Informática. Universidad Complutense de Madrid
**
**  Propósito:
**    Contiene las definiciones de los prototipos de las funciones
**    para la inicialización del sistema
**
**  Notas de diseño:
**
**-----------------------------------------------------------------*/
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <common_types.h>

#define NOP \
  asm volatile ( "nop" );
  
#define SWI( num ) \
  asm volatile ( "swi %0" : : "i" (num) )

#define GET_SWI_NUMBER( num_p )                          \
  asm volatile ( "ldr r0, [lr, #-4]" : : : "r0" );       \
  asm volatile ( "bic r0, r0, #0xff000000" : : : "r0" ); \
  asm volatile ( "str r0, %0" : "=m" (*num_p) : : );
  
#define INT_DISABLE \
  asm volatile( "mrs   r0, cpsr" : : : "r0" ); \
  asm volatile( "stmfd sp!, {r0}" ); \
  asm volatile( "orr   r0, r0, #0b11000000" ); \
  asm volatile( "msr   cpsr_c, r0" );
	
#define INT_ENABLE \
    asm volatile( "ldmfd sp!, {r0}" : : : "r0" ); \
	asm volatile( "msr   cpsr_c, r0" );

#define IRQ_NESTING_ENABLE                      \
  asm volatile ( "mrs   lr, spsr" );            \
  asm volatile ( "stmfd sp!, {lr}" );           \
  asm volatile ( "msr   cpsr_c, #0b00011111" ); \
  asm volatile ( "stmfd sp!, {lr}" );

#define IRQ_NESTING_DISABLE                     \
  asm volatile ( "ldmfd sp!, {lr}" );           \
  asm volatile ( "msr   cpsr_c, #0b10010010" ); \
  asm volatile ( "ldmfd sp!, {lr}" );           \
  asm volatile ( "msr   spsr, lr" );

#define LOCK_MUTEX( mutex_p )                               \
  asm ( "mov r0, #1" : : : "r0" );                 \
  asm ( "swpb r0, r0, [%0]" : : "r" (mutex_p) : ); \
  asm ( "cmp r0, #1" );                            \
  asm ( "subeq pc, pc, #20")

#define UNLOCK_MUTEX( mutex_p )                            \
  asm ( "mov r0, #0" : : : "r0" );                \
  asm ( "strb r0, [%0]" : : "r" (mutex_p) )

  
#define MEMBAR                          \
  asm volatile ( "" : : : "memory" );

/*
**  Inicializa el sistema
**    Deshabilita el watchdog
**    Enmascara todas las interrupciones
**    Configura el gestor de reloj
**      Tiempo de estabilización del PLL máximo
**      MCLK = 64 MHz
**      MCLK_SLOW = 500 KHz
**      Modo de funcionamiento normal
**      Reloj distribuido a todos los controladores
**    Arbitro de bus con prioridades por defecto
**    Cache deshabilitada
**    Configura los puertos de E/S
**    Configura el controlador de interrupciones
**      Fija prioridades por defecto
**      Todas las fuentes de interrupción en modo IRQ
**      Instala dummy ISR
**      Borra interrupciones pendientes externas e internas
**      IRQ vectorizadas, linea IRQ activada, linea FIQ desactivada
**  Inicializa el UART0
**  Muestra información del sistema por la UART0
*/
void sys_init( void );

/*
**  Pone el procesador en estado IDLE
*/
inline void sleep( void );

/*
**  Devuelve un puntero al comienzo de una región libre y contigua de memoria del tamaño indicado
*/
void *getmem( uint32 nbytes );

#endif
