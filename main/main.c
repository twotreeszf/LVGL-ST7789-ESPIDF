/* LVGL Example project
 *
 * Basic project to test LVGL on ESP32 based projects.
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "lvgl.h"
#include "lvgl_helpers.h"
#include "demos/widgets/lv_demo_widgets.h"

#define TAG "demo"
#define LV_TICK_PERIOD_MS 10
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 172

static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);

void app_main() {
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 0);
}

SemaphoreHandle_t xGuiSemaphore;
static void guiTask(void *pvParameter) {

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    lvgl_driver_init();

    size_t buff_size = SCREEN_WIDTH * 40 * sizeof(lv_color16_t);
    lv_color16_t* buf1 = heap_caps_malloc(buff_size * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    lv_color16_t* buf2 = heap_caps_malloc(buff_size * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    lv_display_t* disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);    
    lv_display_set_default(disp);
    lv_display_set_buffers(disp, buf1, buf2, buff_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, disp_driver_flush);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create the demo application */
    create_demo_application();

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    /* A task should NEVER return */
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

static void create_demo_application(void)
{
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();
}

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}