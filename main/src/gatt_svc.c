/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"
#include "temperature.h"
#include "host/ble_gatt.h"

/* Private function declarations */
// static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                                  struct ble_gatt_access_ctxt *ctxt, void *arg);
// static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

static int temperature_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);
static int8_t temp_payload[5] = {0};
/* Private variables */

// Temperature Service
static const ble_uuid16_t temperature_svc_uuid = BLE_UUID16_INIT(0x1809);
static uint16_t temperature_chr_val_handle;
static const ble_uuid16_t temperature_chr_uuid = BLE_UUID16_INIT(0x2A1C);

static uint16_t temperature_chr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool temperature_chr_conn_handle_inited = false;
static bool temperature_ind_status = false;

static int
gatt_svr_dsc_cccd_access(uint16_t conn_handle, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        // The phone is checking its subscription status.
        // Return 0 to allow NimBLE to send the status back automatically.
        return 0; 
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_DSC) {
        // CRITICAL SANITY CHECK: A BLE subscription change MUST be exactly 2 bytes.
        // (0x0001 for Notify, 0x0002 for Indicate, 0x0000 for Off)
        if (OS_MBUF_PKTLEN(ctxt->om) != 2) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN; // Reject bad data lengths
        }
        
        // Return 0 to tell NimBLE: "Data length is valid, go ahead and save it."
        return 0; 
    }

    // Fallback error if the stack attempts an unsupported operation
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // Temperature Service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &temperature_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
              .uuid = &temperature_chr_uuid.u,
              .access_cb = temperature_chr_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &temperature_chr_val_handle,

              .descriptors = (struct ble_gatt_dsc_def[]) { {
                /*** CCCD Descriptor (Allows phone to toggle subscription) ***/
                    .uuid = BLE_UUID16_DECLARE(0x2902),
                    .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE,
                    .access_cb = gatt_svr_dsc_cccd_access, // The custom helper we built
                }, {
                    0 // Descriptor Sentinel
                } }
            },
            {
                0,
            }
        }
    }, 
    {
        0,
    },
};

static int load_temperature(int8_t* out_payload, int8_t flag) {
    if(out_payload == NULL) {
        return ERR_NULL_PTR;
    }

    int rc = 0;

    int32_t temp = 0;
    rc = get_temperature(&temp);
    
    if(rc != OK) {
        return ERR_SENSOR_FAULT;
    }

    out_payload[0] = flag;
    out_payload[1] = BYTE_0(temp);
    out_payload[2] = BYTE_1(temp);
    out_payload[3] = BYTE_2(temp);
    out_payload[4] = TEMP_SVC_EXP;
    rc = OK;

    return rc;
}

static int temperature_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc = 0;

    switch(ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            } else {
                ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                        attr_handle);
            }

            if (attr_handle == temperature_chr_val_handle) {
                /* Update access buffer value */
                rc = load_temperature(temp_payload, TEMP_SVC_FLAG);
                if(rc != OK) {
                    ESP_LOGE(TAG, "Temp reading failed with error %d", rc);
                    goto error;
                }
              
                rc = os_mbuf_append(ctxt->om, temp_payload,
                                    sizeof(temp_payload));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            goto error;

        default:
            goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to heart rate characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    if(event == NULL) {
        return;
    }

    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    if(event->subscribe.attr_handle == temperature_chr_val_handle) {
        temperature_chr_conn_handle = event->subscribe.conn_handle;
        temperature_chr_conn_handle_inited = true;
        temperature_ind_status = event->subscribe.cur_indicate;
    }
}

void gatt_svr_reset_temperature_subscription(void) {
    temperature_chr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    temperature_chr_conn_handle_inited = false;
    temperature_ind_status = false;
} 

void indicate_temperature() {
    if(temperature_ind_status && temperature_chr_conn_handle_inited) {
        ble_gatts_indicate(temperature_chr_conn_handle, temperature_chr_val_handle);
        ESP_LOGI(TAG, "Temperature indication sent!");
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc = 0;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
