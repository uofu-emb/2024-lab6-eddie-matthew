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

#define TEST_DURATION 5000

void busy_busy(void)
{
    for (int i = 0; ; i++);
}

void busy_yield(void)
{
    for (int i = 0; ; i++) {
        taskYIELD();
    }
}

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



void test(void* func1,void* func2, uint64_t *t1_time, uint64_t *t2_time, uint64_t t1_t2_delay_ms,  uint16_t priority1,  uint16_t priority2) {
    TaskHandle_t t1, t2;

    uint64_t start_count = portGET_RUN_TIME_COUNTER_VALUE();

    xTaskCreate(func1, "t1", configMINIMAL_STACK_SIZE, NULL, priority1, &t1);

    vTaskDelay(t1_t2_delay_ms / portTICK_PERIOD_MS);

    xTaskCreate(func2, "t2", configMINIMAL_STACK_SIZE, NULL, priority2, &t2);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    *t1_time = ulTaskGetRunTimeCounter(t1);
    *t2_time = ulTaskGetRunTimeCounter(t2);
    
    printf("Task 1 Runtime: %llu\n", *t1_time);
    printf("Task 2 Runtime: %llu\n", *t2_time);

    vTaskDelete(t1);
    vTaskDelete(t2);
}

void both_busy_busy(void) {
    uint64_t t1_time = 0, t2_time = 0;
    test(busy_busy, busy_busy, &t1_time, &t2_time, 0, LOW_TASK_PRIORITY , LOW_TASK_PRIORITY );

    TEST_ASSERT(t1_time > 400000 && t1_time < 600000);
    TEST_ASSERT(t2_time > 400000 && t2_time < 600000);
}

void both_busy_yield(void) {
    uint64_t t1_time = 0, t2_time = 0;
    test(busy_yield, busy_yield, &t1_time, &t2_time, 0, LOW_TASK_PRIORITY , LOW_TASK_PRIORITY );

    TEST_ASSERT(t1_time > 400000 && t1_time < 600000);
    TEST_ASSERT(t2_time > 400000 && t2_time < 600000);
}

void one_busy_one_yield(void) {
    uint64_t t1_time = 0, t2_time = 0;
    test(busy_busy, busy_yield, &t1_time, &t2_time, 0,  LOW_TASK_PRIORITY,  LOW_TASK_PRIORITY);

    TEST_ASSERT(t1_time > 900000);
    TEST_ASSERT(t2_time < 100000);
}



void tearDown(void) {}
void setUp(void) {}

void main_thread (__unused void *params) {
    while (1) {
        printf("Start tests\n");
        UNITY_BEGIN();
        //RUN_TEST(test_priority_inversion_binary);
        //RUN_TEST(test_priority_inversion_mutex);
        RUN_TEST(both_busy_busy);
        RUN_TEST(both_busy_yield);
        RUN_TEST(one_busy_one_yield);
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

    return 0;
}
