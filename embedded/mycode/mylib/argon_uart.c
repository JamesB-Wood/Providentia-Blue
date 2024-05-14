#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

static const struct device *const my_uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

/*
 * send data over uart
 *
 * @param buf array of bytes to be sent
 * @param len the length of the array to send
 */
void send_uart_bytes(uint8_t *buf, int len) {

    for (int i = 0; i < len; i++) {
        uart_poll_out(my_uart, buf[i]);
    }
}