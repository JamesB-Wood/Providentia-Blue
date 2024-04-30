#include "json_uart.h"
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

/* ENSURE FOLLOWING SET IN K-CONFIG 
    CONFIG_SERIAL=y
    CONFIG_UART_INTERRUPT_DRIVEN=y
    CONFIG_JSON_LIBRARY=y
*/

// UART interrupt MSGQ setup
#define MSG_SIZE 128

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 16, 1);

static int rx_buf_pos = 0;
static uint8_t rx_buf[MSG_SIZE] = {0};

static const struct device *const my_uart = DEVICE_DT_GET(DT_NODELABEL(usart2));

// JSON Descriptor for sensor data
struct json_obj_descr sens_data_descr[] = {
    JSON_OBJ_DESCR_PRIM(sensor_data_t, tvoc, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, co2, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, humidity, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, temp, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, lux, JSON_TOK_NUMBER),
};

// JSON Descriptor for control data
struct json_obj_descr contr_data_descr[] = {
    JSON_OBJ_DESCR_PRIM(control_data_t, brightness, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(control_data_t, openness, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(control_data_t, flashing, JSON_TOK_FALSE),
};

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void receive_uart_isr(const struct device *dev, void *user_data) {

    if (!uart_irq_update(my_uart) || !uart_irq_rx_ready(my_uart)) {
        return;
    }

    uint8_t c;

    // read until FIFO empty
    while (uart_fifo_read(my_uart, &c, 1) == 1) {

        // add to buffer
        rx_buf[rx_buf_pos++] = c;

        // Add to queue if last char was '}' assuming no nested relationships or
        // strings containing '}'
        if (c == '}') {
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

            // reset variables
            memset(rx_buf, 0, MSG_SIZE);
            rx_buf_pos = 0;
        }
    }
}

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

/*
 * Send the sensor data struct of UART
 *
 *  @param data The data to be sent over UART
 */
void send_sensor_data(sensor_data_t data) {
    char buff[256];
    json_obj_encode_buf(sens_data_descr, 5, &data, buff, sizeof(buff));
    send_uart_bytes(buff, strlen(buff)); // confirm this works
    uart_poll_out(my_uart, '\n');
}

/*
 * Main thread for UART reading
 */
int uart_thread(void) {

    // Initialisation
    if (!device_is_ready(my_uart)) {
        return 0;
    }
    int ret = uart_irq_callback_user_data_set(my_uart, receive_uart_isr, NULL);
    if (ret != 0) {
        return 0;
    }
    
    uart_irq_rx_enable(my_uart);

    uint8_t tx_buf[MSG_SIZE];
    control_data_t control_data;

    // wait till item in buffer
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        // Decode the returned json to 
        json_obj_parse(tx_buf, strlen(tx_buf), contr_data_descr, 3,
                       &control_data);

        //send data to Crikit Board...
    }

    return 0;
}
