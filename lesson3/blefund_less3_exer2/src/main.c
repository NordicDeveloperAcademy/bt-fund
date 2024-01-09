/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>

#include <dk_buttons_and_leds.h>

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	BT_GAP_ADV_FAST_INT_MIN_1, /* 0x30 units, 48 units, 30ms */
	BT_GAP_ADV_FAST_INT_MAX_1, /* 0x60 units, 96 units, 60ms */
	NULL); /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(Lesson3_Exercise2, LOG_LEVEL_INF);
struct bt_conn *my_conn = NULL;

/* STEP 11.2 - Create variable that holds callback for MTU negotiation */

/* STEP 13.4 - Forward declaration of exchange_func(): */

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define USER_BUTTON DK_BTN1_MSK
#define RUN_STATUS_LED DK_LED1
#define CONNECTION_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
};

/* STEP 7.1 - Define the function to update the connection's PHY */

/* STEP 10 - Define the function to update the connection's data length */

/* STEP 11.1 - Define the function to update the connection's MTU */

/* Callbacks */
void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection error %d", err);
		return;
	}
	LOG_INF("Connected");
	my_conn = bt_conn_ref(conn);
	dk_set_led(CONNECTION_STATUS_LED, 1);
	/* STEP 1.1 - Declare a structure to store the connection parameters */

	/* STEP 1.2 - Add the connection parameters to your log */

	/* STEP 7.2 - Update the PHY mode */

	/* STEP 13.5 - Update the data length and MTU */
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected. Reason %d", reason);
	dk_set_led(CONNECTION_STATUS_LED, 0);
	bt_conn_unref(my_conn);
}

/* STEP 4.2 - Add the callback for connection parameter updates */

/* STEP 8.1 - Write a callback function to inform about updates in the PHY */

/* STEP 13.1 - Write a callback function to inform about updates in data length */

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	/* STEP 4.1 - Add the callback for connection parameter updates */
	/* STEP 8.3 - Add the callback for PHY mode updates */
	/* STEP 13.2 - Add the callback for data length updates */
};

/* STEP 13.3 - Implement callback function for MTU exchange */

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	int err;
	if (has_changed & USER_BUTTON) {
		LOG_INF("Button changed");
		err = bt_lbs_send_button_state(button_state ? true : false);
		if (err) {
			LOG_ERR("Couldn't send notification. err: %d", err);
		}
	}
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_INF("Cannot init buttons (err: %d)", err);
	}

	return err;
}

void main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 3 - Exercise 2\n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return;
	}

	err = init_button();
	if (err) {
		LOG_ERR("Button init failed (err %d)", err);
		return;
	}

	bt_conn_cb_register(&connection_callbacks);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
