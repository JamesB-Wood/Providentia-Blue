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

static struct bt_conn *default_conn;

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (strncmp(addr_str, NODE_BASE, strlen(NODE_BASE)) == 0) {
		// Majors: ad->data[25], ad->data[26]
		// Minors: ad->data[27], ad->data[28]
		printk("ad->data[25]: %u\n", ad->data[25]);
	
		printk("ad->data[26]: %u\n", ad->data[26]);

		// Servo
		printk("ad->data[27]: %u\n", ad->data[27]);

		// true or false
		printk("ad->data[28]: %u\n", ad->data[28]);
	}
}

int main(void)
{
	if (usb_enable(NULL)) {
		return 0;
	}

	printk("The I2C scanner started\n");
    // const struct device *i2c_dev;
    int error;

    // i2c_dev = device_get_binding("I2C_1");
    if (!i2c_dev) {
        printk("Binding failed.");
        return 0;
    }

    /* Demonstration of runtime configuration */
    i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
    printk("Value of NRF_TWIM2->PSEL.SCL : %d \n",NRF_TWIM1->PSEL.SCL);
    printk("Value of NRF_TWIM2->PSEL.SDA : %d \n",NRF_TWIM1->PSEL.SDA);
    printk("Value of NRF_TWIM2->FREQUENCY: %d \n",NRF_TWIM1->FREQUENCY);
    printk("26738688 -> 100k\n");

    // for (uint8_t i = 4; i <= 0x7F; i++) {
    //     struct i2c_msg msgs[1];
    //     uint8_t dst = 1;

    //     msgs[0].buf = &dst;
    //     msgs[0].len = 1U;
    //     msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    //     error = i2c_transfer(i2c_dev, &msgs[0], 1, i);
    //     if (error == 0) {
    //         printk("0x%2x FOUND\n", i);
    //     }
    //     else {
    //         //printk("error %d \n", error);
    //     }

    // }

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