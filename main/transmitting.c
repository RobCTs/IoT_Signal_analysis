#include "transmitting.h"
#include "network.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include "cJSON.h"

static const char* TAG = "TransmittingModule";

//mosquitto_sub -h localhost -p 8883 -t "#"


extern esp_mqtt_client_handle_t client;  // Extern declaration for the MQTT client

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
