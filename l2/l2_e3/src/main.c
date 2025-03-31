/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
/* STEP 3.2.1 - Include the header file of the UUID helper macros and definitions */

/* STEP 4.1 - Include the header file for managing Bluetooth LE addresses */

#include <dk_buttons_and_leds.h>

/* STEP 5.1 - Create the advertising parameter for connectable advertising */

LOG_MODULE_REGISTER(Lesson2_Exercise3, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000
static struct k_work adv_work;
static const struct bt_data ad[] = {
	/* STEP 3.1 - Set the flags and populate the device name in the advertising packet */

};

static const struct bt_data sd[] = {
	/* STEP 3.2.2 - Include the 16-bytes (128-Bits) UUID of the LBS service in the scan response packet */

};
/* STEP 5.2 - Resume advertising after a disconnection */

int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 2 - Exercise 3 \n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}

	/* STEP 4.2 - Change the random static address */

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");
	/* STEP 5.3 - Start connectable advertising */

	LOG_INF("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
