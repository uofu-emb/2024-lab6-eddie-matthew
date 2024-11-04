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

};

SemaphoreHandle_t semaphore;


void high_task (__unused void *params) {
    
}

void low_task (__unused void *params) {

}

void supervisor_task(__unused void *params) {

}

int main( void )
{
    stdio_init_all();
    const char *rtos_name;
    rtos_name = "FreeRTOS";
    TaskHandle_t task;

    semaphore = xSemaphoreCreateBoolean();

    xTaskCreate(supervisor_task, "MainThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, &task);
    vTaskStartScheduler();
    return 0;
}
