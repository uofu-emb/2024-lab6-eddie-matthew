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
QueueHandle_t priority_level_msg_queue;

TaskHandle_t supervisor_task_handle;
TaskHandle_t low_task_handle;
TaskHandle_t high_task_handle;
TaskHandle_t medium_task_handle;

void take_and_message (char level) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
    xQueueSendToBack(priority_level_msg_queue, level, portMAX_DELAY);
}

void high_task (__unused void *params) {
    vTaskDelay(portTICK_PERIOD_MS*100);
    take_and_message('H');
    while (1) {};
}

void medium_task (__unused void *params) {    
    vTaskDelay(portTICK_PERIOD_MS*200);
    busy_wait_ms(10000);
    vTaskDelay(portTICK_PERIOD_MS*5000);
    while (1) {};
}

void low_task (__unused void *params) {
    take_and_message('L');
    int i = 0;
    busy_wait_ms(200);
    xSemaphoreGive(semaphore);
    while (1) {};
}

void supervisor_task(__unused void *params) {
    char semphr_owner;
    xQueueReceive(priority_level_msg_queue, &semphr_owner, portMAX_DELAY);
    TEST_ASSERT_EQUAL_CHAR('L', semphr_owner);
    xQueueReceive(priority_level_msg_queue, &semphr_owner, portMAX_DELAY);
    TEST_ASSERT_EQUAL_CHAR('H', semphr_owner);
    TickType_t time = xTaskGetTickCount();
    TEST_ASSERT_GREATER_THAN_UINT32(10000*portTICK_PERIOD_MS, time);
}

void test_priority_inversion_binary()
{
    semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore);
    priority_level_msg_queue = xQueueCreate(10, sizeof(char));

    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, &supervisor_task_handle);
    xTaskCreate(low_task, "LowThread",
                LOW_TASK_STACK_SIZE, NULL, LOW_TASK_PRIORITY, &low_task_handle);
    xTaskCreate(medium_task, "MediumThread",
                MEDIUM_TASK_STACK_SIZE, NULL, MEDIUM_TASK_PRIORITY, &medium_task_handle);
    xTaskCreate(high_task, "HighThread",
                HIGH_TASK_STACK_SIZE, NULL, HIGH_TASK_PRIORITY, &high_task_handle); 
    
    vTaskStartScheduler();
}

void test_priority_inversion_mutex () {
    semaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(semaphore);
    priority_level_msg_queue = xQueueCreate(10, sizeof(char));

    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, &supervisor_task_handle);
    xTaskCreate(low_task, "LowThread",
                LOW_TASK_STACK_SIZE, NULL, LOW_TASK_PRIORITY, &low_task_handle);
    xTaskCreate(medium_task, "MediumThread",
                MEDIUM_TASK_STACK_SIZE, NULL, MEDIUM_TASK_PRIORITY, &medium_task_handle);
    xTaskCreate(high_task, "HighThread",
                HIGH_TASK_STACK_SIZE, NULL, HIGH_TASK_PRIORITY, &high_task_handle); 
    
    vTaskStartScheduler();
}

void setUp(void) {}

void tearDown(void) {}

int main (void)
{
    stdio_init_all();
    sleep_ms(5000); // Give time for TTY to attach.
    printf("\nStart tests\n");
    UNITY_BEGIN();
    RUN_TEST(test_priority_inversion_binary);
    // RUN_TEST(test_priority_inversion_mutex);
    sleep_ms(5000);
    return UNITY_END();
}
