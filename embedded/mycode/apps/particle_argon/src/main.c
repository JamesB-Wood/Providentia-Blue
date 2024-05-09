#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/drivers/i2c.h>

#define NODE_BASE "F6:F2:9E:CA:3A:DE"

#define CRICKIT_SERVO 17
#define CRICKIT_DUTY_CYCLE_MAX 65535
#define CRICKIT_ADDR 0x49
#define CRICKIT_TIMER_FREQ 0x02
#define CRICKIT_MIN_PULSE 3277
#define CRICKIT_MAX_PULSE 6554

static struct bt_conn *default_conn;

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (strncmp(addr_str, NODE_BASE, strlen(NODE_BASE)) == 0) {
		// // Majors: ad->data[25], ad->data[26]
		// // Minors: ad->data[27], ad->data[28]
		// printk("ad->data[25]: %u\n", ad->data[25]);
	
		// printk("ad->data[26]: %u\n", ad->data[26]);

		// // Servo
		// printk("ad->data[27]: %u\n", ad->data[27]);

		// // true or false
		// printk("ad->data[28]: %u\n", ad->data[28]);
	}
}

int main(void)
{
	if (usb_enable(NULL)) {
		return 0;
	}

	int error;

	if (!device_is_ready(i2c_dev)) {
		printk("I2C device is not ready\n");
		return 0;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		printk("I2C config failed\n");
		return 0;
	}

	uint8_t init_msg[2] = {0x00, 0x01};

	uint8_t feq_msg[5] = {0x08, 0x02, 0x03, 0x00, 0x32};

	uint16_t pwm_val = 1000;
	uint8_t pwm_msg[5] = {0x08, 0x01, 0x03, (uint8_t)(pwm_val >> 8), (uint8_t)pwm_val};

	error = i2c_write(i2c_dev, feq_msg, 0, CRICKIT_ADDR);
	error = i2c_write(i2c_dev, feq_msg, 5, CRICKIT_ADDR);

	while(true) {

		error = i2c_write(i2c_dev, pwm_msg, 0, CRICKIT_ADDR);
		error = i2c_write(i2c_dev, pwm_msg, 5, CRICKIT_ADDR);
		printk("I2C error: %d\n", error);
		k_msleep(1000);

	}

	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0010,
		.window     = 0x0010,
	};
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return 0;
	}
}