// WORK IN PROGRESS
// LOCALLY IMPLEMENTED
// NEED TO CONFIGURE IT PROPERLY
// CHECK IF VALID SYNTAX
// JUST THE IDEA I AM GOING AFTER







// #include "transmitting.h"
// #include "esp_log.h"
// #include "mqtt_client.h"
// #include "network.h"
// #include <string.h>

// static const char *TAG = "TransmittingModule";
// extern esp_mqtt_client_handle_t client; // Assuming this is initialized in `network.c`

// // Function to initialize the MQTT connection
// void transmitting_init() {
//     mqtt_app_start(); // configure and connect the MQTT client
// }

// // Generic function to publish data
// void mqtt_publish(const char* topic, const char* message) {
//     if (client == NULL) {
//         ESP_LOGE(TAG, "MQTT client not initialized");
//         return;
//     }

//     int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
//     if (msg_id != -1) {
//         ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
//     } else {
//         ESP_LOGE(TAG, "Failed to send publish");
//     }
// }

// // Function to publish signal data
// void publish_signal_data(float time, float signal_value) {
//     char topic[100];
//     char message[100];
//     sprintf(topic, "sensor/signal");
//     sprintf(message, "%.3f,%.3f", time, signal_value);
//     mqtt_publish(topic, message);
// }

// // Function to publish FFT data
// void publish_fft_data(int index, float fft_magnitude) {
//     char topic[100];
//     char message[100];
//     sprintf(topic, "sensor/fft");
//     sprintf(message, "%d,%.3f", index, fft_magnitude);
//     mqtt_publish(topic, message);
// }

// // Function to publish average data
// void publish_average(float average) {
//     char topic[100] = "sensor/average";
//     char message[100];
//     snprintf(message, sizeof(message), "{\"average\": %.3f}", average);
//     mqtt_publish(topic, message);
// }
