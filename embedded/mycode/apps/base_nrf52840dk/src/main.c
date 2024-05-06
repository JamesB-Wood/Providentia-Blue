#include "json_uart.h" // Include header file for JSON UART functionality
#include "scan_adv.h" // Include header file for scanning and advertising functionality
#include <zephyr/kernel.h> // Include Zephyr kernel header for threading

#define UART_PRIORITY 7      // Priority level for UART thread
#define SCAN_ADV_PRIORITY 6  // Priority level for scan/advertise thread
#define SEND_JSON_PRIORITY 7 // Priority level for JSON sending thread
#define STACKSIZE 2048       // Stack size for each thread

// Define a thread for UART handling
K_THREAD_DEFINE(uart_thread_id, STACKSIZE, uart_thread, NULL, NULL, NULL,
                UART_PRIORITY, 0, 0);

// Define a thread for scanning and advertising operations
K_THREAD_DEFINE(scan_adv_thread_id, STACKSIZE, scan_adv_thread, NULL, NULL,
                NULL, SCAN_ADV_PRIORITY, 0, 0);

// Define a thread for sending JSON data
K_THREAD_DEFINE(send_json_thread_id, STACKSIZE, send_json_thread, NULL, NULL,
                NULL, SEND_JSON_PRIORITY, 0, 0);