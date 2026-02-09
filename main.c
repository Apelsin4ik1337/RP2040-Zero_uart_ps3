#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define PS3_UART          uart0
#define PS3_UART_IRQ      UART0_IRQ
#define PS3_TX_PIN        0
#define PS3_RX_PIN        1
#define PS3_BAUD_RATE     115200

#define LED_PIN           16
#define RING_BUF_SIZE     4096

typedef struct {
    uint8_t  data[RING_BUF_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} ring_buf_t;

static ring_buf_t uart_rx_buf;

static inline void ring_buf_init(ring_buf_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

static inline bool ring_buf_is_empty(ring_buf_t *rb) {
    return rb->head == rb->tail;
}

static inline bool ring_buf_is_full(ring_buf_t *rb) {
    return ((rb->head + 1) % RING_BUF_SIZE) == rb->tail;
}

static inline uint32_t ring_buf_available(ring_buf_t *rb) {
    return (rb->head - rb->tail + RING_BUF_SIZE) % RING_BUF_SIZE;
}

static inline bool ring_buf_put(ring_buf_t *rb, uint8_t byte) {
    if (ring_buf_is_full(rb)) return false;
    rb->data[rb->head] = byte;
    rb->head = (rb->head + 1) % RING_BUF_SIZE;
    return true;
}

static inline bool ring_buf_get(ring_buf_t *rb, uint8_t *byte) {
    if (ring_buf_is_empty(rb)) return false;
    *byte = rb->data[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUF_SIZE;
    return true;
}

static volatile uint32_t last_activity_ms = 0;

static void on_uart_rx(void) {
    while (uart_is_readable(PS3_UART)) {
        uint8_t ch = uart_getc(PS3_UART);
        ring_buf_put(&uart_rx_buf, ch);
        last_activity_ms = to_ms_since_boot(get_absolute_time());
    }
}

static void uart_bridge_init(void) {
    uart_init(PS3_UART, PS3_BAUD_RATE);

    gpio_set_function(PS3_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PS3_RX_PIN, GPIO_FUNC_UART);
    gpio_pull_up(PS3_RX_PIN);

    uart_set_format(PS3_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(PS3_UART, true);
    uart_set_hw_flow(PS3_UART, false, false);

    irq_set_exclusive_handler(PS3_UART_IRQ, on_uart_rx);
    irq_set_enabled(PS3_UART_IRQ, true);
    uart_set_irq_enables(PS3_UART, true, false);

    ring_buf_init(&uart_rx_buf);
}

static void uart_to_usb_task(void) {
    if (!tud_cdc_connected()) return;

    uint32_t available = ring_buf_available(&uart_rx_buf);
    if (available == 0) return;

    uint32_t usb_space = tud_cdc_write_available();
    if (usb_space == 0) return;

    uint32_t to_send = (available < usb_space) ? available : usb_space;
    if (to_send > 64) to_send = 64;

    uint8_t buf[64];
    uint32_t count = 0;

    for (uint32_t i = 0; i < to_send; i++) {
        uint8_t byte;
        if (ring_buf_get(&uart_rx_buf, &byte)) {
            buf[count++] = byte;
        } else {
            break;
        }
    }

    if (count > 0) {
        tud_cdc_write(buf, count);
        tud_cdc_write_flush();
    }
}

static void usb_to_uart_task(void) {
    if (!tud_cdc_connected()) return;
    if (!tud_cdc_available()) return;

    uint8_t buf[64];
    uint32_t count = tud_cdc_read(buf, sizeof(buf));

    if (count > 0) {
        uart_write_blocking(PS3_UART, buf, count);
        last_activity_ms = to_ms_since_boot(get_absolute_time());
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    uint32_t baud = p_line_coding->bit_rate;
    if (baud > 0 && baud <= 921600) {
        uart_set_baudrate(PS3_UART, baud);
    }
}

static uint32_t led_update_ms = 0;
static uint8_t led_phase = 0;

static void led_task(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - led_update_ms < 100) return;
    led_update_ms = now;

    if (now - last_activity_ms < 200) {
        led_phase ^= 1;
        gpio_put(LED_PIN, led_phase & 1);
    } else if (tud_cdc_connected()) {
        gpio_put(LED_PIN, 1);
    } else {
        led_phase++;
        gpio_put(LED_PIN, (led_phase & 0x04) != 0);
    }
}

static bool banner_sent = false;

static void send_banner(void) {
    if (banner_sent) return;
    if (!tud_cdc_connected()) return;

    const char *banner =
        "\r\n"
        "========================================\r\n"
        "  PS3 SC UART Bridge - RP2040-Zero\r\n"
        "  UART: GP0(TX) GP1(RX) @ 115200/57600 8N1\r\n"
        "  Single COM port: read + write\r\n"
        "========================================\r\n"
        "\r\n";

    tud_cdc_write(banner, strlen(banner));
    tud_cdc_write_flush();
    banner_sent = true;
}

int main(void) {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    tusb_init();
    uart_bridge_init();

    while (true) {
        tud_task();
        send_banner();
        uart_to_usb_task();
        usb_to_uart_task();
        led_task();
    }

    return 0;
}