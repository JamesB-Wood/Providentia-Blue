#include "json_uart.h"
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

/* ENSURE FOLLOWING SET IN K-CONFIG 
    CONFIG_SERIAL=y
    CONFIG_UART_INTERRUPT_DRIVEN=y
    CONFIG_JSON_LIBRARY=y
*/

// UART interrupt MSGQ setup
#define MSG_SIZE 128

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 16, 1);

static int rx_buf_pos = 0;
static uint8_t rx_buf[MSG_SIZE] = {0};

// sensor data queue
char sensor_queue_buffer[20 * sizeof(sensor_data_t)];
struct k_msgq sensor_queue;

static const struct device *const my_uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

// JSON Descriptor for sensor data
struct json_obj_descr sens_data_descr[] = {
    JSON_OBJ_DESCR_PRIM(sensor_data_t, tvoc, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, co2, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, humidity, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(sensor_data_t, temp, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_ARRAY_NAMED(sensor_data_t, "ir_grid", ir_grid.ir_grid_data, 64, ir_grid.grid_data_len, JSON_TOK_NUMBER),
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
    char buff[512];
    data.ir_grid.grid_data_len = 64;
    json_obj_encode_buf(sens_data_descr, 5, &data, buff, sizeof(buff));
    send_uart_bytes(buff, strlen(buff));
    uart_poll_out(my_uart, '\r');
    uart_poll_out(my_uart, '\n');
}

void send_json_thread(void){
    k_msgq_init(&sensor_queue, sensor_queue_buffer, sizeof(sensor_data_t),
                20);

    sensor_data_t data_buff;

    while (k_msgq_get(&sensor_queue, &data_buff, K_FOREVER) == 0) {
        send_sensor_data(data_buff);
    }
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
        struct bt_data ad[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
                        0x02, 0x15,                            /* iBeacon */
                        0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
                        0x01, 0x6b,                            /* UUID[11..10] */
                        0x4b, 0xec,                            /* UUID[9..8] */
                        0xad, 0x96,                            /* UUID[7..6] */
                        0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
                        0x21, control_data.brightness,                            /* Major */
                        control_data.openness, control_data.flashing,                            /* Minor */
                        IBEACON_RSSI) /* Calibrated RSSI @ 1m */
        };

        bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
    }

    return 0;
}
