/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#ifndef GATT_SVR_H
#define GATT_SVR_H

/* Includes */
/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

/* NimBLE GAP APIs */
#include "host/ble_gap.h"

#define TEMP_SVC_EXP (-2)
#define TEMP_SVC_FLAG (0)

/* Public function declarations */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_subscribe_cb(struct ble_gap_event *event);
void gatt_svr_reset_temperature_subscription(void);
int gatt_svc_init(void);
void indicate_temperature(void);
#endif // GATT_SVR_H
