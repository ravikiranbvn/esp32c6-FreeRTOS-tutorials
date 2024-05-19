/*
 * Dp-1-SW-TIMERS
 * Timer manager
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

static const char *TAG = "timer_manager";

// Timer type enumeration
typedef enum {
    TIMER_TYPE_ONE_SHOT,
    TIMER_TYPE_PERIODIC,
    TIMER_TYPE_UNKNOWN
} TimerType_t;

// Timer manager
typedef struct {
    TimerHandle_t timer_handle;                 // timer handle
    uint32_t start_time;                        // timer start time
    uint32_t stop_time;                         // timer stop time
    uint32_t elapsed_time_before_stop;          // time elapsed (between start time and stop time)
    const char *timer_name;                     // timer name
    TimerType_t type;                           // timer type
    bool isRunning;                             // timer status
} TimerManager_t;

void TimerManager_Init(TimerManager_t *tm, const char *name, const uint32_t timer_period, TimerCallbackFunction_t callback, TimerType_t type) {
    tm->timer_name = name;
    tm->start_time = 0;
    tm->stop_time = 0;
    tm->elapsed_time_before_stop = 0;
    tm->isRunning = false;
    if (type == TIMER_TYPE_ONE_SHOT) {
        tm->timer_handle = xTimerCreate(tm->timer_name, pdMS_TO_TICKS(timer_period), pdFALSE, (void*)tm, callback);
    } else {
        tm->timer_handle = xTimerCreate(tm->timer_name, pdMS_TO_TICKS(timer_period), pdTRUE, (void*)tm, callback);
    }

    if (tm->timer_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create timer with period %" PRIu32 " milliseconds", timer_period);
    } else {
        ESP_LOGI(TAG, "Timer created successfully with period %" PRIu32 " milliseconds", timer_period);
    }
}

void TimerManager_Start(TimerManager_t *tm) {
    tm->start_time = esp_log_timestamp();
    if (xTimerStart(tm->timer_handle, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start timer: %s", tm->timer_name);
    } else {
        tm->isRunning = true;
        ESP_LOGI(TAG, "Timer %s started at %" PRIu32 " milliseconds", tm->timer_name, tm->start_time);
    }
}

void TimerManager_Stop(TimerManager_t *tm) {
    if (xTimerStop(tm->timer_handle, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to stop timer: %s", tm->timer_name);
    } else {
        tm->isRunning = false;
        tm->stop_time = esp_log_timestamp();
        tm->elapsed_time_before_stop += tm->stop_time - tm->start_time;
        ESP_LOGI(TAG, "Timer %s stopped at %" PRIu32 " milliseconds. Elapsed time: %" PRIu32 " milliseconds",
                 tm->timer_name, tm->stop_time, tm->elapsed_time_before_stop);
    }
}

void TimerManager_Reset(TimerManager_t *tm) {
    ESP_LOGI(TAG, "Timer %s reset.", tm->timer_name);
    tm->start_time = 0;
    tm->stop_time = 0;
    tm->elapsed_time_before_stop = 0;
    tm->isRunning = false;
    tm->type = TIMER_TYPE_UNKNOWN;
    tm->timer_name = "";
}

uint32_t TimerManager_GetElapsedTime(TimerManager_t *tm) {
    if (xTimerIsTimerActive(tm->timer_handle)) {
        uint32_t current_time = esp_log_timestamp();
        return tm->elapsed_time_before_stop + (current_time - tm->start_time);
    } else {
        return tm->elapsed_time_before_stop;
    }
}

void vTimerCb( TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Timer expired!");
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

void app_main() {
    // print chip info
    printChipInfo();
    
    // set log level
    esp_log_level_set("*", ESP_LOG_INFO);

    // print FreeRTOS version
    ESP_LOGI(TAG, "FreeRTOS Version: %s", tskKERNEL_VERSION_NUMBER);

    // timer manager
    TimerManager_t tm;
    TimerManager_Init(&tm, "periodicTimer", 3000, vTimerCb, TIMER_TYPE_PERIODIC);

    // trigger timer manager
    TimerManager_Start(&tm);

    // simulate
    vTaskDelay(pdMS_TO_TICKS(1000));

    // stop
    TimerManager_Stop(&tm);

    // print time elapsed
    uint32_t total_elapsed_time = TimerManager_GetElapsedTime(&tm);
    ESP_LOGI(TAG, "Total elapsed time: %" PRIu32 " milliseconds", total_elapsed_time);
    vTaskDelay(pdMS_TO_TICKS(100));

    // reset
    TimerManager_Reset(&tm);
}
