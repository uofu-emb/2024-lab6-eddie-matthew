#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include "FreeRTOS.h"
#include <task.h>
#include <semphr.h>

#define SUPERVISOR_TASK_PRIORITY      ( tskIDLE_PRIORITY + 4UL )
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define LOW_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1UL )
#define LOW_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define MEDIUM_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2UL )
#define MEDIUM_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define HIGH_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3UL )
#define HIGH_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t semaphore;
SemaphoreHandle_t high_task_completion_flag;

TaskHandle_t supervisor_task_handle;
TaskHandle_t low_task_handle;
TaskHandle_t high_task_handle;
TaskHandle_t medium_task_handle;

void high_task (__unused void *params) {
    vTaskDelay(portTICK_PERIOD_MS*100);
    xSemaphoreTake(semaphore, portMAX_DELAY);
    xSemaphoreGive(high_task_completion_flag);
    while (1) {};
}

void medium_task (__unused void *params) {    
    vTaskDelay(portTICK_PERIOD_MS*200);
    busy_wait_ms(3000);
    vTaskDelay(portTICK_PERIOD_MS*5000);
    while (1) {};
}

void low_task (__unused void *params) {
    bool res = xSemaphoreTake(semaphore, 100);
    int i = 0;
    busy_wait_ms(200);
    xSemaphoreGive(semaphore);
    while (1) {};
}

uint32_t induce_priority_inversion () {
    TaskHandle_t high;
    TaskHandle_t med;
    TaskHandle_t low;
    high_task_completion_flag = xSemaphoreCreateCounting(1, 0);

    TickType_t start_time = xTaskGetTickCount();
    xTaskCreate(low_task, "LOW", LOW_TASK_STACK_SIZE, NULL, LOW_TASK_PRIORITY, &low);
    xTaskCreate(medium_task, "MED", MEDIUM_TASK_STACK_SIZE, NULL, MEDIUM_TASK_PRIORITY, &med);
    xTaskCreate(high_task, "HIGH", HIGH_TASK_STACK_SIZE, NULL, HIGH_TASK_PRIORITY, &high);

    xSemaphoreTake(high_task_completion_flag, portMAX_DELAY);

    vSemaphoreDelete(high_task_completion_flag);

    TickType_t end_time = xTaskGetTickCount();

    uint32_t elapsed = (end_time - start_time)*portTICK_PERIOD_MS;

    vTaskDelete(high);
    vTaskDelete(med);
    vTaskDelete(low);

    return elapsed;
}

void test_priority_inversion_binary () {
    semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore);
    uint32_t elapsed_ms = induce_priority_inversion();

    TEST_ASSERT_GREATER_THAN_UINT32(3000, elapsed_ms);

    vSemaphoreDelete(semaphore);
}

void test_priority_inversion_mutex () {
    semaphore = xSemaphoreCreateMutex();

    uint32_t elapsed_ms = induce_priority_inversion();

    TEST_ASSERT_LESS_THAN_UINT32(3000, elapsed_ms);

    vSemaphoreDelete(semaphore);
}

void tearDown(void) {}
void setUp(void) {}

void main_thread (__unused void *params) {
    while (1) {
        printf("Start tests\n");
        UNITY_BEGIN();
        RUN_TEST(test_priority_inversion_binary);
        RUN_TEST(test_priority_inversion_mutex);
        sleep_ms(10000);
        UNITY_END();
    }
}

int main (void)
{
    stdio_init_all();
    sleep_ms(5000); // Give time for TTY to attach.

    TaskHandle_t main_handle;
    xTaskCreate(main_thread, "MainThread",
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+10, &main_handle);
    vTaskStartScheduler();
}
