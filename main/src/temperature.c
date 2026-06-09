#include "temperature.h"
#include "common.h"
#include "gatt_svc.h"

#define TAG_M "temperature_module"

temperature_sensor_handle_t temp_handle = NULL;
static float temperature_c = 0;

int temperature_init() {
    temperature_c = 0;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10,50);

    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));

    return ESP_OK;
}

void temperature_task(void *param) {
    while(1) {
        esp_err_t ret = temperature_sensor_get_celsius(temp_handle, &temperature_c);
        if(ret == ESP_OK) {
            ESP_LOGI(TAG_M, "Chip temperature: %.2f C", temperature_c);
        } else {
            ESP_LOGI(TAG_M, "Failed to read temperature with error: %s", esp_err_to_name(ret));
        }

        indicate_temperature();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

errorType_e get_temperature(int32_t *temp) {
    if(temp == NULL) {
        return ERR_NULL_PTR;
    }
    *temp = (int32_t)(temperature_c * 100.0f);
    return OK;
}