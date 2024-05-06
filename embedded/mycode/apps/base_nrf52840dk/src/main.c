#include <zephyr/kernel.h>
#include "json_uart.h"
#include "scan_adv.h"

#define UART_PRIORITY 7
#define SCAN_ADV_PRIORITY 6
#define SEND_JSON_PRIORITY 7
#define STACKSIZE 2048

K_THREAD_DEFINE(uart_thread_id, STACKSIZE, uart_thread, NULL, NULL, NULL,
                UART_PRIORITY, 0, 0);

K_THREAD_DEFINE(scan_adv_thread_id, STACKSIZE, scan_adv_thread, NULL, NULL, NULL,
                SCAN_ADV_PRIORITY, 0, 0);

// K_THREAD_DEFINE(send_json_thread_id, STACKSIZE, send_json_thread, NULL, NULL, NULL,
//                 SEND_JSON_PRIORITY, 0, 0);