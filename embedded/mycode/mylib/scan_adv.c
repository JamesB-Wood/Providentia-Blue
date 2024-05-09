#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <json_uart.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define NODE_THINGY "EF:B9:B8:80:14:B7"

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static struct bt_conn *default_conn;

sensor_data_t sensor_data;

static struct sensor_value temp_value[64];

const struct device *const dev = DEVICE_DT_GET_ONE(panasonic_amg88xx);

int ret;

/*
 * Set iBeacon advertisement data.
 *
 * UUID:  18ee1516-016b-4bec-ad96-bcb96d166e97
 * Major: 0
 * Minor: 0
 * RSSI:  -56 dBm
 */
struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
                  0x02, 0x15,                            /* iBeacon */
                  0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
                  0x01, 0x6b,                            /* UUID[11..10] */
                  0x4b, 0xec,                            /* UUID[9..8] */
                  0xad, 0x96,                            /* UUID[7..6] */
                  0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
                  0x21, 0x00,                            /* Major */
                  0x00, 0x00,                            /* Minor */
                  IBEACON_RSSI) /* Calibrated RSSI @ 1m */
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad_rev)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (strncmp(addr_str, NODE_THINGY, strlen(NODE_THINGY)) == 0) {

		if (ad_rev->data[25] == 0x63) {

			sensor_data.co2 = (ad_rev->data[27] << 8) | ad_rev->data[28];

		} else if (ad_rev->data[25] == 0x54) {

			sensor_data.tvoc = (ad_rev->data[27] << 8) | ad_rev->data[28];

		} else if (ad_rev->data[25] == 0x74) {

			sensor_data.temp = (ad_rev->data[27] << 8) | ad_rev->data[28];

		} else if (ad_rev->data[25] == 0x68) {

			sensor_data.humidity = (ad_rev->data[27] << 8) | ad_rev->data[28];

		}

		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("Failed to fetch a sample, %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 (struct sensor_value *)temp_value);
		if (ret) {
			printk("Failed to get sensor values, %d\n", ret);
			return;
		}

		for (int i = 0; i < 64; i++) {
			sensor_data.ir_grid.ir_grid_data[i] = (temp_value[i].val1 * 100 + temp_value[i].val2 / 10000);
		}

		k_msgq_put(&sensor_queue, &sensor_data, K_NO_WAIT);
	}
}

int scan_adv_thread(void)
{
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

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return 0;
	}

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
					NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	return 0;
}
