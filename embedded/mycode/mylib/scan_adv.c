/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define NODE_THINGY "EF:B9:B8:80:14:B7"

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static struct bt_conn *default_conn;

/*
 * Set iBeacon demo advertisement data. These values are for
 * demonstration only and must be changed for production environments!
 *
 * UUID:  18ee1516-016b-4bec-ad96-bcb96d166e97
 * Major: 0
 * Minor: 0
 * RSSI:  -56 dBm
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
		      0x4c, 0x00, /* Apple */
		      0x02, 0x15, /* iBeacon */
		      0x18, 0xee, 0x15, 0x16, /* UUID[15..12] */
		      0x01, 0x6b, /* UUID[11..10] */
		      0x4b, 0xec, /* UUID[9..8] */
		      0xad, 0x96, /* UUID[7..6] */
		      0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97, /* UUID[5..0] */
		      0x00, 0x00, /* Major */
		      0x00, 0x00, /* Minor */
		      IBEACON_RSSI) /* Calibrated RSSI @ 1m */
};

void update_adv_data(void) {
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
					0x02, 0x15,                            /* iBeacon */
					0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
					0x01, 0x6b,                            /* UUID[11..10] */
					0x4b, 0xec,                            /* UUID[9..8] */
					0xad, 0x96,                            /* UUID[7..6] */
					0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
					0x41, 0x00,                            /* Major */
					0x00, 0x00,                            /* Minor */
					IBEACON_RSSI) /* Calibrated RSSI @ 1m */
	};

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
}

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

			printk("CO2 value: %u\n", ((ad_rev->data[27] << 8) | ad_rev->data[28]));

		} else if (ad_rev->data[25] == 0x54) {

			printk("TVOC value: %u\n", ((ad_rev->data[27] << 8) | ad_rev->data[28]));

		} else if (ad_rev->data[25] == 0x74) {

			printk("Temp value: %u\n", ((ad_rev->data[27] << 8) | ad_rev->data[28]));

            // uint16_t temp_raw = ((ad_rev->data[27] << 8) | ad_rev->data[28]);

            // double temperature = (double)(temp_raw / 0xFFFF);

            // printk("Temperature:%f C\n", temperature);

		} else if (ad_rev->data[25] == 0x68) {

			printk("Hum value: %u\n", ((ad_rev->data[27] << 8) | ad_rev->data[28]));

			// uint16_t hum_raw = ((ad_rev->data[27] << 8) | ad_rev->data[28]);

            // double humidity = (double)(hum_raw / 0xFFFF);

            // printk("Relative Humidity:%.1f%%\n", humidity);

		}
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

	printk("Bluetooth initialized\n");

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

	return 0;
}
