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

#define USER_BUTTON DK_BTN1_MSK
#define RUN_STATUS_LED DK_LED1
#define CONNECTION_STATUS_LED   DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

struct bt_conn *my_conn = NULL;
static struct k_work adv_work;

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONN |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	BT_GAP_ADV_FAST_INT_MIN_1, /* 0x30 units, 48 units, 30ms */
	BT_GAP_ADV_FAST_INT_MAX_1, /* 0x60 units, 96 units, 60ms */
	NULL); /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(Lesson3_Exercise2, LOG_LEVEL_INF);

/* STEP 11.2 - Create variable that holds callback for MTU negotiation */
static struct bt_gatt_exchange_params exchange_params;

/* STEP 13.4 - Forward declaration of exchange_func(): */
static void exchange_func(struct bt_conn *conn, uint8_t att_err, struct bt_gatt_exchange_params *params);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
			  BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
};

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

/* STEP 7.1 - Define the function to update the connection's PHY */
static void update_phy(struct bt_conn *conn)
{
	int err;
	const struct bt_conn_le_phy_param preferred_phy = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};
	err = bt_conn_le_phy_update(conn, &preferred_phy);
	if (err) {
		LOG_ERR("bt_conn_le_phy_update() returned %d", err);
	}
}

/* STEP 10 - Define the function to update the connection's data length */
static void update_data_length(struct bt_conn *conn)
{
	int err;
	struct bt_conn_le_data_len_param my_data_len = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};
	err = bt_conn_le_data_len_update(my_conn, &my_data_len);
	if (err) {
		LOG_ERR("data_len_update failed (err %d)", err);
	}
}

/* STEP 11.1 - Define the function to update the connection's MTU */
static void update_mtu(struct bt_conn *conn)
{
	int err;
	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}


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
    k_sleep(K_MSEC(100)); 
    
	/* STEP 1.1 - Declare a structure to store the connection parameters */
	struct bt_conn_info info;
	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}
	/* STEP 1.2 - Add the connection parameters to your log */
	double connection_interval = info.le.interval*1.25; // in ms
	uint16_t supervision_timeout = info.le.timeout*10; // in ms
	LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, info.le.latency, supervision_timeout);
	/* STEP 7.2 - Update the PHY mode */
	update_phy(my_conn);
	/* STEP 13.5 - Update the data length and MTU */
	update_data_length(my_conn);
	update_mtu(my_conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected. Reason %d", reason);
	dk_set_led(CONNECTION_STATUS_LED, 0);
	bt_conn_unref(my_conn);
}

void on_recycled(void)
{
	advertising_start();
}

/* STEP 4.2 - Add the callback for connection parameter updates */
void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
	double connection_interval = interval*1.25;         // in ms
	uint16_t supervision_timeout = timeout*10;          // in ms
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, latency, supervision_timeout);
}
/* STEP 8.1 - Write a callback function to inform about updates in the PHY */
void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	// PHY Updated
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	}
	else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	}
	else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}

/* STEP 13.1 - Write a callback function to inform about updates in data length */
void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
	uint16_t tx_len     = info->tx_max_len; 
	uint16_t tx_time    = info->tx_max_time;
	uint16_t rx_len     = info->rx_max_len;
	uint16_t rx_time    = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time, rx_time);
}

struct bt_conn_cb connection_callbacks = {
	.connected          = on_connected,
	.disconnected       = on_disconnected,
	.recycled           = on_recycled,
	/* STEP 4.1 - Add the callback for connection parameter updates */
	.le_param_updated   = on_le_param_updated,
	/* STEP 8.3 - Add the callback for PHY mode updates */
	.le_phy_updated     = on_le_phy_updated,
	/* STEP 13.2 - Add the callback for data length updates */
	.le_data_len_updated    = on_le_data_len_updated,
};

/* STEP 13.3 - Implement callback function for MTU exchange */
static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
	if (!att_err) {
		uint16_t payload_mtu = bt_gatt_get_mtu(conn) - 3;   // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	int err;
	bool user_button_changed = (has_changed & USER_BUTTON) ? true : false;
	bool user_button_pressed = (button_state & USER_BUTTON) ? true : false;
	if (user_button_changed) {
		LOG_INF("Button %s", (user_button_pressed ? "pressed" : "released"));

		err = bt_lbs_send_button_state(user_button_pressed);
		if (err) {
			LOG_ERR("Couldn't send notification. (err: %d)", err);
		}
	}
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	return err;
}

int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 3 - Exercise 2\n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return -1;
	}

	err = init_button();
	if (err) {
		LOG_ERR("Button init failed (err %d)", err);
		return -1;
	}

	err = bt_conn_cb_register(&connection_callbacks);
	if (err) {
		LOG_ERR("Connection callback register failed (err %d)", err);
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized");
	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
