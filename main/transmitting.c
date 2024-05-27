#include "transmitting.h"
#include "network.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include "cJSON.h"

static const char* TAG = "TransmittingModule";

//mosquitto_sub -h localhost -p 8883 -t "#"


extern esp_mqtt_client_handle_t client;  // Extern declaration for the MQTT client

// Function to transmit data as JSON array
void transmit_data(const char* topic, const float* data, int data_len) {
    char message[512];  // Adjust size based on expected data length
    cJSON *root = cJSON_CreateArray();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create cJSON array");
        return;
    }

    for (int i = 0; i < data_len; i++) {
        cJSON_AddItemToArray(root, cJSON_CreateNumber(data[i]));
    }

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);  // Delete the cJSON object as soon as we're done with it

    if (json_string != NULL) {
        snprintf(message, sizeof(message), "%s", json_string);
        cJSON_free(json_string);
    } else {
        ESP_LOGE(TAG, "Failed to print cJSON to string");
        return;
    }

    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
    if (msg_id != -1) {
        ESP_LOGI(TAG, "Data published to topic %s", topic);
    } else {
        ESP_LOGE(TAG, "Failed to publish data");
    }
}

// Generic function to publish data
void mqtt_publish(const char* topic, const char* message) {
    if (client == NULL) {
        ESP_LOGE(TAG, "(mqtt_publish)MQTT client not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
    if (msg_id != -1) {
        ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "Failed to send publish");
     }
    }
