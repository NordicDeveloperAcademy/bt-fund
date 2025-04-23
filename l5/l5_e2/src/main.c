/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <dk_buttons_and_leds.h>

#include "lbs.h"



LOG_MODULE_REGISTER(Lesson5_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define USER_LED DK_LED3

#define USER_BUTTON DK_BTN1_MSK

/* STEP 1.2 - Add the header file for the Settings module */

/* STEP 2.1 - Add extra button for bond deleting function */

/* STEP 4.2.1 - Add extra button for enabling pairing mode */

/* STEP 3.2.2 - Define advertising parameter for when Accept List is used */

static bool app_button_state;
static struct k_work adv_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};



/* STEP 3.3.1 - Define the callback to add addreses to the Accept List */

/* STEP 3.3.2 - Define the function to loop through the bond list */

/* STEP 3.4.1 - Define the function to advertise with the Accept List */

static void adv_work_handler(struct k_work *work)
{
	int err;
/* STEP 4.2.3 Add extra code to advertise without using Accept List when pairing_mode is set to true */

/* STEP 3.4.3 - Remove the original advertising code*/
	
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_INF("Advertising failed to start (err %d)\n", err);
		return;
	}
	LOG_INF("Advertising successfully started\n");
/* STEP 3.4.2 - Start advertising with the Accept List */
	
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	LOG_INF("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
	dk_set_led_off(CON_STATUS_LED);
}

static void recycled_cb(void)
{
	LOG_INF("Connection object available from previous conn. Disconnect/stop advertising is completed!\n");
	advertising_start();
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u\n", addr, level);
	} else {
		LOG_INF("Security failed: %s level %u err %d\n", addr, level, err);
	}
}
struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled         = recycled_cb,
	.security_changed = on_security_changed,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static void app_led_cb(bool led_state)
{
	dk_set_led(USER_LED, led_state);
}

static bool app_button_cb(void)
{
	return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
	.led_cb = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;

		bt_lbs_send_button_state(user_button_state);
		app_button_state = user_button_state ? true : false;
	}
	/* STEP 2.2 - Add extra button handling to remove bond information */

	/* STEP 4.2.2 Add extra button handling pairing mode (advertise without using Accept List) */
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_INF("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Bluetooth Peripheral LBS example\n");

	err = dk_leds_init();
	if (err) {
		LOG_INF("LEDs init failed (err %d)\n", err);
		return -1;
	}

	err = init_button();
	if (err) {
		LOG_INF("Button init failed (err %d)\n", err);
		return -1;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_INF("Failed to register authorization callbacks.\n");
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");

	/* STEP 1.3 - Add setting load function */

	err = bt_lbs_init(&lbs_callbacs);
	if (err) {
		LOG_INF("Failed to init LBS (err:%d)\n", err);
		return -1;
	}
	/* STEP 3.4.2 - Start advertising with the Accept List */

	/* STEP 3.4.3 - Remove the original code that does normal advertising */

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();


	LOG_INF("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
