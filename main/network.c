// #include "network.h"
// #include "sampling_and_analyzing.h"
// #include "generating_signal.h"
// #include "esp_log.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "mqtt_client.h"
// #include "esp_netif.h"
// #include "esp_timer.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include <stdlib.h>
// #include <string.h>


// static const char *TAG = "NetworkModule";
// static esp_mqtt_client_handle_t client;
// static uint64_t start_time, end_time;
// static int total_bytes_sent = 0;


// //Initialize NVS
// esp_err_t  nvs_init() {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
// }

// //Check what I really need to store and protect.

// //Store WIFI credentials in NVS
// void store_wifi_credentials(const char* ssid, const char* password) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("wifi_credentials", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "ssid", ssid));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "password", password));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);
// }

// //Retrieve WIFI credentials from NVS
// void retrieve_wifi_credentials(char* ssid, size_t ssid_size, char* password, size_t password_size) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("wifi_credentials", NVS_READONLY, &nvs_handle));
//     size_t ssid_size = sizeof(ssid);
//     size_t password_size = sizeof(password);
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size));
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "password", password, &password_size));
//     nvs_close(nvs_handle);
// }

// //Store MQTT credentials in NVS
// void store_mqtt_credentials(const char* client_key, const char* client_cert) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_credentials", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "key_cert", client_key));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "client_cert", client_cert));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);
// }

// //Retrieve MQTT credentials from NVS
// void retrieve_mqtt_credentials(char* client_key, size_t client_key_size, char* client_cert, size_t client_cert_size) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_credentials", NVS_READONLY, &nvs_handle));
//     size_t username_size = sizeof(client_key);
//     size_t password_size = sizeof(client_cert);
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "client_key", client_key, &client_key_size));
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "client_cert", client_cert, &client_cert_size));
//     nvs_close(nvs_handle);
// }

// //Store MQTT broker URI in NVS
// void store_mqtt_broker_uri(const char* uri) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_broker_uri", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "uri", uri));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);
// }

// //Retrieve MQTT broker URI from NVS
// void retrieve_mqtt_broker_uri(char* uri, size_t uri_size) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_broker_uri", NVS_READONLY, &nvs_handle));
//     size_t uri_size = sizeof(uri);
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "uri", uri, &uri_size));
//     nvs_close(nvs_handle);
// }

// // Should I store certificates in NVS? Read.
// // Store MQTT broker certificate in NVS
// void store_mqtt_broker_cert(const char* cert) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_cert", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "cert", cert));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);
// }

// // Retrieve MQTT broker certificate from NVS
// void retrieve_mqtt_broker_cert(char* cert, size_t cert_size) {
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("mqtt_cert", NVS_READONLY, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "cert", cert, &cert_size));
//     nvs_close(nvs_handle);
// }


// static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     if (event_base == WIFI_EVENT) {
//         switch (event_id) {
//             case WIFI_EVENT_STA_START:
//                 esp_wifi_connect();
//                 ESP_LOGI(TAG, "WiFi started, attempting to connect...");
//                 break;
//             case WIFI_EVENT_STA_DISCONNECTED:
//                 esp_wifi_connect();
//                 ESP_LOGI(TAG, "Disconnected from WiFi, attempting to reconnect...");
//                 break;
//             default:
//                 ESP_LOGI(TAG, "Unhandled WiFi Event: %d", event_id);
//                 break;
//         }
//     } else if (event_base == IP_EVENT) {
//         switch (event_id) {
//             case IP_EVENT_STA_GOT_IP:
//                 ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//                 ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
//                 break;
//             default:
//                 ESP_LOGI(TAG, "Unhandled IP Event: %d", event_id);
//                 break;
//         }
//     }
// }


// static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
//      switch (event->event_id) {
//         case MQTT_EVENT_CONNECTED:
//             ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//             break;
//         case MQTT_EVENT_DISCONNECTED:
//             ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//             break;
//         case MQTT_EVENT_PUBLISHED:
//             end_time = esp_timer_get_time(); // capture the end time when data is published
//             ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, Latency: %llu us", end_time - start_time);
//             total_bytes_sent += event->data_len; // ad the leangth of the data sent to total
//             ESP_LOGI(TAG, "Total bytes sent: %d", total_bytes_sent);
//             break;
//         case MQTT_EVENT_DATA:
//             ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//             break;
//         case MQTT_EVENT_ERROR:
//             ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//             break;
//         default:
//             ESP_LOGI(TAG, "Unhandled event %d", event->event_id);
//             break;
//     }
//     return ESP_OK;
// }


// void network_init() {
//     // Initialize NVS
//     nvs_flash_init();
    
//     // Initialize TCP/IP stack
//     esp_netif_init();
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
    
//     // Create default WIFI STA
//     esp_netif_create_default_wifi_sta();

//     // Initialize Wi-Fi configuration
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
//     // Register event handlers
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

//     // Configure Wi-Fi storage and mode
//     ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
//     // Retrieve Wi-Fi credentials
//     char ssid[32];
//     char password[64];
//     size_t ssid_size = sizeof(ssid);
//     size_t password_size = sizeof(password);
//     retrieve_wifi_credentials(ssid, ssid_size, password, password_size);

//     // Set Wi-Fi configuration
//     wifi_config_t sta_config = {
//         .sta = {
//             .ssid = ssid,
//             .password = password,
//         },
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    
//     //Start and connect
//     ESP_ERROR_CHECK(esp_wifi_start());
//     ESP_ERROR_CHECK(esp_wifi_connect());
// }

// //MQTT
// // Low-bandwidth, high-latency, or unreliable networks. 
// // It operates on a publish-subscribe model.
// void mqtt_app_start() {
//     char mqtt_uri[256];
//     char mqtt_username[64];
//     char mqtt_password[64];
//     char mqtt_cert[1024];
//     char client_cert[1024];
//     char client_key[1024];

//     retrieve_mqtt_broker_uri(mqtt_uri, sizeof(mqtt_uri));
//     retrieve_mqtt_credentials(mqtt_username, sizeof(mqtt_username), mqtt_password, sizeof(mqtt_password));
//     retrieve_mqtt_broker_cert(mqtt_cert, sizeof(mqtt_cert));
//     retrieve_mqtt_credentials(client_cert, sizeof(client_cert), client_key, sizeof(client_key));

//     const esp_mqtt_client_config_t mqtt_cfg = {
//          //make sure that MQTT_BROKER_URI starts with mqtts://, otw mqtt over ssl will not work!!!
//         .broker.address.uri = mqtt_uri,
//         .broker.address.transport = MQTT_TRANSPORT_OVER_SSL,
//         .broker.verification.certificate = mqtt_cert, //ca_cert
//         .credentials = {
//             .authentication = {
//                 .certificate = client_cert, 
//                 .key = client_key, 
//             },
//         },
//     };
//     //Initialize client with configuration
//     client = esp_mqtt_client_init(&mqtt_cfg);

//     esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
//     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client); //in case try NULL?
//     // Start the client
//     esp_mqtt_client_start(client);
// }


// // First try of a function to publish average data to MQTT...
// // Need to check when everything properly set!
// void publish_average(float average) {
//     char payload[50];
//     snprintf(payload, sizeof(payload), "{\"average\": %.3f}", average);
//     start_time = esp_timer_get_time(); // capture the start time before publishing
//     int msg_id = esp_mqtt_client_publish(client, "sensor/average", payload, 0, 1, 0);
//     ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
// }
