#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"

esp_err_t nvs_init();
void load_configuration();
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
void network_init();
void mqtt_app_start();

#endif // NETWORK_H
