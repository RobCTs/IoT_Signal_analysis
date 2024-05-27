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
#include "driver/i2c.h"
#include "ina219.h"

static const char* TAG = "TaskHandler";

// #define I2C_PORT 0
// #define CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM 100
// #define CONFIG_EXAMPLE_I2C_ADDR 0x40
// #define CONFIG_EXAMPLE_I2C_MASTER_SCL 2
// #define CONFIG_EXAMPLE_I2C_MASTER_SDA 1
// #define I2C_ADDR CONFIG_EXAMPLE_I2C_ADDR
// ina219_t dev;
// float power;

extern QueueHandle_t signalQueue;
char message[256];  // buffer for MQTT messages

TaskHandle_t xsignalReadyTask = NULL;
TaskHandle_t xanalysisCompleteTask = NULL;
TaskHandle_t xaverageComputeTask = NULL;
TaskHandle_t xoversampleTask = NULL;
TaskHandle_t xmonitorSystemHealthTask = NULL;
//TaskHandle_t xTransmittingTask = NULL;
TaskHandle_t xNetworkConfigTask = NULL;
TaskHandle_t xPowerMeasurementTask = NULL;  

EventGroupHandle_t syncEventGroup;
const int TASK_1_READY_BIT = BIT0;
const int TASK_2_READY_BIT = BIT1;
const int TASK_3_READY_BIT = BIT2;
const int TASK_4_READY_BIT = BIT3;
const int TASK_5_READY_BIT = BIT4;
//const int TASK_6_READY_BIT = BIT5;
const int TASK_7_READY_BIT = BIT6;
const int TASK_8_READY_BIT = BIT7; 



void generate_signal_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 1 initialized.");
    xEventGroupSync(syncEventGroup, TASK_1_READY_BIT, TASK_1_READY_BIT | TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT | TASK_5_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL);  // add this task to the watchdog timer
    int64_t start_time, end_time;

    while (true) {
        ESP_LOGI(TAG, "Task 1 generating signal...");
        start_time = esp_timer_get_time();
        generate_random_signal();
        end_time = esp_timer_get_time(); 
        ESP_LOGI(TAG, "Task 1 signal generated, time taken: %lld us. Signaling task 2.", end_time - start_time);
        snprintf(message, sizeof(message), "Generating signal task took: %lld us", end_time - start_time);
        mqtt_publish("/task_time", message);

        // xTaskNotifyGive(xanalysisCompleteTask);
        // //ESP_LOGI(TAG, "Task 1 waiting for other tasks to complete...");
        // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_task_wdt_reset();
        vTaskDelay(5000);
    }
    // esp_task_wdt_delete(NULL); 
    // vTaskDelete(NULL);
}

void sample_and_analyze_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 2 initialized.");
    float signal; // declare the variable to hold the signal value from the queue
    int64_t start_time, end_time;
    UBaseType_t uxHighWaterMark;
    
    xEventGroupSync(syncEventGroup, TASK_2_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT | TASK_5_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL); 

    while (true) {
        //ESP_LOGI(TAG, "Task 2 waiting for signal from Task 1...");
        //ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xQueueReceive(signalQueue, &signal, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("DataProcess", "Received signal: %.2f", signal);
            
            ESP_LOGI(TAG, "Task 2 analyzing signal...");
            start_time = esp_timer_get_time();
            sample_and_analyze_signal(); // update optimal_sampling_rate
            end_time = esp_timer_get_time();
            ESP_LOGI(TAG, "Task 2 analysis complete, time taken: %lld us. Notifying Task 3.", end_time - start_time);
            //uxTaskGetStackHighWaterMark to check stack usage?
            snprintf(message, sizeof(message), "Sampòoing and analyzing task took: %lld us", end_time - start_time);
            mqtt_publish("/task_time", message);

        xTaskNotifyGive(xaverageComputeTask);
        esp_task_wdt_reset();

        // Check the remaining stack space
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Minimum stack space left in sample_and_analyze_task: %u", uxHighWaterMark);
    }
    }
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}

void compute_average_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 3 initialized.");
    int64_t start_time, end_time;

    xEventGroupSync(syncEventGroup, TASK_3_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT | TASK_5_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL); 

    while (true) {
        //ESP_LOGI(TAG, "Task 3 waiting for signal from Task 2...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Task 3 computing average...");
        start_time = esp_timer_get_time();
        float average = sample_signal_and_compute_average(5); //change the window length if needed
        end_time = esp_timer_get_time();
        ESP_LOGI(TAG, "Task 3 average signal value: %.2f, time taken: %lld us", average, end_time - start_time);
        snprintf(message, sizeof(message), "Averaging task took: %lld us", end_time - start_time);
        mqtt_publish("/task_time", message);

        //ESP_LOGI(TAG, "Task 3 notifying Task 1 to start next cycle.");
        xTaskNotifyGive(xoversampleTask);
    }
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}

void oversample_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 4 initialized.");
    int64_t start_time, end_time;

    xEventGroupSync(syncEventGroup, TASK_4_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT | TASK_5_READY_BIT, portMAX_DELAY);
    esp_task_wdt_add(NULL);  // add this task to the watchdog timer
    UBaseType_t uxHighWaterMark;

    while (true) {
        //ESP_LOGI(TAG, "Task 4 waiting for signal from Task 3...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        ESP_LOGI(TAG, "Task 4 starting oversampling...");
        start_time = esp_timer_get_time();
        int crashed_rate = oversample();
        end_time = esp_timer_get_time();
        ESP_LOGE(TAG, "Crash detected at %d, time taken: %lld us", crashed_rate, end_time - start_time);
        snprintf(message, sizeof(message), "Oversampling task took: %lld us", end_time - start_time);
        mqtt_publish("/task_time", message);

        //ESP_LOGI(TAG, "Task 4 notifying Task 1 to start next cycle.");
        xTaskNotifyGive(xmonitorSystemHealthTask);

        // Check the remaining stack space
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Minimum stack space left in oversample_task: %u", uxHighWaterMark);
    
    }
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}

void monitor_system_health() {
    ESP_LOGI(TAG, "Task 6 initialized.");
    xEventGroupSync(syncEventGroup, TASK_5_READY_BIT, TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT | TASK_5_READY_BIT, portMAX_DELAY);
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    unsigned int free_heap = esp_get_free_heap_size();
    unsigned int minimum_ever_free_heap = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG, "Free Heap: %u bytes", free_heap);
    snprintf(message, sizeof(message), "Free Heap: %u bytes", free_heap);
    mqtt_publish("/monitoring", message);
    ESP_LOGI(TAG, "Minimum Ever Free Heap: %u bytes", minimum_ever_free_heap);
    snprintf(message, sizeof(message), "Minimum Ever Free Heap: %u bytes", minimum_ever_free_heap);
    mqtt_publish("/monitoring", message);
    xTaskNotifyGive(xsignalReadyTask);

    vTaskDelay(pdMS_TO_TICKS(10000));  // Log every 10 seconds
    vTaskDelay(5000 / portTICK_PERIOD_MS);  // pause before starting next cycle
}


// // Function to initialize INA219 sensor
// void initialize_ina219_library(void) {
//     ESP_ERROR_CHECK(i2cdev_init());
//     memset(&dev, 0, sizeof(ina219_t));
//     assert(CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM > 0);
//     ESP_ERROR_CHECK(ina219_init_desc(&dev, I2C_ADDR, I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO));
//     ESP_LOGI(TAG, "Initializing ina219");
//     ESP_ERROR_CHECK(ina219_init(&dev));
//     ESP_LOGI(TAG, "Configuring ina219");
//     ESP_ERROR_CHECK(ina219_configure(&dev, INA219_BUS_RANGE_16V, INA219_GAIN_0_125,
//                                      INA219_RES_12BIT_1S, INA219_RES_12BIT_1S, INA219_MODE_CONT_SHUNT_BUS));
//     ESP_LOGI(TAG, "Calibrating ina219");
//     ESP_ERROR_CHECK(ina219_calibrate(&dev, (float)CONFIG_EXAMPLE_SHUNT_RESISTOR_MILLI_OHM / 1000.0f));
// }

// // Power measurement task to read and log power values
// void power_measurement_task(void *pvParameters) {
//     float power;
//     ESP_LOGI(TAG, "Power measurement task started");
//     while (true) {
//         esp_err_t ret = ina219_get_power(&dev, &power);
//         if (ret == ESP_OK) {
//             ESP_LOGI(TAG, "Power: %.2f mW", power);
//         } else {
//             ESP_LOGE(TAG, "Failed to get power reading: %s", esp_err_to_name(ret));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));  // Measure every second
//     }
// }



void network_config_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network Config initialized.");    
    network_init();  // Initialize network
    ESP_LOGI(TAG, "Network Configuration completed.");
    mqtt_app_start();  // Start MQTT
    ESP_LOGI(TAG, "Credentials correctly configured");

    // // Signal that this task has completed its configuration.
    // xEventGroupSetBits(syncEventGroup2, TASK_6_READY_BIT);

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

    xTaskCreate(network_config_task, "NetworkConfigTask", 4096, NULL, 9, &xNetworkConfigTask); //NULL?

    vTaskDelay(pdMS_TO_TICKS(15000));  
    xTaskCreate(generate_signal_task, "GenerateSignalTask", 8192, NULL, 7, &xsignalReadyTask);
    xTaskCreate(sample_and_analyze_task, "SampleAndAnalyzeTask", 8192, NULL, 6, &xanalysisCompleteTask);
    xTaskCreate(compute_average_task, "ComputeAverageTask", 4096, NULL, 5, &xaverageComputeTask);
    xTaskCreate(oversample_task, "OversampleTask", 8192, NULL, 4, &xoversampleTask);

    // Keep the main function running
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(2000)); // Sleep for 1000ms (1 second)
        xTaskCreate(monitor_system_health, "MonitorHealthTask", 4096, NULL, 5, &xmonitorSystemHealthTask);
        //xTaskCreate(power_measurement_task, "PowerMeasurementTask", 4096, NULL, 6, &xPowerMeasurementTask);
    }  
}