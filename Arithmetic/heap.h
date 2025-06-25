#ifndef __HEAP_H__
#define __HEAP_H__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "stm32f10x.h"

#define PRIVILEGED_FUNCTION
#define PRIVILEGED_DATA
#define FREERTOS_SYSTEM_CALL

#define configTOTAL_HEAP_SIZE ((size_t) (15 * 1024))

#define portBYTE_ALIGNMENT    8

#if portBYTE_ALIGNMENT == 32
    #define portBYTE_ALIGNMENT_MASK    ( 0x001f )
#elif portBYTE_ALIGNMENT == 16
    #define portBYTE_ALIGNMENT_MASK    ( 0x000f )
#elif portBYTE_ALIGNMENT == 8
    #define portBYTE_ALIGNMENT_MASK    ( 0x0007 )
#elif portBYTE_ALIGNMENT == 4
    #define portBYTE_ALIGNMENT_MASK    ( 0x0003 )
#elif portBYTE_ALIGNMENT == 2
    #define portBYTE_ALIGNMENT_MASK    ( 0x0001 )
#elif portBYTE_ALIGNMENT == 1
    #define portBYTE_ALIGNMENT_MASK    ( 0x0000 )
#else /* if portBYTE_ALIGNMENT == 32 */
    #error "Invalid portBYTE_ALIGNMENT definition"
#endif /* if portBYTE_ALIGNMENT == 32 */

#define configASSERT assert_param

#ifndef portPOINTER_SIZE_TYPE
    #define portPOINTER_SIZE_TYPE    uint32_t
#endif

/*
 * Map to the memory management routines required for the port.
 */
void * pvPortMalloc( size_t xWantedSize ) PRIVILEGED_FUNCTION;
void * pvPortCalloc( size_t xNum,
                     size_t xSize ) PRIVILEGED_FUNCTION;
void vPortFree( void * pv ) PRIVILEGED_FUNCTION;
void vPortInitialiseBlocks( void ) PRIVILEGED_FUNCTION;
size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION;
size_t xPortGetAllHeapSize(void) PRIVILEGED_FUNCTION;
size_t xPortGetMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;

#if ( configSTACK_ALLOCATION_FROM_SEPARATE_HEAP == 1 )
    void * pvPortMallocStack( size_t xSize ) PRIVILEGED_FUNCTION;
    void vPortFreeStack( void * pv ) PRIVILEGED_FUNCTION;
#else
    #define pvPortMallocStack    pvPortMalloc
    #define vPortFreeStack       vPortFree
#endif

#endif /* __HEAP_H__ */