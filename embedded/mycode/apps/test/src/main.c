#include "json_uart.h"
#include <zephyr/kernel.h>


int main(){
    sensor_data_t data;
    data.tvoc = 1000;
    data.co2 = 2000;
    data.humidity = 505;
    data.temp = 165;
    int data_values[64] = {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
    memcpy(data.ir_grid.ir_grid_data, data_values, sizeof(data_values));
    send_sensor_data(data);
}