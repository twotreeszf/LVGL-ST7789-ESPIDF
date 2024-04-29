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

#define TAG "demo"
#define LV_TICK_PERIOD_MS 40

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

    size_t buff_size = CONFIG_LV_HOR_RES_MAX * 40 * sizeof(lv_color16_t);
    lv_color16_t* buf1 = heap_caps_malloc(buff_size * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    lv_color16_t* buf2 = heap_caps_malloc(buff_size * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    lv_display_t* disp = lv_display_create(CONFIG_LV_HOR_RES_MAX, CONFIG_LV_VER_RES_MAX);    
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
        vTaskDelay(pdMS_TO_TICKS(LV_TICK_PERIOD_MS));

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

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}

static void lv_example_anim_2(void);
static void create_demo_application() {
    lv_example_anim_2();
}

/**
 * Create a playback animation
 */
static void lv_example_anim_2(void)
{
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);

    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 10, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 10, 50);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_playback_delay(&a, 100);
    lv_anim_set_playback_duration(&a, 300);
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_set_exec_cb(&a, anim_size_cb);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_values(&a, 10, 260);
    lv_anim_start(&a);
}