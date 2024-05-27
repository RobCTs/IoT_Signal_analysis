#ifndef TRANSMITTING_H
#define TRANSMITTING_H

#include "mqtt_client.h"
#include "network.h"

extern esp_mqtt_client_handle_t client;

void mqtt_publish(const char* topic, const char* message);

#endif // TRANSMITTING_H
