/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <dk_buttons_and_leds.h>

#define ADV_INTERVAL_MS 500
#define ADV_INTERVAL_MIN (ADV_INTERVAL_MS / 0.625)
#define ADV_INTERVAL_MAX (ADV_INTERVAL_MIN + 1)

#define USER_BUTTON DK_BTN1_MSK

 /* STEP 2.1 - Declare the Company identifier (Company ID) */
#define COMPANY_IDENTIFIER 0x0059 /* Nordic Semiconductor ASA */

/* STEP 2.2 - Declare the structure for your custom data  */
typedef struct
{
    uint16_t company_identifier;
    uint16_t number_press;          /* Number of button presses. */
} adv_mfg_data_t;

/* STEP 1 - Create an LE Advertising Parameters variable */
static struct bt_le_adv_param * p_adv_param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE,
                                                          ADV_INTERVAL_MIN,
                                                          ADV_INTERVAL_MAX,
                                                          NULL);

/* STEP 2.3 - Define and initialize a variable of type adv_mfg_data_type */
adv_mfg_data_t adv_mfg_data = {
    .company_identifier = COMPANY_IDENTIFIER,
    .number_press = 0,
};

static unsigned char url_data[] = { 0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm',
                    'y',  '.', 'n', 'o', 'r', 'd', 'i', 'c', 's',
                    'e',  'm', 'i', '.', 'c', 'o', 'm' };
LOG_MODULE_REGISTER(Lesson2_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    /* STEP 3 - Include the Manufacturer Specific Data in the advertising packet. */
    BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};
/* STEP 5 - Add the definition of callback function and update the advertising data dynamically */
void button_handler(uint32_t button_state, uint32_t has_changed)
{
    int err;
    if (has_changed & USER_BUTTON)
    {
        if (button_state & USER_BUTTON)
        {
            adv_mfg_data.number_press++;
            /* STEP 5.1 - Update the advertising data */
            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
            if (err)
            {
                LOG_ERR("Advertising failed to start (err %d)\n", err);
                return;
            }
        }
    }
}

/* STEP 4.1 - Define the initialization function of the buttons and setup interrupt.  */
static int init_button(void)
{
    int err;

    err = dk_buttons_init(button_handler);
    if (err)
    {
        LOG_ERR("Cannot init buttons (err %d)\n", err);
    }
    return err;
}



void main(void)
{
    int blink_status = 0;
    int err;

    LOG_INF("Starting Lesson 2 - Exercise 2 \n");

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("LEDs init failed (err %d)\n", err);
        return;
    }
    /* STEP 4.2 - Setup buttons on your board  */
    err = init_button();
    if (err)
    {
        LOG_ERR("Buttons init failed (err %d)\n", err);
        return;
    }

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return;
    }

    LOG_INF("Bluetooth initialized\n");

    err = bt_le_adv_start(p_adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return;
    }

    LOG_INF("Advertising successfully started\n");

    for (;;)
    {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}
