#include <stdio.h>

#include <stm32f10x.h>

#include "DAP_config.h"
#include "DAP.h"

//-----Soft reset + Hard reset-------------------------------------------------
#define PIN_SWCLK_SET PIN_SWCLK_TCK_SET
#define PIN_SWCLK_CLR PIN_SWCLK_TCK_CLR

#define RST_CLOCK_CYCLE() \
    PIN_SWCLK_CLR();      \
    PIN_DELAY();          \
    PIN_SWCLK_SET();      \
    PIN_DELAY()

#define RST_WRITE_BIT(bit) \
    PIN_SWDIO_OUT(bit);    \
    PIN_SWCLK_CLR();       \
    PIN_DELAY();           \
    PIN_SWCLK_SET();       \
    PIN_DELAY()

#define RST_READ_BIT(bit) \
    PIN_SWCLK_CLR();      \
    PIN_DELAY();          \
    bit = PIN_SWDIO_IN(); \
    PIN_SWCLK_SET();      \
    PIN_DELAY()

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

uint8_t RST_Transfer(uint32_t request, uint32_t data) {
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

        /* Packet Request */
    parity = 0U;
    RST_WRITE_BIT(1U); /* Start Bit */
    bit = request >> 0;
    RST_WRITE_BIT(bit); /* APnDP Bit */
    parity += bit;
    bit = request >> 1;
    RST_WRITE_BIT(bit); /* RnW Bit */
    parity += bit;
    bit = request >> 2;
    RST_WRITE_BIT(bit); /* A2 Bit */
    parity += bit;
    bit = request >> 3;
    RST_WRITE_BIT(bit); /* A3 Bit */
    parity += bit;
    RST_WRITE_BIT(parity); /* Parity Bit */
    RST_WRITE_BIT(0U);     /* Stop Bit */
    RST_WRITE_BIT(1U);     /* Park Bit */

        /* Turnaround */
    PIN_SWDIO_OUT_DISABLE();
    for (n = DAP_Data.swd_conf.turnaround; n; n--) {
        RST_CLOCK_CYCLE();
    }

    /* Acknowledge response */
    RST_READ_BIT(bit);
    ack = bit << 0;
    RST_READ_BIT(bit);
    ack |= bit << 1;
    RST_READ_BIT(bit);
    ack |= bit << 2;

    /* Data transfer */ /* Turnaround */
    for (n = DAP_Data.swd_conf.turnaround; n; n--) {
        RST_CLOCK_CYCLE();
    }
    PIN_SWDIO_OUT_ENABLE(); /* Write data */
    val    = data;
    parity = 0U;
    for (n = 32U; n; n--) {
        RST_WRITE_BIT(val); /* Write WDATA[0:31] */
        parity += val;
        val >>= 1;
    }
    RST_WRITE_BIT(parity); /* Write Parity Bit */
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t) ack);
}

void vResetTarget(uint8_t bit) {
    uint32_t i;
    // soft-reset for Cortex-M
    RST_Transfer(0x00000CC5, 0xE000ED0C);   // set AIRCR address
    for (i = 0; i < 100; i++)
        ;
    RST_Transfer(0x00000CDD, 0x05FA0007);   // set RESET data
    for (i = 0; i < 100; i++)
        ;
    RST_Transfer(0x00000CC5, 0xE000ED0C);   // repeat
    for (i = 0; i < 100; i++)
        ;
    RST_Transfer(0x00000CDD, 0x05FA0007);

    if (bit & 0x1) GPIO_OUTPUT_L(nRESET);
    else           GPIO_OUTPUT_H(nRESET);
}
