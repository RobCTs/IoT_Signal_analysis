#include "generating_signal.h"
#include "sampling_and_analyzing.h"
#include "network.h"
//#include "security.h"
//#include "monitoring.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include <time.h>
#include "esp_task_wdt.h"

static const char* TAG = "TaskHandler";

TaskHandle_t xsignalReadyTask = NULL;
TaskHandle_t xanalysisCompleteTask = NULL;
TaskHandle_t xaverageComputeTask = NULL;
TaskHandle_t xoversampleTask = NULL;
//TaskHandle_t xtransmittingTask = NULL;  

EventGroupHandle_t syncEventGroup;
const int TASK_1_READY_BIT = BIT0;
const int TASK_2_READY_BIT = BIT1;
const int TASK_3_READY_BIT = BIT2;
const int TASK_4_READY_BIT = BIT3;
//const int TASK_4_READY_BIT = BIT4; 

// void configure_wifi_mqtt() {
//     char ssid[32], password[64], mqtt_uri[256];

//     printf("Enter WiFi SSID: ");
//     gets(ssid); // safer alternatives fgets with specified length?

//     printf("Enter WiFi Password: ");
//     gets(password);
//     store_wifi_credentials(ssid, password);

//     printf("Enter MQTT URI: ");
//     gets(mqtt_uri);
//     store_mqtt_broker_uri(mqtt_uri);
// }

void generate_signal_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 1 initialized.");
    xEventGroupSync(syncEventGroup, TASK_1_READY_BIT, TASK_1_READY_BIT | TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT, portMAX_DELAY);

    while (true) {
        ESP_LOGI(TAG, "Task 1 generating signal...");
        generate_random_signal();
        ESP_LOGI(TAG, "Task 1 signal generated, signaling Task 2.");

        xTaskNotifyGive(xanalysisCompleteTask);
        ESP_LOGI(TAG, "Task 1 waiting for Task 3 to complete...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Task 1 resumes after Task 3 completion.");
    }
    vTaskDelete(NULL);
}

void sample_and_analyze_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 2 initialized.");
    xEventGroupSync(syncEventGroup, TASK_2_READY_BIT, TASK_1_READY_BIT | TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT, portMAX_DELAY);

    while (true) {
        ESP_LOGI(TAG, "Task 2 waiting for signal from Task 1...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Task 2 analyzing signal...");
        sample_and_analyze_signal(); // update optimal_sampling_rate
        ESP_LOGI(TAG, "Task 2 analysis complete, notifying Task 3.");


        xTaskNotifyGive(xaverageComputeTask);
        
    }
    vTaskDelete(NULL);
}

void compute_average_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 3 initialized.");
    xEventGroupSync(syncEventGroup, TASK_3_READY_BIT, TASK_1_READY_BIT | TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT, portMAX_DELAY);

    while (true) {
        ESP_LOGI(TAG, "Task 3 waiting for signal from Task 2...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Task 3 computing average...");
        float average = sample_signal_and_compute_average(5); //change the window length if needed
        ESP_LOGI(TAG, "Task 3 average signal value: %.2f", average);

        ESP_LOGI(TAG, "Task 3 notifying Task 1 to start next cycle.");
        xTaskNotifyGive(xoversampleTask);
    }
    vTaskDelete(NULL);
}

void oversample_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task 4 initialized.");
    xEventGroupSync(syncEventGroup, TASK_4_READY_BIT, TASK_1_READY_BIT | TASK_2_READY_BIT | TASK_3_READY_BIT | TASK_4_READY_BIT, portMAX_DELAY);

    while (true) {

        ESP_LOGI(TAG, "Task 4 waiting for signal from Task 3...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        ESP_LOGI(TAG, "Task 4 starting oversampling...");
        int crashed_rate = oversample();
        ESP_LOGE(TAG, "Crash detected at %d", crashed_rate);

        ESP_LOGI(TAG, "Task 4 notifying Task 1 to start next cycle.");
        xTaskNotifyGive(xsignalReadyTask);
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // pause before starting next cycle
    }

    vTaskDelete(NULL);
}


void app_main() {
    srand(time(NULL)); // seed the random number generator

    syncEventGroup = xEventGroupCreate(); // create the event group for task synchronization
    // initialize_crypto();
    //Initialize NVS for the whole application
    //ESP_ERROR_CHECK(nvs_init());

    //configure_wifi_mqtt(); // set up all configurations via Serial

    // Initialize other components
    // network_init();  // initialize network
    // mqtt_app_start();  // start MQTT

    xTaskCreate(generate_signal_task, "GenerateSignalTask", 4096, NULL, 5, &xsignalReadyTask);
    xTaskCreate(sample_and_analyze_task, "SampleAndAnalyzeTask", 4096, NULL, 5, &xanalysisCompleteTask);
    xTaskCreate(compute_average_task, "ComputeAverageTask", 4096, NULL, 5, &xaverageComputeTask);
    xTaskCreate(oversample_task, "OversampleTask", 4096, NULL, 5, &xoversampleTask);
    //xTaskCreate(transmitting_task, "TransmittingResultsTask", 4096, NULL, 5, &xtransmittingTask);
}
