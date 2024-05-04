#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

/* The devicetree node identifier for the "led0" alias. */
#define LED1_NODE DT_ALIAS(led1)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static bool app_fw_2;

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

void update_co2_ibeacon(uint16_t co2) {

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
					0x02, 0x15,                            /* iBeacon */
					0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
					0x01, 0x6b,                            /* UUID[11..10] */
					0x4b, 0xec,                            /* UUID[9..8] */
					0xad, 0x96,                            /* UUID[7..6] */
					0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
					0x63, 0x00,                            /* Major */
					(co2 >> 8), (co2 & 0xFF),            /* Minor */
					IBEACON_RSSI) /* Calibrated RSSI @ 1m */
	};

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
}

void update_tvoc_ibeacon(uint16_t tvoc) {

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
					0x02, 0x15,                            /* iBeacon */
					0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
					0x01, 0x6b,                            /* UUID[11..10] */
					0x4b, 0xec,                            /* UUID[9..8] */
					0xad, 0x96,                            /* UUID[7..6] */
					0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
					0x54, 0x00,                            /* Major */
					(tvoc >> 8), (tvoc & 0xFF),            /* Minor */
					IBEACON_RSSI) /* Calibrated RSSI @ 1m */
	};

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
}

static int process_ccs811_sample(const struct device *dev)
{
	struct sensor_value co2, tvoc;
	int rc = 0;

	if (rc == 0) {
		rc = sensor_sample_fetch(dev);
	}

	if (rc == 0) {
		const struct ccs811_result_type *rp = ccs811_result(dev);

		sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2);
		sensor_channel_get(dev, SENSOR_CHAN_VOC, &tvoc);

		printk("\nCCS811: %u ppm eCO2; %u ppb eTVOC\n", co2.val1, tvoc.val1);

		update_tvoc_ibeacon((uint16_t)sensor_value_to_double(&tvoc));
		k_msleep(100);
		update_co2_ibeacon((uint16_t)sensor_value_to_double(&co2));

		if (app_fw_2 && !(rp->status & CCS811_STATUS_DATA_READY)) {
			printk("STALE DATA\n");
		}

		if (rp->status & CCS811_STATUS_ERROR) {
			printk("ERROR: %02x\n", rp->error);
		}
	}


	return rc;
}

void update_temp_ibeacon(uint16_t temp) {

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
					0x02, 0x15,                            /* iBeacon */
					0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
					0x01, 0x6b,                            /* UUID[11..10] */
					0x4b, 0xec,                            /* UUID[9..8] */
					0xad, 0x96,                            /* UUID[7..6] */
					0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
					0x74, 0x00,                            /* Major */
					(temp >> 8), (temp & 0xFF),            /* Minor */
					IBEACON_RSSI) /* Calibrated RSSI @ 1m */
	};

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
}

void update_hum_ibeacon(uint16_t hum) {

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
					0x02, 0x15,                            /* iBeacon */
					0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
					0x01, 0x6b,                            /* UUID[11..10] */
					0x4b, 0xec,                            /* UUID[9..8] */
					0xad, 0x96,                            /* UUID[7..6] */
					0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97,    /* UUID[5..0] */
					0x68, 0x00,                            /* Major */
					(hum >> 8), (hum & 0xFF),            /* Minor */
					IBEACON_RSSI) /* Calibrated RSSI @ 1m */
	};

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
}

static void process_hts221_sample(const struct device *dev)
{
	struct sensor_value temp, hum;
	if (sensor_sample_fetch(dev) < 0) {
		printk("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		printk("Cannot read HTS221 temperature channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
		printk("Cannot read HTS221 humidity channel\n");
		return;
	}

	double temp_double = sensor_value_to_double(&temp);
	double hum_double = sensor_value_to_double(&hum);

	/* display temperature */
	printk("Temperature:%.1f C\n", temp_double);

	/* display humidity */
	printk("Relative Humidity:%.1f%%\n", hum_double);

	update_temp_ibeacon((uint16_t)temp_double);
	k_msleep(100);
	update_hum_ibeacon((uint16_t)hum_double);
}

static void do_main(const struct device *dev, const struct device *dev1)
{
	while (true) {
		int rc = process_ccs811_sample(dev);
		k_msleep(100);
		process_hts221_sample(dev1);

		if (rc == 0) {
			printk("Timed fetch got %d\n", rc);
		} else if (-EAGAIN == rc) {
			printk("Timed fetch got stale data\n");
		} else {
			printk("Timed fetch failed: %d\n", rc);
			break;
		}
		k_msleep(100);
	}
}

void enable_sensors(void) {

	const struct device *const dev = DEVICE_DT_GET_ONE(ams_ccs811);
	struct ccs811_configver_type cfgver;
	int rc;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

	const struct device *const dev1 = DEVICE_DT_GET_ONE(st_hts221);

	if (!device_is_ready(dev1)) {
		printk("sensor: device not ready.\n");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	rc = ccs811_configver_fetch(dev, &cfgver);
	if (rc == 0) {
		printk("HW %02x; FW Boot %04x App %04x ; mode %02x\n",
		       cfgver.hw_version, cfgver.fw_boot_version,
		       cfgver.fw_app_version, cfgver.mode);
		app_fw_2 = (cfgver.fw_app_version >> 8) > 0x11;
	}

	if (rc == 0) {
		do_main(dev, dev1);
	}

}

void enable_gpio(void) {

	int ret;

    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("iBeacon started\n");
}

void enable_ibeacon(void) {

	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}

int main(void)
{
	enable_gpio();

	enable_ibeacon();

	enable_sensors();

	return 0;
}
