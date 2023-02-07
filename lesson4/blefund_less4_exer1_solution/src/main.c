/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>


#include <dk_buttons_and_leds.h>
/* STEP 7 - Include the header file of MY LBS customer service */
#include "my_lbs.h"

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
                800, /*Min Advertising Interval 500ms (800*0.625ms) */
                801, /*Max Advertising Interval 500.625ms (801*0.625ms)*/
                NULL); /* Set to NULL for undirected advertising*/


LOG_MODULE_REGISTER(Lesson4_Exercise1, LOG_LEVEL_INF);

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2

/* STEP 8.1 - Specify the LED to control */
#define USER_LED                DK_LED3

/* STEP 9.1 - Specify the button to monitor */
#define USER_BUTTON             DK_BTN1_MSK

#define RUN_LED_BLINK_INTERVAL  1000

static bool app_button_state;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

/* STEP 8.2 - Define the application callback function for controlling the LED*/
static void app_led_cb(bool led_state)
{
	dk_set_led(USER_LED, led_state);
}

/* STEP 9.2 - Define the application callback function for reading the state of the button */
static bool app_button_cb(void)
{
	return app_button_state;
}

/* STEP 10 - Declare a varaible app_callbacks of type my_lbs_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2 .*/
static struct my_lbs_cb app_callbacks = {
	.led_cb    = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;
		app_button_state = user_button_state ? true : false;
	}
}
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
}

struct bt_conn_cb connection_callbacks = {
    .connected              = on_connected,
    .disconnected           = on_disconnected,  
};

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

void main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 4 - Exercise 1 \n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
    bt_conn_cb_register(&connection_callbacks);

	/* STEP 11 - Pass your application callback functions stored in app_callbacks to the MY LBS service */
	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
