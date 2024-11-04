/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

#define SUPERVISOR_TASK_PRIORITY      ( tskIDLE_PRIORITY + 3UL )
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define LOW_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1UL )
#define LOW_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define HIGH_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2UL )
#define HIGH_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

typedef struct PIData {
    SemaphoreHandle_t sempahore;
};

SemaphoreHandle_t semaphore;
bool on;
int count;

int low_counter = 0;


void high_task (__unused void *params) {
    vTaskDelay(portTICK_PERIOD_MS*100);
    xSemaphoreTake(semaphore, portMAX_DELAY);

    hard_assert(cyw43_arch_init() == PICO_OK);
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        if (count++ % 11) on = !on;
        vTaskDelay(500);
    }
}

void low_task (__unused void *params) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
    while(1) {
        low_counter++;
        vTaskDelay(portTICK_PERIOD_MS*1000);
    }
}

void supervisor_task(__unused void *params) {
    while (1) {
        vTaskDelay(portTICK_PERIOD_MS*5000);
        putchar(32 + low_counter);
    }
}

int main( void )
{
    stdio_init_all();
    const char *rtos_name;
    rtos_name = "FreeRTOS";

    TaskHandle_t supervisor_task;
    TaskHandle_t low_task;
    TaskHandle_t high_task;

    semaphore = xSemaphoreCreateBinary();

    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, &supervisor_task);
    xTaskCreate(low_task, "LowThread",
                LOW_TASK_STACK_SIZE, NULL, LOW_TASK_PRIORITY, &low_task);
    xTaskCreate(high_task, "HighThread",
                HIGH_TASK_STACK_SIZE, NULL, HIGH_TASK_PRIORITY, &high_task); 
    
    vTaskStartScheduler();
    return 0;
}
