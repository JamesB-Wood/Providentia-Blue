#ifndef JSON_UART_H
#define JSON_UART_H

#include <zephyr/kernel.h>

typedef struct {
    int tvoc; // in ppm
    int co2; // in ppm
    int humidity; // 1000x percentage e.g (50.5% --> 505)
    int temp; // 10x degrees c e.g (16.5c --> 165)
    int lux; //may need to change 
} sensor_data_t;

typedef struct {
    int brightness; // as a percent 0-100
    int openness; // as a percent 0-100
    bool flashing;
} control_data_t;

/*
 * Main thread for UART reading
 */
int uart_thread(void);

/*
 * Send the sensor data struct of UART 
 * 
 * @param data The data to be sent over UART
 */
void send_sensor_data(sensor_data_t data);

#endif