/*
 * Dp-1-SW-TIMERS
 * Introduction to timers in FreeRTOS
 * author: Venkata Naga Ravikiran Bulusu
 *
 */

#include <stdio.h>
#include <inttypes.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "DP-1-SW-TIMERS";
static uint32_t one_shot_start_time;
static uint32_t periodic_start_time;

void vTimerCbOneShot(TimerHandle_t xTimer) {
    uint32_t end_time = esp_log_timestamp();
    uint32_t elapsed_time = end_time - one_shot_start_time;
    ESP_LOGI(TAG, "[%" PRIu32 "] one shot timer expired!", elapsed_time);
}

void vTimerCbPeriodic(TimerHandle_t xTimer) {
   uint32_t end_time = esp_log_timestamp();
   uint32_t elapsed_time = end_time - periodic_start_time;
   ESP_LOGI(TAG, "[%" PRIu32 "] periodic timer expired!", elapsed_time);
}

void setupTimers(void) {
    // handles
    TimerHandle_t oneshot_xTimer;
    TimerHandle_t periodic_xTimer;

    // create one shot timer
    oneshot_xTimer = xTimerCreate(
                     "oneShotTimer",             // Name of timer
                     pdMS_TO_TICKS(2000),        // Period of timer (in ticks)
                     pdFALSE,                    // one-shot
                     (void *)0,                  // Timer ID
                     vTimerCbOneShot);           // Callback function
    configASSERT(oneshot_xTimer);

    one_shot_start_time = esp_log_timestamp(); 
    if (xTimerStart(oneshot_xTimer, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE(TAG, "failed to start one stop timer");
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // create periodic timer
    periodic_xTimer = xTimerCreate(
                      "Auto-reload timer",        // Name of timer
                      pdMS_TO_TICKS(1000),        // Period of timer (in ticks)
                      pdTRUE,                     // Auto-reload
                      (void *)1,                  // Timer ID
                      vTimerCbPeriodic);          // Callback function                  
    configASSERT(periodic_xTimer);

    periodic_start_time = esp_log_timestamp();
    if (xTimerStart(periodic_xTimer, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE(TAG, "failed to start periodic timer");
    }
}

void printChipInfo(void) {
    printf("----------Printing chip information!----------\n");
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}

void app_main(void) {
    // print chip info
    printChipInfo();
    
    // set log level
    esp_log_level_set("*", ESP_LOG_INFO);

    // print FreeRTOS version
    ESP_LOGI(TAG, "FreeRTOS Version: %s", tskKERNEL_VERSION_NUMBER);

    // timer setup
    setupTimers();
}