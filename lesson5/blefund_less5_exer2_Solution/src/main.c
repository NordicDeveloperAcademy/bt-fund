/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "lbs.h"

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

#define USER_LED                DK_LED3

#define USER_BUTTON             DK_BTN1_MSK

// Step 3.1.1 add an extra button for bond deleting function

#define BOND_DELETE_BUTTON             DK_BTN2_MSK

// Step 5.2 define an extra button for "pairing mode"

#define PAIRING_BUTTON             DK_BTN3_MSK

// Step 4.2 Add new advertising configurations when Accept List is used and when Accept List is empty

#define BT_LE_ADV_CONN_ACCEPT_LIST BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_FILTER_CONN|BT_LE_ADV_OPT_ONE_TIME, \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2, NULL)
#define BT_LE_ADV_CONN_NO_ACCEPT_LIST BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_ONE_TIME, \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2, NULL)


static bool app_button_state;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};
// Step 4.3 Add the code that loops through bond list to add the addresses to the list
static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;

	if ((*bond_cnt) < 0) {
		return;
	}

	int err = bt_le_filter_accept_list_add(&info->addr);
	printk("Added following peer to accept list: %x %x\n",info->addr.a.val[0],info->addr.a.val[1]);
	if (err) {
		printk("Cannot add peer to filter accept list (err: %d)\n", err);
		(*bond_cnt) = -EIO;
	} else {
		(*bond_cnt)++;
	}
}

static int setup_accept_list(uint8_t local_id)
{
	int err = bt_le_filter_accept_list_clear();

	if (err) {
		printk("Cannot clear accept list (err: %d)\n", err);
		return err;
	}

	int bond_cnt = 0;

	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

	return bond_cnt;
}

/* Step 4.4.1 Make a new function to start advertising which executes setup_accept_list() to build the list,
 and start advertising with accordingly parameters.*/

void advertise_with_acceptlist (struct k_work *work)
{
	int err=0;
	int allowed_cnt= setup_accept_list(BT_ID_DEFAULT);
	if (allowed_cnt<0){
		printk("Acceptlist setup failed (err:%d)\n", allowed_cnt);
	} else {
		if (allowed_cnt==0){
			printk("Advertising with no Accept list \n"); 
			err = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
					sd, ARRAY_SIZE(sd));
		}
		else {
			printk("Acceptlist setup number  = %d \n",allowed_cnt);
			err = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
				sd, ARRAY_SIZE(sd));	
		}
		if (err) {
		 	printk("Advertising failed to start (err %d)\n", err);
			return;
		}
		printk("Advertising successfully started\n");
	}
}
K_WORK_DEFINE(advertise_acceptlist_work, advertise_with_acceptlist);
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	dk_set_led_off(CON_STATUS_LED);
	k_work_submit(&advertise_acceptlist_work);

}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
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
	.led_cb    = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;

		bt_lbs_send_button_state(user_button_state);
		app_button_state = user_button_state ? true : false;
	}
// Step 3.1.2 add an extra button handling to remove bond information
	if (has_changed & BOND_DELETE_BUTTON) {
		uint32_t bond_delete_button_state = button_state & BOND_DELETE_BUTTON;
		if (bond_delete_button_state==0) {
			int err= bt_unpair(BT_ID_DEFAULT,BT_ADDR_LE_ANY);
			if (err) {
				printk("Cannot delete bond (err: %d)\n", err);
			} else	{
				printk("Bond deleted succesfully");
			}				
			
		}
	}
// Step 5.2 Add code to advertise without using accept list. 

	if (has_changed & PAIRING_BUTTON) {
		uint32_t pairing_button_state = button_state & PAIRING_BUTTON;
		if (pairing_button_state==0) {
			int err_code = bt_le_adv_stop();
			if (err_code) {
				printk("Cannot stop advertising err= %d \n", err_code);
				return;
			}
			err_code = bt_le_filter_accept_list_clear();
			if (err_code) {
				printk("Cannot clear accept list (err: %d)\n", err_code);
			} else	{
				printk("Accept list cleared succesfully");
			}				
			err_code = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
					sd, ARRAY_SIZE(sd));
			if (err_code) {
				printk("Cannot start open advertising (err: %d)\n", err_code);
			} else	{
				printk("Advertising in pairing mode started");

			}	
		}	
	}
}


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

	printk("Starting Bluetooth Peripheral LBS example\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	// Step 2.2 add setting load function 
	settings_load();


	err = bt_lbs_init(&lbs_callbacs);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}

// Step 4.4.2 submit advertise_acceptlist_work in main()
	k_work_submit(&advertise_acceptlist_work);

// Step 4.4.3 remove the original code that does normal advertising 
	// err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
	// 		      sd, ARRAY_SIZE(sd));
	// if (err) {
	// 	printk("Advertising failed to start (err %d)\n", err);
	// 	return;
	// }
	//printk("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
