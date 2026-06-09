#include "common.h"
#include "gap.h"
#include "gatt_svc.h"
#include "temperature.h"
#include "driver/temperature_sensor.h"

/* Private function declarations */
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);

void ble_store_config_init(void);


void nimble_host_stack(void *param) {

            /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}


static void nimble_host_config_init(void) {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

void app_main(void)
{
    BaseType_t rc = 0;
    esp_err_t ret;

    ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES 
    || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialise nvs flash, error code: %d", ret);
        return;
    }

    ret = temperature_init();

    if(ret != ESP_OK) {
        ESP_LOGE(TAG,"Temperature sensor init failed");
        return;
    }

    ret = nimble_port_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Nimble init failed with error code: %d", ret);
        return;
    }

#if CONFIG_BT_NIMBLE_GAP_SERVICE
    rc = gap_init();
    if(rc != ESP_OK) {
        ESP_LOGE(TAG, "Gap service init failed with error code %d", rc);
        return;
    }
#endif

    rc = gatt_svc_init();
    if(rc != ESP_OK) {
        ESP_LOGE(TAG, "Gatt service init failed with error code: %d", rc);
        return;
    }

    nimble_host_config_init();

    rc = xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);

    if(rc != pdPASS) {
        ESP_LOGE(TAG, "Failed to create NimBLE host task");
        return;
    }

    rc = xTaskCreate(temperature_task, "Temperature", 1024, NULL, 5, NULL);

    if(rc != pdPASS) {
        ESP_LOGE(TAG, "Failed to create temperature task");
        return;
    }

    return;
}
