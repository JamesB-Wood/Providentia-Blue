#include "scan_adv.h"
#include "json_uart.h"
#include <zephyr/kernel.h>

#define UART_PRIORITY 7
#define SCAN_ADV_PRIORITY 7
#define STACKSIZE 1024

K_THREAD_DEFINE(uart_thread_id, STACKSIZE, uart_thread, NULL, NULL, NULL,
                UART_PRIORITY, 0, 0);

K_THREAD_DEFINE(scan_adv_thread_id, STACKSIZE, scan_adv_thread, NULL, NULL, NULL,
                SCAN_ADV_PRIORITY, 0, 0);