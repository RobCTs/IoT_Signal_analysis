#include "generating_signal.h"
#include "sampling_and_analyzing.h"
#include "network.h"
#include "transmitting.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ina219.h>

static const char* TAG = "TaskHandler";

#define I2C_PORT 0
#define SHUNT_RESISTOR_MILLI_OHM 100
#define EXAMPLE_I2C_ADDR 0x40
#define I2C_MASTER_SCL 2
#define I2C_MASTER_SDA 1
#define I2C_ADDR EXAMPLE_I2C_ADDR
ina219_t dev;
float power;
int64_t task1_total_time = 0;
int64_t task2_total_time = 0;
int64_t task3_total_time = 0;
int64_t task4_total_time = 0;

extern QueueHandle_t signalQueue;
char message[256];  // buffer for MQTT messages

TaskHandle_t xsignalReadyTask = NULL;
TaskHandle_t xanalysisCompleteTask = NULL;
TaskHandle_t xaverageComputeTask = NULL;
TaskHandle_t xoversampleTask = NULL;
TaskHandle_t xmonitorSystemHealthTask = NULL;
TaskHandle_t xNetworkConfigTask = NULL;
TaskHandle_t xPowerMeasurementTask = NULL;  
TaskHandle_t xTaskPower = NULL;

EventGroupHandle_t syncEventGroup;
const int TASK_1_READY_BIT = BIT0;
const int TASK_2_READY_BIT = BIT1;
const int TASK_3_READY_BIT = BIT2;


void generate_signal_task(void *pvParameters) {
    ESP_LOGI(TAG, "Generate signal initialized.");
    esp_task_wdt_add(NULL);  // add this task to the watchdog timer
    int64_t start_time, end_time;

    while (true) {
        ESP_LOGI(TAG, "(Task 1) Generating signal...");
        start_time = esp_timer_get_time();
        generate_random_signal();
        end_time = esp_timer_get_time();
        task1_total_time += (end_time - start_time); 
        ESP_LOGI(TAG, "Signal generated, time taken: %lld us.", end_time - start_time);
        snprintf(message, sizeof(message), "Generating signal task took: %lld us", end_time - start_time);
        mqtt_publish("/task_time", message);

        esp_task_wdt_reset();
        vTaskDelay(5000);
    }
}

void sample_and_analyze_task(void *pvParameters) {
    ESP_LOGI(TAG, "Sampling and analysis initialized.");
    float signal; // to hold the signal value from the queue
    int64_t start_time, end_time;
    UBaseType_t uxHighWaterMark;
    
    xEventGroupSync(syncEventGroup, TASK_2_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL); 

    while (true) {
        if (xQueueReceive(signalQueue, &signal, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("DataProcess", "Received signal: %.2f", signal);
            
            ESP_LOGI(TAG, "Analyzing signal...");
            start_time = esp_timer_get_time();
            sample_and_analyze_signal(); // update optimal_sampling_rate
            end_time = esp_timer_get_time();
            task2_total_time += (end_time - start_time);
            ESP_LOGI(TAG, "Analysis complete, time taken: %lld us. Notifying Task 3.", end_time - start_time);
            snprintf(message, sizeof(message), "Sampling and analyzing task took: %lld us", end_time - start_time);
            mqtt_publish("/task_time", message);

        xTaskNotifyGive(xaverageComputeTask);
        esp_task_wdt_reset();

        // Check the remaining stack space
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Minimum stack space left in sample_and_analyze_task: %u", uxHighWaterMark);
        }
    }
    // esp_task_wdt_delete(NULL);
    // vTaskDelete(NULL);
}

void compute_average_task(void *pvParameters) {
    ESP_LOGI(TAG, "Computing average initialized.");
    int64_t start_time, end_time;

    xEventGroupSync(syncEventGroup, TASK_3_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL); 

    while (true) {
        //ESP_LOGI(TAG, "Task 3 waiting for signal from Task 2...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Computing average...");
        start_time = esp_timer_get_time();
        float average = sample_signal_and_compute_average(5); //change the window length if needed
        end_time = esp_timer_get_time();
        task3_total_time += (end_time - start_time);
        ESP_LOGI(TAG, "(Task 3) The average signal value: %.2f, time taken: %lld us", average, end_time - start_time);
        snprintf(message, sizeof(message), "Averaging task took: %lld us", end_time - start_time);
        mqtt_publish("/task_time", message);

        // //ESP_LOGI(TAG, "Task 3 notifying Task 1 to start next cycle.");
        // xTaskNotifyGive(xoversampleTask);
    }
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}

void oversample_task(void *pvParameters) {
    ESP_LOGI(TAG, "Oversampling initialized.");
    int64_t start_time, end_time;
    float signal2; 

    esp_task_wdt_add(NULL);  // add this task to the watchdog timer
    UBaseType_t uxHighWaterMark;

    while (true) {
        if (xQueueReceive(signalQueue, &signal2, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("DataProcess", "Received signal: %.2f", signal2);
    
            ESP_LOGI(TAG, "Starting oversampling...");
            start_time = esp_timer_get_time();
            int crashed_rate = oversample();
            end_time = esp_timer_get_time();
            task4_total_time += (end_time - start_time);
            ESP_LOGE(TAG, "Crash detected at %d, time taken: %lld us", crashed_rate, end_time - start_time);
            snprintf(message, sizeof(message), "Oversampling task took: %lld us", end_time - start_time);
            mqtt_publish("/task_time", message);

            // Check the remaining stack space
            uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "Minimum stack space left in oversample_task: %u", uxHighWaterMark);
        }
    // esp_task_wdt_delete(NULL);
    // vTaskDelete(NULL);
    }
}

void monitor_system_health() {
    ESP_LOGI(TAG, "Task 6 initialized.");
    while (true) {
    unsigned int free_heap = esp_get_free_heap_size();
    unsigned int minimum_ever_free_heap = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG, "Free Heap: %u bytes", free_heap);
    snprintf(message, sizeof(message), "Free Heap: %u bytes", free_heap);
    mqtt_publish("/monitoring", message);
    ESP_LOGI(TAG, "Minimum Ever Free Heap: %u bytes", minimum_ever_free_heap);
    snprintf(message, sizeof(message), "Minimum Ever Free Heap: %u bytes", minimum_ever_free_heap);
    mqtt_publish("/monitoring", message);
    xTaskNotifyGive(xsignalReadyTask);

    vTaskDelay(pdMS_TO_TICKS(3000));  // 3s
    }
}

// Initialize INA219 sensor
void initialize_power_sensor(void) {
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&dev, 0, sizeof(ina219_t));
    assert(SHUNT_RESISTOR_MILLI_OHM > 0);
    ESP_ERROR_CHECK(ina219_init_desc(&dev, I2C_ADDR, I2C_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL));
    ESP_LOGI(TAG, "Initializing ina219");
    ESP_ERROR_CHECK(ina219_init(&dev));
    ESP_LOGI(TAG, "Configuring ina219");
    ESP_ERROR_CHECK(ina219_configure(&dev, INA219_BUS_RANGE_16V, INA219_GAIN_0_125,
                                     INA219_RES_12BIT_1S, INA219_RES_12BIT_1S, INA219_MODE_CONT_SHUNT_BUS));
    ESP_LOGI(TAG, "Calibrating ina219");
    ESP_ERROR_CHECK(ina219_calibrate(&dev, (float)SHUNT_RESISTOR_MILLI_OHM / 1000.0f));
}

void system_power_measurement_task(void *pvParameters) {
    float power;
    ESP_LOGI(TAG, "System power measurement task started");
    while (true) {
        esp_err_t ret = ina219_get_power(&dev, &power);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Power: %.2f mW", power);
            snprintf(message, sizeof(message), "Power: %.2f mW", power);
            mqtt_publish("/power_measurement", message);
        } else {
            ESP_LOGE(TAG, "Failed to get power reading: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void log_energy_consumption() {
    float power;
    char message[256];

    if (ina219_get_power(&dev, &power) == ESP_OK) {
        printf("Power: %.2f mW\n", power);
        
        // Calculate total elapsed time
        int64_t total_time = task1_total_time + task2_total_time + task3_total_time + task4_total_time;

        // Calculate energy consumption for each task
        float task1_energy = (task1_total_time / (float)total_time) * power * (total_time / 1000000.0);
        float task2_energy = (task2_total_time / (float)total_time) * power * (total_time / 1000000.0);
        float task3_energy = (task3_total_time / (float)total_time) * power * (total_time / 1000000.0);
        float task4_energy = (task4_total_time / (float)total_time) * power * (total_time / 1000000.0);

        printf("Task 1 Energy: %.2f mJ\n", task1_energy);
        snprintf(message, sizeof(message), "(Task 1) Generating signal Energy: %.2f mJ\n", task1_energy);
        mqtt_publish("/task_time", message);
        printf("Task 2 Energy: %.2f mJ\n", task2_energy);
        snprintf(message, sizeof(message), "(Task 2)Analyzing Energy: %.2f mJ\n", task2_energy);
        mqtt_publish("/task_time", message);
        printf("Task 3 Energy: %.2f mJ\n", task3_energy);
        snprintf(message, sizeof(message), "(Task 3) Computing averageEnergy: %.2f mJ\n", task3_energy);
        mqtt_publish("/task_time", message);
        printf("Task 4 Energy: %.2f mJ\n", task4_energy);
        snprintf(message, sizeof(message), "Oversampling Energy: %.2f mJ\n", task4_energy);
        mqtt_publish("/task_time", message);
    } else {
        printf("Failed to read power\n");
    }
}

void tasks_power_measurment_task(void *pvParameters) {
    while (true) {
        log_energy_consumption();
        vTaskDelay(pdMS_TO_TICKS(1000)); // log energy consumption every second
    }
}

void network_config_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network Config initialized.");    
    network_init();  // initialize network

    ESP_LOGI(TAG, "Network Configuration completed.");
    mqtt_app_start();  // start MQTT
    ESP_LOGI(TAG, "Credentials correctly configured");

    // Suspend this task indefinitely
    vTaskSuspend(NULL);
}


void app_main() {
    ESP_LOGI(TAG, "Application starting...");
    
    setup_signal_queue();
    ESP_LOGI(TAG, "Signal queue setup complete.");
    
    srand(time(NULL)); // seed the random number generator
    nvs_init();
    
    syncEventGroup = xEventGroupCreate();

    // Initialize INA219 sensor
    initialize_power_sensor();

    xTaskCreate(network_config_task, "NetworkConfigTask", 4096, NULL, 9, &xNetworkConfigTask);

    vTaskDelay(pdMS_TO_TICKS(10000));  // five some time for the configuration

    xTaskCreate(generate_signal_task, "GenerateSignalTask", 8192, NULL, 7, &xsignalReadyTask);
    xTaskCreate(sample_and_analyze_task, "SampleAndAnalyzeTask", 8192, NULL, 6, &xanalysisCompleteTask);
    xTaskCreate(compute_average_task, "ComputeAverageTask", 4096, NULL, 5, &xaverageComputeTask);
    xTaskCreate(oversample_task, "OversampleTask", 8192, NULL, 4, &xoversampleTask);

    xTaskCreate(monitor_system_health, "MonitorHealthTask", 4096, NULL, 4, &xmonitorSystemHealthTask);
    xTaskCreate(system_power_measurement_task, "PowerMeasurementTask", 4096, NULL, 5, &xPowerMeasurementTask);
    xTaskCreate(tasks_power_measurment_task, "TasksPowerMeasurementTask", 4096, NULL, 6, &xTaskPower);
}