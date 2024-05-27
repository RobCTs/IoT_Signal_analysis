#include "network.h"
#include "transmitting.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "cJSON.h"


static const char *TAG = "NetworkModule";
esp_mqtt_client_handle_t client;
static uint64_t start_time, end_time;
static int total_bytes_sent = 0;

const char ssid[] = "your_ssid";
const char password[] = "your_password";
const char mqtt_uri[] = "your_uri"; //mqtts fot tls
const char mqtt_cert[] = "-----BEGIN CERTIFICATE-----\n"
"your_certificate\n"
"-----END CERTIFICATE-----";
const char client_cert[] = "-----BEGIN CERTIFICATE-----\n"
"your_cert\n"
"-----END CERTIFICATE-----";
const char client_key[] = "-----BEGIN PRIVATE KEY-----\n"
"your_key\n"
"-----END PRIVATE KEY-----";

//Initialize NVS
esp_err_t  nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS due to no free pages or new version found");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    return ret;
}


void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG, "WiFi started, attempting to connect...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
            static int retry_num = 0;
            if (retry_num < 5) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_wifi_connect();
                retry_num++;
                ESP_LOGI(TAG, "Retry to connect to the AP");
            } else {
                ESP_LOGI(TAG, "Failed to connect to the AP");
            }
            ESP_LOGI(TAG,"WiFi disconnected, retrying...");
    break;
            default:
                ESP_LOGI(TAG, "Unhandled WiFi Event: %" PRId32, event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                break;
            default:
                ESP_LOGI(TAG, "Unhandled IP Event: %" PRIx32, event_id);
                break;
        }
    }
}


void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            start_time = esp_timer_get_time(); 
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            end_time = esp_timer_get_time(); // capture the end time when data is published
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, Latency: %llu us", end_time - start_time);
            total_bytes_sent += event->data_len; // ad the leangth of the data sent to total
            ESP_LOGI(TAG, "Total bytes sent: %d", total_bytes_sent);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA, Topic: %.*s, Data: %.*s", event->topic_len, event->topic, event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Unhandled event %d", event->event_id);
            break;
    }
}


void network_init() {
    ESP_LOGI(TAG, "Initializing network...");
    ESP_LOGI(TAG, "SSID: %s", ssid);

    // Initialize TCP/IP stack
    esp_netif_init();
    esp_event_loop_create_default();
    // Create default WIFI STA
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    // // Configure Wi-Fi storage and mode
    // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Set Wi-Fi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",

        }

    };
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, password);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_LOGI(TAG, "Wi-Fi configured");
    
    //Start and connect
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Wi-Fi configuration finished.");
}

//MQTT
// Low-bandwidth, high-latency, or unreliable networks. 
// It operates on a publish-subscribe model.
void mqtt_app_start() {
    ESP_LOGI(TAG, "Starting MQTT client...");
    ESP_LOGI(TAG, "MQTT URI: %s", mqtt_uri);

    const esp_mqtt_client_config_t mqtt_cfg = {
         //make sure that MQTT_BROKER_URI starts with mqtts://, otw mqtt over ssl will not work!!!
        .broker.address.uri = mqtt_uri,
        .broker.address.transport = MQTT_TRANSPORT_OVER_SSL,
        .broker.verification.certificate = mqtt_cert, //ca_cert
        .credentials = {
            .authentication = {
                .certificate = client_cert, 
                .key = client_key, 
            },
        },
    };
    //Initialize client with configuration
    client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_LOGI(TAG, "MQTT client initialized");
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // Start the client
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT client started");
}
