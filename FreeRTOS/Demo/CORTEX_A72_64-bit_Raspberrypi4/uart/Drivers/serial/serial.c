/*This file has been prepared for Doxygen automatic documentation generation.*/
/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* Demo application includes. */
#include "serial.h"

#define IE_TXWM        (1 << 0)
/*-----------------------------------------------------------*/

#define serINVALID_COMPORT_HANDLER        ( ( xComPortHandle ) 0 )
#define serINVALID_QUEUE                  ( ( xQueueHandle ) 0 )
#define serHANDLE                         ( ( xComPortHandle ) 1 )
#define serNO_BLOCK                       ( ( portTickType ) 0 )

/*-----------------------------------------------------------*/

/* Queues used to hold received characters, and characters waiting to be
transmitted. */
static xQueueHandle xRxedChars;
static xQueueHandle xCharsForTx;

/*-----------------------------------------------------------*/

/* Forward declaration. */
static void vprvSerialCreateQueues( unsigned portBASE_TYPE uxQueueLength,
                                    xQueueHandle *pxRxedChars,
                                    xQueueHandle *pxCharsForTx );

static portBASE_TYPE prvUSART_ISR_NonNakedBehaviour( void )
{
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        unsigned char cChar;
        portBASE_TYPE retstatus;

        extern unsigned char is_uart_rx_ready(void);
        extern unsigned char get_uart_char_from_chip(void);

        extern unsigned char is_uart_tx_ready(void);
        extern void put_uart_char_to_chip(uint8_t c);

	if (is_uart_tx_ready())
        {
                /* The interrupt was caused by the THR becoming empty.  Are there any
                   more characters to transmit?
                   Because FreeRTOS is not supposed to run with nested interrupts, put all OS
                   calls in a critical section . */
                portENTER_CRITICAL();
                retstatus = xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken );
                portEXIT_CRITICAL();

                if (retstatus == pdTRUE)
                {
                        /* A character was retrieved from the queue so can be sent to the
                           THR now. */
                        put_uart_char_to_chip(cChar);
                }
                else
                {
                        /* Queue empty, nothing to send so turn off the Tx interrupt. */
                }
        }

        if (is_uart_rx_ready())
        {
                /* The interrupt was caused by the receiver getting data. */
                cChar = get_uart_char_from_chip();

                /* Because FreeRTOS is not supposed to run with nested interrupts, put all OS
                   calls in a critical section . */
                portENTER_CRITICAL();
                xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken);
                portEXIT_CRITICAL();
        }

        /* The return value will be used by portEXIT_SWITCHING_ISR() to know if it
           should perform a vTaskSwitchContext(). */
        return ( xHigherPriorityTaskWoken );
}

void vUSART_ISR( void )
{
#if 0
    //portENTER_SWITCHING_ISR();
#endif

    prvUSART_ISR_NonNakedBehaviour();

#if 0
    //portEXIT_SWITCHING_ISR();
#endif
}
/*-----------------------------------------------------------*/

/*
 * Init the serial port for the Minimal implementation.
 */
xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
        xComPortHandle    xReturn = serHANDLE;

        /* Create the rx and tx queues. */
        vprvSerialCreateQueues( uxQueueLength, &xRxedChars, &xCharsForTx );

        /* Configure USART. */
        if( ( xRxedChars != serINVALID_QUEUE ) &&
                        ( xCharsForTx != serINVALID_QUEUE ) &&
                        ( ulWantedBaud != ( unsigned portLONG ) 0 ) )
        {
                portENTER_CRITICAL();
                {
                        extern void uart_init(void);
                        extern void uart_puts(const char* str);

                        uart_init();

                        uart_puts("Hello World !!!\r\n");
                }
                portEXIT_CRITICAL();
        }
        else
        {
                xReturn = serINVALID_COMPORT_HANDLER;
        }

        return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed portCHAR *pcRxedChar, portTickType xBlockTime )
{
    /* The port handle is not required as this driver only supports UART0. */
    ( void ) pxPort;

    /* Get the next character from the buffer.  Return false if no characters
    are available, or arrive before xBlockTime expires. */
    if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
    {
        return pdTRUE;
    }
    else
    {
        return pdFALSE;
    }
}
/*-----------------------------------------------------------*/

void vSerialPutString( xComPortHandle pxPort, const signed portCHAR * const pcString, unsigned portSHORT usStringLength )
{
     signed portCHAR *pxNext;



    /* NOTE: This implementation does not handle the queue being full as no
    block time is used! */

    /* The port handle is not required as this driver only supports UART0. */
    ( void ) pxPort;
    ( void ) usStringLength;

    /* Send each character in the string, one at a time. */
    pxNext = ( signed portCHAR * ) pcString;
    while( *pxNext )
    {
        xSerialPutChar( pxPort, *pxNext, serNO_BLOCK );
        pxNext++;
    }
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed portCHAR cOutChar, portTickType xBlockTime )
{
#if 0
    /* Place the character in the queue of characters to be transmitted. */
    if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) != pdPASS )
    {
        return pdFAIL;
    }

    /* Turn on the Tx interrupt so the ISR will remove the character from the
    queue and send it.   This does not need to be in a critical section as
    if the interrupt has already removed the character the next interrupt
    will simply turn off the Tx interrupt again. */
    usart->ier = (1 << AVR32_USART_IER_TXRDY_OFFSET);

    return pdPASS;
#else
    	( void ) pxPort;
	( void ) xBlockTime;

        extern void uart_putchar(uint8_t c);
        uart_putchar(cOutChar);
    	return pdPASS;
#endif
}
/*-----------------------------------------------------------*/

void vSerialClose( xComPortHandle xPort )
{
    	( void ) xPort;
  /* Not supported as not required by the demo application. */
}
/*-----------------------------------------------------------*/

/*###########################################################*/

/*
 * Create the rx and tx queues.
 */
static void vprvSerialCreateQueues(  unsigned portBASE_TYPE uxQueueLength, xQueueHandle *pxRxedChars, xQueueHandle *pxCharsForTx )
{
    /* Create the queues used to hold Rx and Tx characters. */
    xRxedChars = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
    xCharsForTx = xQueueCreate( uxQueueLength + 1, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );

    /* Pass back a reference to the queues so the serial API file can
    post/receive characters. */
    *pxRxedChars = xRxedChars;
    *pxCharsForTx = xCharsForTx;
}


