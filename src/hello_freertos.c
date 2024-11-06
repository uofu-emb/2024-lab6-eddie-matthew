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

#define SUPERVISOR_TASK_PRIORITY      ( tskIDLE_PRIORITY + 4UL )
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define LOW_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1UL )
#define LOW_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define MEDIUM_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2UL )
#define MEDIUM_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define HIGH_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3UL )
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
}

void medium_task (__unused void *params) {    vTaskDelay(portTICK_PERIOD_MS*5000);
    vTaskDelay(portTICK_PERIOD_MS*200);
    busy_wait_ms(500);
}

void low_task (__unused void *params) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
    int i = 0;
    busy_wait_ms(200);
    xSemaphoreGive(semaphore);
}

void supervisor_task(__unused void *params) {
    vTaskDelay(portTICK_PERIOD_MS*5000);
    while(1){};
}

int main( void )
{
    stdio_init_all();
    const char *rtos_name;
    rtos_name = "FreeRTOS";

    TaskHandle_t supervisor_task_handle;
    TaskHandle_t low_task_handle;
    TaskHandle_t high_task_handle;
    TaskHandle_t medium_task_handle;

    semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore);

    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, &supervisor_task_handle);
    xTaskCreate(low_task, "LowThread",
                LOW_TASK_STACK_SIZE, NULL, LOW_TASK_PRIORITY, &low_task_handle);
    xTaskCreate(medium_task, "MediumThread",
                MEDIUM_TASK_STACK_SIZE, NULL, MEDIUM_TASK_PRIORITY, &medium_task_handle);
    xTaskCreate(high_task, "HighThread",
                HIGH_TASK_STACK_SIZE, NULL, HIGH_TASK_PRIORITY, &high_task_handle); 
    
    vTaskStartScheduler();
    return 0;
}
