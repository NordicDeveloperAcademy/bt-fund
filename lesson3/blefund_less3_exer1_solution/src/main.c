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
/* STEP 1 - Include the header file for managing Bluetooth LE Connections */
#include <zephyr/bluetooth/conn.h>
/* STEP 8.2 - Include the header file for the LED Button Service */
#include <bluetooth/services/lbs.h>

#include <dk_buttons_and_leds.h>

#define USER_BUTTON             DK_BTN1_MSK
#define RUN_STATUS_LED          DK_LED1
/* STEP 3.1 - Define an LED to show the connection status */
#define CONNECTION_STATUS_LED   DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

struct bt_conn *my_conn = NULL;


static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
                BT_GAP_ADV_FAST_INT_MIN_1, /* 0x30 units, 48 units, 30ms */
                BT_GAP_ADV_FAST_INT_MAX_1, /* 0x60 units, 96 units, 60ms */
                NULL); /* Set to NULL for undirected advertising*/


LOG_MODULE_REGISTER(Lesson3_Exercise1, LOG_LEVEL_INF);

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)



static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
};

/* STEP 2.2 - Implement the callback functions */
void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection error %d", err);
        return;
    }
    LOG_INF("Connected");
    my_conn = bt_conn_ref(conn);

	/* STEP 3.2  Turn the connection status LED on */
    dk_set_led(CONNECTION_STATUS_LED, 1);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected. Reason %d", reason);
    bt_conn_unref(my_conn);

	/* STEP 3.3  Turn the connection status LED off */
    dk_set_led(CONNECTION_STATUS_LED, 0);
}

/* STEP 2.1 - Declare the connection_callback structure */
struct bt_conn_cb connection_callbacks = {
    .connected              = on_connected,
    .disconnected           = on_disconnected,
};


/* STEP 8.3 - Send a notification using the LBS characteristic. */
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
/*STEP 8.4 - Complete the implementation of the init_button() function. */
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

	LOG_INF("Starting Lesson 3 - Exercise 1\n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return;
	}
	
	err = init_button();
	if (err) {
		LOG_INF("Button init failed (err %d)", err);
		return;
	}

   /* STEP 2.3 - Register our custom callbacks */
    bt_conn_cb_register(&connection_callbacks);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
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
