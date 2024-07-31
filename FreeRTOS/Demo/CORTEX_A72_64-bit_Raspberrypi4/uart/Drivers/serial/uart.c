/* uart.c */
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "board.h"
#include "interrupt.h"
#include "semphr.h"
#include "uart.h"

struct UARTCTL {
    SemaphoreHandle_t tx_mux;
    QueueHandle_t     rx_queue;
};
struct UARTCTL *uartctl;

/* GPIO */

enum {
    PERIPHERAL_BASE = 0xFE000000,
    GPFSEL0         = PERIPHERAL_BASE + 0x200000,
    GPSET0          = PERIPHERAL_BASE + 0x20001C,
    GPCLR0          = PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0       = PERIPHERAL_BASE + 0x2000E4
};

enum {
    GPIO_MAX_PIN       = 53,
    GPIO_FUNCTION_OUT  = 1,
    GPIO_FUNCTION_ALT5 = 2,
    GPIO_FUNCTION_ALT3 = 7
};

enum {
    Pull_None = 0,
    Pull_Down = 2,
    Pull_Up = 1
};

void mmio_write(long reg, unsigned int val) { *(volatile unsigned int *)reg = val; }
unsigned int mmio_read(long reg) { return *(volatile unsigned int *)reg; }

unsigned int gpio_call(unsigned int pin_number, unsigned int value, unsigned int base, unsigned int field_size, unsigned int field_max) {
    unsigned int field_mask = (1 << field_size) - 1;

    if (pin_number > field_max) return 0;
    if (value > field_mask) return 0;

    unsigned int num_fields = 32 / field_size;
    unsigned int reg = base + ((pin_number / num_fields) * 4);
    unsigned int shift = (pin_number % num_fields) * field_size;

    unsigned int curval = mmio_read(reg);
    curval &= ~(field_mask << shift);
    curval |= value << shift;
    mmio_write(reg, curval);

    return 1;
}

unsigned int gpio_set     (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPSET0, 1, GPIO_MAX_PIN); }
unsigned int gpio_clear   (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPCLR0, 1, GPIO_MAX_PIN); }
unsigned int gpio_pull    (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPPUPPDN0, 2, GPIO_MAX_PIN); }
unsigned int gpio_function(unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPFSEL0, 3, GPIO_MAX_PIN); }

void gpio_useAsAlt3(unsigned int pin_number) {
    gpio_pull(pin_number, Pull_None);
    gpio_function(pin_number, GPIO_FUNCTION_ALT3);
}

void gpio_useAsAlt5(unsigned int pin_number) {
    gpio_pull(pin_number, Pull_None);
    gpio_function(pin_number, GPIO_FUNCTION_ALT5);
}

void gpio_initOutputPinWithPullNone(unsigned int pin_number) {
    gpio_pull(pin_number, Pull_None);
    gpio_function(pin_number, GPIO_FUNCTION_OUT);
}

void gpio_setPinOutputBool(unsigned int pin_number, unsigned int onOrOff) {
    if (onOrOff) {
        gpio_set(pin_number, 1);
    } else {
        gpio_clear(pin_number, 1);
    }
}

/* UART */

enum {
    AUX_BASE        = PERIPHERAL_BASE + 0x215000,
    AUX_IRQ         = AUX_BASE,
    AUX_ENABLES     = AUX_BASE + 4,
    AUX_MU_IO_REG   = AUX_BASE + 64,
    AUX_MU_IER_REG  = AUX_BASE + 68,
    AUX_MU_IIR_REG  = AUX_BASE + 72,
    AUX_MU_LCR_REG  = AUX_BASE + 76,
    AUX_MU_MCR_REG  = AUX_BASE + 80,
    AUX_MU_LSR_REG  = AUX_BASE + 84,
    AUX_MU_MSR_REG  = AUX_BASE + 88,
    AUX_MU_SCRATCH  = AUX_BASE + 92,
    AUX_MU_CNTL_REG = AUX_BASE + 96,
    AUX_MU_STAT_REG = AUX_BASE + 100,
    AUX_MU_BAUD_REG = AUX_BASE + 104,
    AUX_UART_CLOCK  = 500000000,
    UART_MAX_QUEUE  = 16 * 1024
};

#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK/(baud*8))-1)

unsigned char uart_output_queue[UART_MAX_QUEUE];
unsigned int uart_output_queue_write = 0;
unsigned int uart_output_queue_read = 0;

unsigned int uart_isReadByteReady()  { return mmio_read(AUX_MU_LSR_REG) & 0x01; }
unsigned int uart_isWriteByteReady() { return mmio_read(AUX_MU_LSR_REG) & 0x20; }

unsigned int uart_isOutputQueueEmpty() {
    return uart_output_queue_read == uart_output_queue_write;
}

void uart_putchar(uint8_t ch)
{
    xSemaphoreTake(uartctl->tx_mux, (portTickType) portMAX_DELAY);

    while (!uart_isWriteByteReady());
    mmio_write(AUX_MU_IO_REG, (unsigned int)ch);
    asm volatile ("isb");

    xSemaphoreGive(uartctl->tx_mux);
}

void uart_putchar_isr(uint8_t c)
{
    xSemaphoreTakeFromISR(uartctl->tx_mux, NULL);
    /* wait mini uart for tx idle. */
    while (!uart_isWriteByteReady());
    mmio_write(AUX_MU_IO_REG, (unsigned int)c);
    asm volatile ("isb");
    xSemaphoreGiveFromISR(uartctl->tx_mux, NULL);
}
/*----------------------------------------------------------*/

void uart_puts(const char* str)
{
    for (size_t i = 0; str[i] != '\0'; i ++)
        uart_putchar((uint8_t)str[i]);
}
/*-----------------------------------------------------------*/

void uart_puthex(uint64_t v)
{
    const char *hexdigits = "0123456789ABCDEF";
    for (int i = 60; i >= 0; i -= 4)
        uart_putchar(hexdigits[(v >> i) & 0xf]);
}
/*-----------------------------------------------------------*/

uint32_t uart_read_bytes(uint8_t *buf, uint32_t length)
{
    uint32_t num = uxQueueMessagesWaiting(uartctl->rx_queue);
    uint32_t i;

    for (i = 0; i < num || i < length; i++) {
        xQueueReceive(uartctl->rx_queue, &buf[i], (portTickType) portMAX_DELAY);
    }

    return i;
}
/*-----------------------------------------------------------*/

unsigned char is_uart_tx_ready(void)
{
	return !uart_isWriteByteReady();
}

unsigned char is_uart_rx_ready(void)
{
	return !uart_isReadByteReady();
}

unsigned char get_uart_char_from_chip(void)
{
        return (uint8_t) 0xFF & AUX_MU_IO_REG;
}

void put_uart_char_to_chip(uint8_t c)
{
	while (!uart_isWriteByteReady());
	mmio_write(AUX_MU_IO_REG, (unsigned int)c);
}

void uart_isr(void)
{
#if 0
    /* RX data */
    if( !(UART_FR & (0x1U << 4)) ) {
        uint8_t c = (uint8_t) 0xFF & UART_DR;
        xQueueSendToBackFromISR(uartctl->rx_queue, &c, NULL);
    }
#else
	 extern void vUSART_ISR( void );
        vUSART_ISR();
#endif
}
/*-----------------------------------------------------------*/

typedef void (*INTERRUPT_HANDLER) (void);
typedef struct {
        INTERRUPT_HANDLER fn;
} INT_VECTOR;

static INT_VECTOR g_vector_table[64];

#define IRQ_ENABLE_1            ((volatile uint32_t *)(0x3F00B210))
static void uart_isr_register(void (*fn)(void))
{
        g_vector_table[29].fn = fn;

        /* enable AUX miniuart rx interrupt */
//	mmio_write(AUX_MU_IIR_REG, 6);
	//mmio_write(AUX_MU_LCR_REG, (AUX_MU_LCR_REG & ~(1 << 7)));
	mmio_write(AUX_MU_IER_REG, 1);

        /* unmask AUX interrupt */
        *IRQ_ENABLE_1 = 1 << 29;
}

/* 
m
 * wait_linux()
 * This is a busy loop function to wait until Linux completes GIC initialization
 */
static void wait_linux(void)
{
    wait_gic_init();
    return;
}
/*-----------------------------------------------------------*/

void uart_init(void)
{
    mmio_write(AUX_ENABLES, 1); //enable UART1
    mmio_write(AUX_MU_CNTL_REG, 0);
    mmio_write(AUX_MU_IER_REG, 0xD);
    mmio_write(AUX_MU_LCR_REG, 3); //8 bits
    mmio_write(AUX_MU_MCR_REG, 0);
    //mmio_write(AUX_MU_IIR_REG, 0xC6); //disable interrupts
    mmio_write(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
    gpio_useAsAlt5(14);
    gpio_useAsAlt5(15);
    mmio_write(AUX_MU_CNTL_REG, 3); //enable RX/TX

    asm volatile ("isb");

    uartctl = pvPortMalloc(sizeof (struct UARTCTL));
    uartctl->tx_mux = xSemaphoreCreateMutex();
    uartctl->rx_queue = xQueueCreate(16, sizeof (uint8_t));

    isr_register(IRQ_VC_UART, UART_PRIORITY, (0x1U << 0x3U), uart_isr);
    //uart_isr_register(uart_isr);
    return;
}
/*-----------------------------------------------------------*/
